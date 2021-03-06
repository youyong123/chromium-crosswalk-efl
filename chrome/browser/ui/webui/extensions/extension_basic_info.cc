// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/extensions/extension_basic_info.h"

#include "base/values.h"
#include "chrome/common/extensions/manifest_url_handler.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/kiosk_mode_info.h"
#include "extensions/common/manifest_handlers/offline_enabled_info.h"
#include "extensions/common/manifest_handlers/options_page_info.h"

namespace {

// Keys in the dictionary returned by GetExtensionBasicInfo().
const char kDescriptionKey[] = "description";
const char kEnabledKey[] = "enabled";
const char kHomepageUrlKey[] = "homepageUrl";
const char kIdKey[] = "id";
const char kNameKey[] = "name";
const char kKioskEnabledKey[] = "kioskEnabled";
const char kKioskOnlyKey[] = "kioskOnly";
const char kOfflineEnabledKey[] = "offlineEnabled";
const char kOptionsUrlKey[] = "optionsUrl";
const char kDetailsUrlKey[] = "detailsUrl";
const char kVersionKey[] = "version";
const char kPackagedAppKey[] = "packagedApp";

}  // namespace

namespace extensions {

void GetExtensionBasicInfo(const Extension* extension,
                           bool enabled,
                           base::DictionaryValue* info) {
  info->SetString(kIdKey, extension->id());
  info->SetString(kNameKey, extension->name());
  info->SetBoolean(kEnabledKey, enabled);
  info->SetBoolean(kKioskEnabledKey,
                   KioskModeInfo::IsKioskEnabled(extension));
  info->SetBoolean(kKioskOnlyKey,
                   KioskModeInfo::IsKioskOnly(extension));
  info->SetBoolean(kOfflineEnabledKey,
                   OfflineEnabledInfo::IsOfflineEnabled(extension));
  info->SetString(kVersionKey, extension->VersionString());
  info->SetString(kDescriptionKey, extension->description());
  info->SetString(
      kOptionsUrlKey,
      OptionsPageInfo::GetOptionsPage(extension).possibly_invalid_spec());
  info->SetString(
      kHomepageUrlKey,
      ManifestURL::GetHomepageURL(extension).possibly_invalid_spec());
  info->SetString(
      kDetailsUrlKey,
      ManifestURL::GetDetailsURL(extension).possibly_invalid_spec());
  info->SetBoolean(kPackagedAppKey, extension->is_platform_app());
}

}  // namespace extensions
