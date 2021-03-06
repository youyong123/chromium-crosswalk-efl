// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/toolbar/browser_actions_container.h"

#include "base/compiler_specific.h"
#include "base/stl_util.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/extension_view_host.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/extensions/browser_action_drag_data.h"
#include "chrome/browser/ui/views/extensions/extension_popup.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/browser_actions_container_observer.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/common/extensions/command.h"
#include "chrome/grit/generated_resources.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/runtime_data.h"
#include "extensions/common/feature_switch.h"
#include "grit/theme_resources.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_view_state.h"
#include "ui/base/dragdrop/drag_utils.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/nine_image_painter_factory.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/resize_area.h"
#include "ui/views/metrics.h"
#include "ui/views/painter.h"
#include "ui/views/widget/widget.h"

using extensions::Extension;

namespace {

// Horizontal spacing before the chevron (if visible).
const int kChevronSpacing = ToolbarView::kStandardSpacing - 2;

// A version of MenuButton with almost empty insets to fit properly on the
// toolbar.
class ChevronMenuButton : public views::MenuButton {
 public:
  ChevronMenuButton(views::ButtonListener* listener,
                    const base::string16& text,
                    views::MenuButtonListener* menu_button_listener,
                    bool show_menu_marker)
      : views::MenuButton(listener,
                          text,
                          menu_button_listener,
                          show_menu_marker) {
  }

  virtual ~ChevronMenuButton() {}

  virtual scoped_ptr<views::LabelButtonBorder> CreateDefaultBorder() const
      OVERRIDE {
    // The chevron resource was designed to not have any insets.
    scoped_ptr<views::LabelButtonBorder> border =
        views::MenuButton::CreateDefaultBorder();
    border->set_insets(gfx::Insets());
    return border.Pass();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ChevronMenuButton);
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// BrowserActionsContainer::DropPosition

struct BrowserActionsContainer::DropPosition {
  DropPosition(size_t row, size_t icon_in_row);

  // The (0-indexed) row into which the action will be dropped.
  size_t row;

  // The (0-indexed) icon in the row before the action will be dropped.
  size_t icon_in_row;
};

BrowserActionsContainer::DropPosition::DropPosition(
    size_t row, size_t icon_in_row)
    : row(row), icon_in_row(icon_in_row) {
}

////////////////////////////////////////////////////////////////////////////////
// BrowserActionsContainer

// static
int BrowserActionsContainer::icons_per_overflow_menu_row_ = 1;

// static
const int BrowserActionsContainer::kItemSpacing = ToolbarView::kStandardSpacing;

// static
bool BrowserActionsContainer::disable_animations_during_testing_ = false;

BrowserActionsContainer::BrowserActionsContainer(
    Browser* browser,
    View* owner_view,
    BrowserActionsContainer* main_container)
    : initialized_(false),
      profile_(browser->profile()),
      browser_(browser),
      owner_view_(owner_view),
      main_container_(main_container),
      popup_owner_(NULL),
      model_(NULL),
      container_width_(0),
      resize_area_(NULL),
      chevron_(NULL),
      overflow_menu_(NULL),
      suppress_chevron_(false),
      resize_amount_(0),
      animation_target_size_(0),
      show_menu_task_factory_(this) {
  set_id(VIEW_ID_BROWSER_ACTION_TOOLBAR);

  model_ = extensions::ExtensionToolbarModel::Get(browser->profile());
  if (model_)
    model_->AddObserver(this);

  bool overflow_experiment =
      extensions::FeatureSwitch::extension_action_redesign()->IsEnabled();
  DCHECK(!in_overflow_mode() || overflow_experiment);

  if (!in_overflow_mode()) {
    extension_keybinding_registry_.reset(new ExtensionKeybindingRegistryViews(
        browser->profile(),
        owner_view->GetFocusManager(),
        extensions::ExtensionKeybindingRegistry::ALL_EXTENSIONS,
        this));

    resize_animation_.reset(new gfx::SlideAnimation(this));
    resize_area_ = new views::ResizeArea(this);
    AddChildView(resize_area_);

    // 'Main' mode doesn't need a chevron overflow when overflow is shown inside
    // the Chrome menu.
    if (!overflow_experiment) {
      chevron_ = new ChevronMenuButton(NULL, base::string16(), this, false);
      chevron_->EnableCanvasFlippingForRTLUI(true);
      chevron_->SetAccessibleName(
          l10n_util::GetStringUTF16(IDS_ACCNAME_EXTENSIONS_CHEVRON));
      chevron_->SetVisible(false);
      AddChildView(chevron_);
    }
  }
}

BrowserActionsContainer::~BrowserActionsContainer() {
  FOR_EACH_OBSERVER(BrowserActionsContainerObserver,
                    observers_,
                    OnBrowserActionsContainerDestroyed());

  if (overflow_menu_)
    overflow_menu_->set_observer(NULL);
  if (model_)
    model_->RemoveObserver(this);
  StopShowFolderDropMenuTimer();
  HideActivePopup();
  DeleteBrowserActionViews();
}

void BrowserActionsContainer::Init() {
  LoadImages();

  // We wait to set the container width until now so that the chevron images
  // will be loaded.  The width calculation needs to know the chevron size.
  if (model_ && model_->extensions_initialized()) {
    container_width_ = GetPreferredWidth();
    SetChevronVisibility();
  }

  initialized_ = true;
}

BrowserActionView* BrowserActionsContainer::GetViewForExtension(
    const Extension* extension) {
  for (BrowserActionViews::iterator view = browser_action_views_.begin();
       view != browser_action_views_.end(); ++view) {
    if ((*view)->extension() == extension)
      return *view;
  }
  return NULL;
}

void BrowserActionsContainer::RefreshBrowserActionViews() {
  for (size_t i = 0; i < browser_action_views_.size(); ++i)
    browser_action_views_[i]->UpdateState();
}

void BrowserActionsContainer::CreateBrowserActionViews() {
  DCHECK(browser_action_views_.empty());
  if (!model_)
    return;

  extensions::ExtensionActionManager* action_manager =
      extensions::ExtensionActionManager::Get(profile_);
  const extensions::ExtensionList& toolbar_items = model_->toolbar_items();
  for (extensions::ExtensionList::const_iterator i(toolbar_items.begin());
       i != toolbar_items.end(); ++i) {
    if (!ShouldDisplayBrowserAction(i->get()))
      continue;

    BrowserActionView* view =
        new BrowserActionView(i->get(),
                              action_manager->GetExtensionAction(**i),
                              browser_,
                              this);
    browser_action_views_.push_back(view);
    AddChildView(view);
  }
}

void BrowserActionsContainer::DeleteBrowserActionViews() {
  HideActivePopup();
  if (overflow_menu_)
    overflow_menu_->NotifyBrowserActionViewsDeleting();
  STLDeleteElements(&browser_action_views_);
}

size_t BrowserActionsContainer::VisibleBrowserActions() const {
  size_t visible_actions = 0;
  for (size_t i = 0; i < browser_action_views_.size(); ++i) {
    if (browser_action_views_[i]->visible())
      ++visible_actions;
  }
  return visible_actions;
}

size_t BrowserActionsContainer::VisibleBrowserActionsAfterAnimation() const {
  if (!animating())
    return VisibleBrowserActions();

  return WidthToIconCount(animation_target_size_);
}

void BrowserActionsContainer::ExecuteExtensionCommand(
    const extensions::Extension* extension,
    const extensions::Command& command) {
  // Global commands are handled by the ExtensionCommandsGlobalRegistry
  // instance.
  DCHECK(!command.global());
  extension_keybinding_registry_->ExecuteCommand(extension->id(),
                                                 command.accelerator());
}

void BrowserActionsContainer::NotifyActionMovedToOverflow() {
  // When an action is moved to overflow, we shrink the size of the container
  // by 1.
  if (!profile_->IsOffTheRecord()) {
    int icon_count = model_->GetVisibleIconCount();
    // Since this happens when an icon moves from the main bar to overflow, we
    // can't possibly have had no visible icons on the main bar.
    DCHECK_NE(0, icon_count);
    if (icon_count == -1)
      icon_count = browser_action_views_.size();
    model_->SetVisibleIconCount(icon_count - 1);
  }
  Animate(gfx::Tween::EASE_OUT,
          VisibleBrowserActionsAfterAnimation() - 1);
}

bool BrowserActionsContainer::ShownInsideMenu() const {
  return in_overflow_mode();
}

void BrowserActionsContainer::OnBrowserActionViewDragDone() {
  ToolbarVisibleCountChanged();
  FOR_EACH_OBSERVER(BrowserActionsContainerObserver,
                    observers_,
                    OnBrowserActionDragDone());
}

views::MenuButton* BrowserActionsContainer::GetOverflowReferenceView() {
  // With traditional overflow, the reference is the chevron. With the
  // redesign, we use the wrench menu instead.
  return chevron_ ?
      chevron_ :
      BrowserView::GetBrowserViewForBrowser(browser_)->toolbar()->app_menu();
}

void BrowserActionsContainer::SetPopupOwner(BrowserActionView* popup_owner) {
  // We should never be setting a popup owner when one already exists, and
  // never unsetting one when one wasn't set.
  DCHECK((!popup_owner_ && popup_owner) ||
         (popup_owner_ && !popup_owner));
  popup_owner_ = popup_owner;
}

void BrowserActionsContainer::HideActivePopup() {
  if (popup_owner_)
    popup_owner_->view_controller()->HidePopup();
}

BrowserActionView* BrowserActionsContainer::GetMainViewForExtension(
    const Extension* extension) {
  return in_overflow_mode() ?
      main_container_->GetViewForExtension(extension) :
      GetViewForExtension(extension);
}

void BrowserActionsContainer::AddObserver(
    BrowserActionsContainerObserver* observer) {
  observers_.AddObserver(observer);
}

void BrowserActionsContainer::RemoveObserver(
    BrowserActionsContainerObserver* observer) {
  observers_.RemoveObserver(observer);
}

gfx::Size BrowserActionsContainer::GetPreferredSize() const {
  if (in_overflow_mode()) {
    int icon_count = GetIconCount();
    // In overflow, we always have a preferred size of a full row (even if we
    // don't use it), and always of at least one row. The parent may decide to
    // show us even when empty, e.g. as a drag target for dragging in icons from
    // the main container.
    int row_count =
        ((std::max(0, icon_count - 1)) / icons_per_overflow_menu_row_) + 1;
    return gfx::Size(
        IconCountToWidth(icons_per_overflow_menu_row_, false),
        row_count * IconHeight());
  }

  // If there are no actions to show, then don't show the container at all.
  if (browser_action_views_.empty())
    return gfx::Size();

  // We calculate the size of the view by taking the current width and
  // subtracting resize_amount_ (the latter represents how far the user is
  // resizing the view or, if animating the snapping, how far to animate it).
  // But we also clamp it to a minimum size and the maximum size, so that the
  // container can never shrink too far or take up more space than it needs.
  // In other words: MinimumNonemptyWidth() < width() - resize < ClampTo(MAX).
  int preferred_width = std::min(
      std::max(MinimumNonemptyWidth(), container_width_ - resize_amount_),
      IconCountToWidth(-1, false));
  return gfx::Size(preferred_width, IconHeight());
}

int BrowserActionsContainer::GetHeightForWidth(int width) const {
  if (in_overflow_mode())
    icons_per_overflow_menu_row_ = (width - kItemSpacing) / IconWidth(true);
  return GetPreferredSize().height();
}

gfx::Size BrowserActionsContainer::GetMinimumSize() const {
  int min_width = std::min(MinimumNonemptyWidth(), IconCountToWidth(-1, false));
  return gfx::Size(min_width, IconHeight());
}

void BrowserActionsContainer::Layout() {
  if (browser_action_views_.empty()) {
    SetVisible(false);
    return;
  }

  SetVisible(true);
  if (resize_area_)
    resize_area_->SetBounds(0, 0, kItemSpacing, height());

  // If the icons don't all fit, show the chevron (unless suppressed).
  int max_x = GetPreferredSize().width();
  if ((IconCountToWidth(-1, false) > max_x) && !suppress_chevron_ && chevron_) {
    chevron_->SetVisible(true);
    gfx::Size chevron_size(chevron_->GetPreferredSize());
    max_x -=
        ToolbarView::kStandardSpacing + chevron_size.width() + kChevronSpacing;
    chevron_->SetBounds(
        width() - ToolbarView::kStandardSpacing - chevron_size.width(),
        0,
        chevron_size.width(),
        chevron_size.height());
  } else if (chevron_) {
    chevron_->SetVisible(false);
  }

  // Now draw the icons for the browser actions in the available space.
  int icon_width = IconWidth(false);
  if (in_overflow_mode()) {
    for (size_t i = 0;
         i < main_container_->VisibleBrowserActionsAfterAnimation(); ++i) {
      // Ensure that any browser actions shown in the main view are hidden in
      // the overflow view.
      browser_action_views_[i]->SetVisible(false);
    }

    for (size_t i = main_container_->VisibleBrowserActionsAfterAnimation();
         i < browser_action_views_.size(); ++i) {
      BrowserActionView* view = browser_action_views_[i];
      size_t index = i - main_container_->VisibleBrowserActionsAfterAnimation();
      int row_index = static_cast<int>(index) / icons_per_overflow_menu_row_;
      int x = kItemSpacing + (index * IconWidth(true)) -
          (row_index * IconWidth(true) * icons_per_overflow_menu_row_);
      gfx::Rect rect_bounds(
          x, IconHeight() * row_index, icon_width, IconHeight());
      view->SetBoundsRect(rect_bounds);
      view->SetVisible(true);
    }
  } else {
    for (BrowserActionViews::const_iterator it = browser_action_views_.begin();
         it < browser_action_views_.end(); ++it) {
      BrowserActionView* view = *it;
      int x = ToolbarView::kStandardSpacing +
          ((it - browser_action_views_.begin()) * IconWidth(true));
      view->SetVisible(x + icon_width <= max_x);
      if (view->visible())
        view->SetBounds(x, 0, icon_width, IconHeight());
    }
  }
}

bool BrowserActionsContainer::GetDropFormats(
    int* formats,
    std::set<OSExchangeData::CustomFormat>* custom_formats) {
  return BrowserActionDragData::GetDropFormats(custom_formats);
}

bool BrowserActionsContainer::AreDropTypesRequired() {
  return BrowserActionDragData::AreDropTypesRequired();
}

bool BrowserActionsContainer::CanDrop(const OSExchangeData& data) {
  return BrowserActionDragData::CanDrop(data, profile_);
}

int BrowserActionsContainer::OnDragUpdated(
    const ui::DropTargetEvent& event) {
  // First check if we are above the chevron (overflow) menu.
  if (chevron_ && GetEventHandlerForPoint(event.location()) == chevron_) {
    if (!show_menu_task_factory_.HasWeakPtrs() && !overflow_menu_)
      StartShowFolderDropMenuTimer();
    return ui::DragDropTypes::DRAG_MOVE;
  }
  StopShowFolderDropMenuTimer();

  size_t row_index = 0;
  size_t before_icon_in_row = 0;
  // If there are no visible browser actions (such as when dragging an icon to
  // an empty overflow/main container), then 0, 0 for row, column is correct.
  if (VisibleBrowserActions() != 0) {
    // Figure out where to display the indicator. This is a complex calculation:

    // First, we subtract out the padding to the left of the icon area, which is
    // ToolbarView::kStandardSpacing. If we're right-to-left, we also mirror the
    // event.x() so that our calculations are consistent with left-to-right.
    int offset_into_icon_area =
        GetMirroredXInView(event.x()) - ToolbarView::kStandardSpacing;

    // Next, figure out what row we're on. This only matters for overflow mode,
    // but the calculation is the same for both.
    row_index = event.y() / IconHeight();

    // Sanity check - we should never be on a different row in the main
    // container.
    DCHECK(in_overflow_mode() || row_index == 0);

    // Next, we determine which icon to place the indicator in front of. We want
    // to place the indicator in front of icon n when the cursor is between the
    // midpoints of icons (n - 1) and n.  To do this we take the offset into the
    // icon area and transform it as follows:
    //
    // Real icon area:
    //   0   a     *  b        c
    //   |   |        |        |
    //   |[IC|ON]  [IC|ON]  [IC|ON]
    // We want to be before icon 0 for 0 < x <= a, icon 1 for a < x <= b, etc.
    // Here the "*" represents the offset into the icon area, and since it's
    // between a and b, we want to return "1".
    //
    // Transformed "icon area":
    //   0        a     *  b        c
    //   |        |        |        |
    //   |[ICON]  |[ICON]  |[ICON]  |
    // If we shift both our offset and our divider points later by half an icon
    // plus one spacing unit, then it becomes very easy to calculate how many
    // divider points we've passed, because they're the multiples of "one icon
    // plus padding".
    int before_icon_unclamped =
        (offset_into_icon_area + (IconWidth(false) / 2) +
        kItemSpacing) / IconWidth(true);

    // We need to figure out how many icons are visible on the relevant row.
    // In the main container, this will just be the visible actions.
    int visible_icons_on_row = VisibleBrowserActionsAfterAnimation();
    if (in_overflow_mode()) {
      // If this is the final row of the overflow, then this is the remainder of
      // visible icons. Otherwise, it's a full row (kIconsPerRow).
      visible_icons_on_row =
          row_index ==
              static_cast<size_t>(visible_icons_on_row /
                                  icons_per_overflow_menu_row_) ?
                  visible_icons_on_row % icons_per_overflow_menu_row_ :
                  icons_per_overflow_menu_row_;
    }

    // Because the user can drag outside the container bounds, we need to clamp
    // to the valid range. Note that the maximum allowable value is (num icons),
    // not (num icons - 1), because we represent the indicator being past the
    // last icon as being "before the (last + 1) icon".
    before_icon_in_row =
        std::min(std::max(before_icon_unclamped, 0), visible_icons_on_row);
  }

  if (!drop_position_.get() ||
      !(drop_position_->row == row_index &&
        drop_position_->icon_in_row == before_icon_in_row)) {
    drop_position_.reset(new DropPosition(row_index, before_icon_in_row));
    SchedulePaint();
  }

  return ui::DragDropTypes::DRAG_MOVE;
}

void BrowserActionsContainer::OnDragExited() {
  StopShowFolderDropMenuTimer();
  drop_position_.reset();
  SchedulePaint();
}

int BrowserActionsContainer::OnPerformDrop(
    const ui::DropTargetEvent& event) {
  BrowserActionDragData data;
  if (!data.Read(event.data()))
    return ui::DragDropTypes::DRAG_NONE;

  // Make sure we have the same view as we started with.
  DCHECK_EQ(browser_action_views_[data.index()]->extension()->id(),
            data.id());
  DCHECK(model_);

  size_t i = drop_position_->row * icons_per_overflow_menu_row_ +
             drop_position_->icon_in_row;
  if (in_overflow_mode())
    i += main_container_->VisibleBrowserActionsAfterAnimation();
  // |i| now points to the item to the right of the drop indicator*, which is
  // correct when dragging an icon to the left. When dragging to the right,
  // however, we want the icon being dragged to get the index of the item to
  // the left of the drop indicator, so we subtract one.
  // * Well, it can also point to the end, but not when dragging to the left. :)
  if (i > data.index())
    --i;

  if (profile_->IsOffTheRecord())
    i = model_->IncognitoIndexToOriginal(i);

  // If this was a drag between containers, we will have to adjust the number of
  // visible icons.
  bool drag_between_containers =
      !browser_action_views_[data.index()]->visible();
  model_->MoveExtensionIcon(
      browser_action_views_[data.index()]->extension(), i);

  if (drag_between_containers) {
    // Add one for the dropped icon.
    size_t new_icon_count = VisibleBrowserActionsAfterAnimation() + 1;

    // Let the main container update the model.
    if (in_overflow_mode())
      main_container_->NotifyActionMovedToOverflow();
    else if (!profile_->IsOffTheRecord())  // This is the main container.
      model_->SetVisibleIconCount(model_->GetVisibleIconCount() + 1);

    // The size changed, so we need to animate.
    Animate(gfx::Tween::EASE_OUT, new_icon_count);
  }

  OnDragExited();  // Perform clean up after dragging.
  return ui::DragDropTypes::DRAG_MOVE;
}

void BrowserActionsContainer::GetAccessibleState(
    ui::AXViewState* state) {
  state->role = ui::AX_ROLE_GROUP;
  state->name = l10n_util::GetStringUTF16(IDS_ACCNAME_EXTENSIONS);
}

void BrowserActionsContainer::OnMenuButtonClicked(views::View* source,
                                                  const gfx::Point& point) {
  if (source == chevron_) {
    overflow_menu_ =
        new BrowserActionOverflowMenuController(this,
                                                browser_,
                                                chevron_,
                                                browser_action_views_,
                                                VisibleBrowserActions(),
                                                false);
    overflow_menu_->set_observer(this);
    overflow_menu_->RunMenu(GetWidget());
  }
}

void BrowserActionsContainer::WriteDragDataForView(View* sender,
                                                   const gfx::Point& press_pt,
                                                   OSExchangeData* data) {
  DCHECK(data);

  for (size_t i = 0; i < browser_action_views_.size(); ++i) {
    BrowserActionView* view = browser_action_views_[i];
    if (view == sender) {
      // Set the dragging image for the icon.
      gfx::ImageSkia badge(view->GetIconWithBadge());
      drag_utils::SetDragImageOnDataObject(badge,
                                           press_pt.OffsetFromOrigin(),
                                           data);

      // Fill in the remaining info.
      BrowserActionDragData drag_data(view->extension()->id(), i);
      drag_data.Write(profile_, data);
      break;
    }
  }
}

int BrowserActionsContainer::GetDragOperationsForView(View* sender,
                                                      const gfx::Point& p) {
  return ui::DragDropTypes::DRAG_MOVE;
}

bool BrowserActionsContainer::CanStartDragForView(View* sender,
                                                  const gfx::Point& press_pt,
                                                  const gfx::Point& p) {
  // We don't allow dragging while we're highlighting.
  return !model_->is_highlighting();
}

void BrowserActionsContainer::OnResize(int resize_amount, bool done_resizing) {
  if (!done_resizing) {
    resize_amount_ = resize_amount;
    OnBrowserActionVisibilityChanged();
    return;
  }

  // Up until now we've only been modifying the resize_amount, but now it is
  // time to set the container size to the size we have resized to, and then
  // animate to the nearest icon count size if necessary (which may be 0).
  int max_width = IconCountToWidth(-1, false);
  container_width_ =
      std::min(std::max(0, container_width_ - resize_amount), max_width);

  // Save off the desired number of visible icons.  We do this now instead of at
  // the end of the animation so that even if the browser is shut down while
  // animating, the right value will be restored on next run.
  // NOTE: Don't save the icon count in incognito because there may be fewer
  // icons in that mode. The result is that the container in a normal window is
  // always at least as wide as in an incognito window.
  int visible_icons = WidthToIconCount(container_width_);
  if (!profile_->IsOffTheRecord())
    model_->SetVisibleIconCount(visible_icons);
  Animate(gfx::Tween::EASE_OUT, visible_icons);
}

void BrowserActionsContainer::AnimationProgressed(
    const gfx::Animation* animation) {
  DCHECK_EQ(resize_animation_.get(), animation);
  resize_amount_ = static_cast<int>(resize_animation_->GetCurrentValue() *
      (container_width_ - animation_target_size_));
  OnBrowserActionVisibilityChanged();
}

void BrowserActionsContainer::AnimationEnded(const gfx::Animation* animation) {
  container_width_ = animation_target_size_;
  animation_target_size_ = 0;
  resize_amount_ = 0;
  suppress_chevron_ = false;
  SetChevronVisibility();
  OnBrowserActionVisibilityChanged();

  FOR_EACH_OBSERVER(BrowserActionsContainerObserver,
                    observers_,
                    OnBrowserActionsContainerAnimationEnded());
}

void BrowserActionsContainer::NotifyMenuDeleted(
    BrowserActionOverflowMenuController* controller) {
  DCHECK_EQ(overflow_menu_, controller);
  overflow_menu_ = NULL;
}

content::WebContents* BrowserActionsContainer::GetCurrentWebContents() {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

extensions::ActiveTabPermissionGranter*
    BrowserActionsContainer::GetActiveTabPermissionGranter() {
  content::WebContents* web_contents =
      browser_->tab_strip_model()->GetActiveWebContents();
  if (!web_contents)
    return NULL;
  return extensions::TabHelper::FromWebContents(web_contents)->
      active_tab_permission_granter();
}

ExtensionPopup* BrowserActionsContainer::TestGetPopup() {
  return popup_owner_ ? popup_owner_->view_controller()->popup() : NULL;
}

void BrowserActionsContainer::TestSetIconVisibilityCount(size_t icons) {
  model_->SetVisibleIconCountForTest(icons);
}

void BrowserActionsContainer::OnPaint(gfx::Canvas* canvas) {
  // If the views haven't been initialized yet, wait for the next call to
  // paint (one will be triggered by entering highlight mode).
  if (model_->is_highlighting() && !browser_action_views_.empty()) {
    views::Painter::PaintPainterAt(
        canvas, highlight_painter_.get(), GetLocalBounds());
  }

  // TODO(sky/glen): Instead of using a drop indicator, animate the icons while
  // dragging (like we do for tab dragging).
  if (drop_position_.get()) {
    // The two-pixel width drop indicator.
    static const int kDropIndicatorWidth = 2;

    // Convert back to a pixel offset into the container.  First find the X
    // coordinate of the drop icon.
    int drop_icon_x = ToolbarView::kStandardSpacing +
        (drop_position_->icon_in_row * IconWidth(true));
    // Next, find the space before the drop icon. This will either be
    // kItemSpacing or ToolbarView::kStandardSpacing, depending on whether this
    // is the first icon.
    // NOTE: Right now, these are the same. But let's do this right for if they
    // ever aren't.
    int space_before_drop_icon = drop_position_->icon_in_row == 0 ?
        ToolbarView::kStandardSpacing : kItemSpacing;
    // Now place the drop indicator halfway between this and the end of the
    // previous icon.  If there is an odd amount of available space between the
    // two icons (or the icon and the address bar) after subtracting the drop
    // indicator width, this calculation puts the extra pixel on the left side
    // of the indicator, since when the indicator is between the address bar and
    // the first icon, it looks better closer to the icon.
    int drop_indicator_x = drop_icon_x -
        ((space_before_drop_icon + kDropIndicatorWidth) / 2);
    int row_height = IconHeight();
    int drop_indicator_y = row_height * drop_position_->row;
    gfx::Rect indicator_bounds(drop_indicator_x,
                               drop_indicator_y,
                               kDropIndicatorWidth,
                               row_height);
    indicator_bounds.set_x(GetMirroredXForRect(indicator_bounds));

    // Color of the drop indicator.
    static const SkColor kDropIndicatorColor = SK_ColorBLACK;
    canvas->FillRect(indicator_bounds, kDropIndicatorColor);
  }
}

void BrowserActionsContainer::OnThemeChanged() {
  LoadImages();
}

void BrowserActionsContainer::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  // No extensions (e.g., incognito).
  if (!model_)
    return;

  if (details.is_add && details.child == this) {
    // Initial toolbar button creation and placement in the widget hierarchy.
    // We do this here instead of in the constructor because AddBrowserAction
    // calls Layout on the Toolbar, which needs this object to be constructed
    // before its Layout function is called.
    CreateBrowserActionViews();
  }
}

// static
int BrowserActionsContainer::IconWidth(bool include_padding) {
  static bool initialized = false;
  static int icon_width = 0;
  if (!initialized) {
    initialized = true;
    icon_width = ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
        IDR_BROWSER_ACTION)->width();
  }
  return icon_width + (include_padding ? kItemSpacing : 0);
}

// static
int BrowserActionsContainer::IconHeight() {
  static bool initialized = false;
  static int icon_height = 0;
  if (!initialized) {
    initialized = true;
    icon_height = ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
        IDR_BROWSER_ACTION)->height();
  }
  return icon_height;
}

void BrowserActionsContainer::ToolbarExtensionAdded(const Extension* extension,
                                                    int index) {
#if defined(DEBUG)
  for (size_t i = 0; i < browser_action_views_.size(); ++i) {
    DCHECK(browser_action_views_[i]->extension() != extension) <<
           "Asked to add a browser action view for an extension that already "
           "exists.";
  }
#endif
  CloseOverflowMenu();

  if (!ShouldDisplayBrowserAction(extension))
    return;

  // Add the new browser action to the vector and the view hierarchy.
  if (profile_->IsOffTheRecord())
    index = model_->OriginalIndexToIncognito(index);
  BrowserActionView* view =
      new BrowserActionView(extension,
                            extensions::ExtensionActionManager::Get(profile_)->
                                GetExtensionAction(*extension),
                            browser_,
                            this);
  browser_action_views_.insert(browser_action_views_.begin() + index, view);
  AddChildViewAt(view, index);

  // If we are still initializing the container, don't bother animating.
  if (!model_->extensions_initialized())
    return;

  // If this is just an upgrade, then don't worry about resizing.
  if (!extensions::ExtensionSystem::Get(profile_)->runtime_data()->
          IsBeingUpgraded(extension)) {
    // We need to resize if either:
    // - The container is set to display all icons (visible count = -1), or
    // - The container will need to expand to include the chevron. This can
    //   happen when the container is set to display <n> icons, where <n> is
    //   the number of icons before the new icon. With the new icon, the chevron
    //   will need to be displayed.
    int model_icon_count = model_->GetVisibleIconCount();
    if (model_icon_count == -1 ||
        (static_cast<size_t>(model_icon_count) < browser_action_views_.size() &&
         (chevron_ && !chevron_->visible()))) {
      suppress_chevron_ = true;
      Animate(gfx::Tween::LINEAR, GetIconCount());
      return;
    }
  }

  // Otherwise, we don't have to resize, so just redraw the (possibly modified)
  // visible icon set.
  OnBrowserActionVisibilityChanged();
}

void BrowserActionsContainer::ToolbarExtensionRemoved(
    const Extension* extension) {
  CloseOverflowMenu();

  size_t visible_actions = VisibleBrowserActionsAfterAnimation();
  for (BrowserActionViews::iterator i(browser_action_views_.begin());
       i != browser_action_views_.end(); ++i) {
    if ((*i)->extension() == extension) {
      delete *i;
      browser_action_views_.erase(i);

      // If the extension is being upgraded we don't want the bar to shrink
      // because the icon is just going to get re-added to the same location.
      if (extensions::ExtensionSystem::Get(profile_)->runtime_data()->
              IsBeingUpgraded(extension))
        return;

      if (browser_action_views_.size() > visible_actions) {
        // If we have more icons than we can show, then we must not be changing
        // the container size (since we either removed an icon from the main
        // area and one from the overflow list will have shifted in, or we
        // removed an entry directly from the overflow list).
        OnBrowserActionVisibilityChanged();
      } else {
        // Either we went from overflow to no-overflow, or we shrunk the no-
        // overflow container by 1.  Either way the size changed, so animate.
        if (chevron_)
          chevron_->SetVisible(false);
        Animate(gfx::Tween::EASE_OUT, browser_action_views_.size());
      }
      return;  // We have found the action to remove, bail out.
    }
  }
}

void BrowserActionsContainer::ToolbarExtensionMoved(const Extension* extension,
                                                    int index) {
  if (!ShouldDisplayBrowserAction(extension))
    return;

  if (profile_->IsOffTheRecord())
    index = model_->OriginalIndexToIncognito(index);

  DCHECK(index >= 0 && index < static_cast<int>(browser_action_views_.size()));

  BrowserActionViews::iterator iter = browser_action_views_.begin();
  int old_index = 0;
  while (iter != browser_action_views_.end() &&
         (*iter)->extension() != extension) {
    ++iter;
    ++old_index;
  }

  DCHECK(iter != browser_action_views_.end());
  if (old_index == index)
    return;  // Already in place.

  BrowserActionView* moved_view = *iter;
  browser_action_views_.erase(iter);
  browser_action_views_.insert(
      browser_action_views_.begin() + index, moved_view);

  Layout();
  SchedulePaint();
}

void BrowserActionsContainer::ToolbarExtensionUpdated(
    const Extension* extension) {
  BrowserActionView* view = GetViewForExtension(extension);
  if (view)
    view->UpdateState();
}

bool BrowserActionsContainer::ShowExtensionActionPopup(
    const Extension* extension,
    bool grant_active_tab) {
  // Don't override another popup, and only show in the active window.
  if (popup_owner_ || !browser_->window()->IsActive())
    return false;

  BrowserActionView* view = GetViewForExtension(extension);
  return view && view->view_controller()->ExecuteAction(ExtensionPopup::SHOW,
                                                        grant_active_tab);
}

void BrowserActionsContainer::ToolbarVisibleCountChanged() {
  if (GetPreferredWidth() != container_width_)
    Animate(gfx::Tween::EASE_OUT, GetIconCount());
}

void BrowserActionsContainer::ToolbarHighlightModeChanged(
    bool is_highlighting) {
  // The visual highlighting is done in OnPaint(). It's a bit of a pain that
  // we delete and recreate everything here, but given everything else going on
  // (the lack of highlight, n more extensions appearing, etc), it's not worth
  // the extra complexity to create and insert only the new extensions.
  DeleteBrowserActionViews();
  CreateBrowserActionViews();
  Animate(gfx::Tween::LINEAR, GetIconCount());
}

Browser* BrowserActionsContainer::GetBrowser() {
  return browser_;
}

void BrowserActionsContainer::LoadImages() {
  ui::ThemeProvider* tp = GetThemeProvider();
  if (!tp || !chevron_)
    return;

  chevron_->SetImage(views::Button::STATE_NORMAL,
                     *tp->GetImageSkiaNamed(IDR_BROWSER_ACTIONS_OVERFLOW));

  const int kImages[] = IMAGE_GRID(IDR_DEVELOPER_MODE_HIGHLIGHT);
  highlight_painter_.reset(views::Painter::CreateImageGridPainter(kImages));
}

void BrowserActionsContainer::OnBrowserActionVisibilityChanged() {
  SetVisible(!browser_action_views_.empty());
  if (owner_view_) {
    owner_view_->Layout();
    owner_view_->SchedulePaint();
  } else {
    // In overflow mode, we don't have an owner view, but we still have to
    // update ourselves.
    Layout();
    SchedulePaint();
  }
}

int BrowserActionsContainer::GetPreferredWidth() {
  size_t visible_actions = GetIconCount();
  return IconCountToWidth(
      visible_actions,
      chevron_ && visible_actions < browser_action_views_.size());
}

void BrowserActionsContainer::SetChevronVisibility() {
  if (chevron_) {
    chevron_->SetVisible(
        VisibleBrowserActionsAfterAnimation() < browser_action_views_.size());
  }
}

void BrowserActionsContainer::CloseOverflowMenu() {
  if (overflow_menu_)
    overflow_menu_->CancelMenu();
}

void BrowserActionsContainer::StopShowFolderDropMenuTimer() {
  show_menu_task_factory_.InvalidateWeakPtrs();
}

void BrowserActionsContainer::StartShowFolderDropMenuTimer() {
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&BrowserActionsContainer::ShowDropFolder,
                 show_menu_task_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(views::GetMenuShowDelay()));
}

void BrowserActionsContainer::ShowDropFolder() {
  DCHECK(!overflow_menu_);
  overflow_menu_ =
      new BrowserActionOverflowMenuController(this,
                                              browser_,
                                              chevron_,
                                              browser_action_views_,
                                              VisibleBrowserActions(),
                                              true);
  overflow_menu_->set_observer(this);
  overflow_menu_->RunMenu(GetWidget());
}

int BrowserActionsContainer::IconCountToWidth(int icons,
                                              bool display_chevron) const {
  if (icons < 0)
    icons = browser_action_views_.size();
  if ((icons == 0) && !display_chevron)
    return ToolbarView::kStandardSpacing;
  int icons_size =
      (icons == 0) ? 0 : ((icons * IconWidth(true)) - kItemSpacing);
  int chevron_size = chevron_ && display_chevron ?
      (kChevronSpacing + chevron_->GetPreferredSize().width()) : 0;
  // In overflow mode, our padding is to use item spacing on either end (just so
  // we can see the drop indicator). Otherwise we use the standard toolbar
  // spacing.
  // Note: These are actually the same thing, but, on the offchance one
  // changes, let's get it right.
  int padding =
      2 * (in_overflow_mode() ? kItemSpacing : ToolbarView::kStandardSpacing);
  return icons_size + chevron_size + padding;
}

size_t BrowserActionsContainer::WidthToIconCount(int pixels) const {
  // Check for widths large enough to show the entire icon set.
  if (pixels >= IconCountToWidth(-1, false))
    return browser_action_views_.size();

  // We reserve space for the padding on either side of the toolbar...
  int available_space = pixels - (ToolbarView::kStandardSpacing * 2);
  // ... and, if the chevron is enabled, the chevron.
  if (chevron_)
    available_space -= (chevron_->GetPreferredSize().width() + kChevronSpacing);

  // Now we add an extra between-item padding value so the space can be divided
  // evenly by (size of icon with padding).
  return static_cast<size_t>(
      std::max(0, available_space + kItemSpacing) / IconWidth(true));
}

int BrowserActionsContainer::MinimumNonemptyWidth() const {
  if (!chevron_)
    return ToolbarView::kStandardSpacing;
  return (ToolbarView::kStandardSpacing * 2) + kChevronSpacing +
      chevron_->GetPreferredSize().width();
}

void BrowserActionsContainer::Animate(gfx::Tween::Type tween_type,
                                      size_t num_visible_icons) {
  int target_size = IconCountToWidth(num_visible_icons,
      num_visible_icons < browser_action_views_.size());
  if (resize_animation_ && !disable_animations_during_testing_) {
    // Animate! We have to set the animation_target_size_ after calling Reset(),
    // because that could end up calling AnimationEnded which clears the value.
    resize_animation_->Reset();
    resize_animation_->SetTweenType(tween_type);
    animation_target_size_ = target_size;
    resize_animation_->Show();
  } else {
    animation_target_size_ = target_size;
    AnimationEnded(resize_animation_.get());
  }
}

bool BrowserActionsContainer::ShouldDisplayBrowserAction(
    const Extension* extension) const {
  // Only display incognito-enabled extensions while in incognito mode.
  return !profile_->IsOffTheRecord() ||
      extensions::util::IsIncognitoEnabled(extension->id(), profile_);
}

size_t BrowserActionsContainer::GetIconCount() const {
  if (!model_)
    return 0u;

  const extensions::ExtensionList& extensions = model_->toolbar_items();

  // Find the absolute value for the model's visible count.
  int model_visible_size = model_->GetVisibleIconCount();
  size_t absolute_model_visible_size =
      model_visible_size == -1 ? extensions.size() : model_visible_size;

  // Find the number of icons which could be displayed.
  size_t displayable_icon_count = 0u;
  size_t main_displayed = 0u;
  for (size_t i = 0; i < extensions.size(); ++i) {
    // Should there be an icon for this extension at all?
    if (ShouldDisplayBrowserAction(extensions[i].get())) {
      ++displayable_icon_count;
      // Should we display it on the main bar? If this is an incognito window,
      // icons have the same overflow status they do in a regular window.
      main_displayed += i < absolute_model_visible_size ? 1u : 0u;
    }
  }

  // If this is an existing (initialized) container from an incognito profile,
  // we can't trust the model (because the incognito bars don't adjust model
  // settings). Instead, we go off what we currently have displayed.
  if (initialized_ && profile_->IsOffTheRecord()) {
    main_displayed = in_overflow_mode() ?
        main_container_->VisibleBrowserActionsAfterAnimation() :
        VisibleBrowserActionsAfterAnimation();
  }

  // The overflow displays any (displayable) icons not shown by the main bar.
  return in_overflow_mode() ?
      displayable_icon_count - main_displayed : main_displayed;
}
