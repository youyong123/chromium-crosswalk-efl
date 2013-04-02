// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill;

import android.graphics.Bitmap;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.chrome.browser.autofill.AutofillDialog.AutofillDialogDelegate;
import org.chromium.ui.gfx.NativeWindow;

/**
* JNI call glue for AutofillDialog C++ and Java objects.
*/
@JNINamespace("autofill")
public class AutofillDialogGlue implements AutofillDialogDelegate,
        AutofillDialogAccountHelper.SignInContinuation {
    @SuppressWarnings("unused")
    private final int mNativeDialogPopup;
    private final AutofillDialog mAutofillDialog;
    private final AutofillDialogAccountHelper mAccountHelper;

    public AutofillDialogGlue(int nativeAutofillDialogViewAndroid, NativeWindow nativeWindow) {
        mNativeDialogPopup = nativeAutofillDialogViewAndroid;
        mAccountHelper = new AutofillDialogAccountHelper(this, nativeWindow.getContext());

        mAutofillDialog = new AutofillDialog(nativeWindow.getContext(), this);
        mAutofillDialog.show();
    }

    @CalledByNative
    private static AutofillDialogGlue create(int nativeAutofillDialogViewAndroid,
            NativeWindow nativeWindow) {
        return new AutofillDialogGlue(nativeAutofillDialogViewAndroid, nativeWindow);
    }

    /**
     * @see AutofillDialog#updateNotificationArea(AutofillDialogNotification[])
     */
    @CalledByNative
    private void updateNotificationArea(AutofillDialogNotification[] notifications) {
        mAutofillDialog.updateNotificationArea(notifications);
    }

    /**
     * @see AutofillDialog#updateSection(int, boolean, AutofillDialogField[],
     *                                   AutofillDialogMenuItem[], int)
     */
    @CalledByNative
    private void updateSection(int section, boolean visible, AutofillDialogField[] dialogInputs,
            AutofillDialogMenuItem[] menuItems, int selectedMenuItem) {
        mAutofillDialog.updateSection(section, visible, dialogInputs, menuItems, selectedMenuItem);
    }

    /**
     * Notifies the dialog that the underlying model is changed and all sections will be updated.
     * @param fetchingIsActive If true, the data is being fetched and is not yet available.
     */
    @CalledByNative
    private void modelChanged(boolean fetchingIsActive) {
        mAutofillDialog.modelChanged(fetchingIsActive);
    }

    /**
     * Updates the account chooser of Autofill dialog.
     * @param accountNames List of accounts to be shown.
     * @param selectedAccountIndex Index of the currently selected account.
     */
    @CalledByNative
    private void updateAccountChooser(String[] accountNames, int selectedAccountIndex) {
        mAutofillDialog.updateAccountChooser(accountNames, selectedAccountIndex);
    }

    /**
     * @see AutofillDialog#getSection(int)
     */
    @CalledByNative
    private AutofillDialogField[] getSection(int section) {
        return mAutofillDialog.getSection(section);
    }

    /**
     * @see AutofillDialog#getCvc()
     */
    @CalledByNative
    private String getCvc() {
        return mAutofillDialog.getCvc();
    }

    /**
     * @see AutofillDialog#shouldUseBillingForShipping()
     */
    @CalledByNative
    private boolean shouldUseBillingForShipping() {
        return mAutofillDialog.shouldUseBillingForShipping();
    }

    /**
     * @see AutofillDialog#shouldSaveDetailsInWallet()
     */
    @CalledByNative
    private boolean shouldSaveDetailsInWallet() {
        return mAutofillDialog.shouldSaveDetailsInWallet();
    }

    /**
     * @see AutofillDialog#shouldSaveDetailsLocally()
     */
    @CalledByNative
    private boolean shouldSaveDetailsLocally() {
        return mAutofillDialog.shouldSaveDetailsLocally();
    }

    /**
     * @see AutofillDialog#updateProgressBar(double)
     */
    @CalledByNative
    private void updateProgressBar(double value) {
        mAutofillDialog.updateProgressBar(value);
    }

    /**
     * Starts an automatic sign-in attempt for a given account.
     * @param accountName An account name (email) to sign into.
     */
    @CalledByNative
    private void startAutomaticSignIn(String accountName) {
        mAccountHelper.startTokensGeneration(accountName);
    }

    /**
     * @return An array of Google account emails the user has.
     */
    @CalledByNative
    private String[] getUserAccountNames() {
        return mAccountHelper.getAccountNames();
    }

    // AutofillDialogAccountHelper.SignInContinuation implementation.
    @Override
    public void onTokensGenerationSuccess(String accountName, String sid, String lsid) {
        nativeContinueAutomaticSignin(mNativeDialogPopup, accountName, sid, lsid);
    }

    @Override
    public void onTokensGenerationFailure() {
        nativeContinueAutomaticSignin(mNativeDialogPopup, "", "", "");
    }

    // AutofillDialogDelegate implementation ------------------------------------------------------

    @Override
    public void itemSelected(int section, int index) {
        nativeItemSelected(mNativeDialogPopup, section, index);
    }

    @Override
    public void accountSelected(int index) {
        nativeAccountSelected(mNativeDialogPopup, index);
    }

    @Override
    public void editingStart(int section) {
        nativeEditingStart(mNativeDialogPopup, section);
    }

    @Override
    public void editingComplete(int section) {
        nativeEditingComplete(mNativeDialogPopup, section);
    }

    @Override
    public void editingCancel(int section) {
        nativeEditingCancel(mNativeDialogPopup, section);
    }

    @Override
    public void dialogSubmit() {
        nativeDialogSubmit(mNativeDialogPopup);
    }

    @Override
    public void dialogCancel() {
        nativeDialogCancel(mNativeDialogPopup);
    }

    @Override
    public Bitmap getIconForField(int fieldType, String input) {
        return nativeGetIconForField(mNativeDialogPopup, fieldType, input);
    }

    @Override
    public String getPlaceholderForField(int section, int fieldType) {
        return nativeGetPlaceholderForField(mNativeDialogPopup, section, fieldType);
    }

    @Override
    public String getLabelForSection(int section) {
        return nativeGetLabelForSection(mNativeDialogPopup, section);
    }

    // Helper methods for AutofillDialogField and AutofillDialogItem ------------------------------

    @CalledByNative
    private static AutofillDialogField[] createAutofillDialogFieldArray(int size) {
        return new AutofillDialogField[size];
    }

    /**
     * @param array AutofillDialogField array that should get a new field added.
     * @param index Index in the array where to place a new field.
     * @param nativePointer The pointer to the corresponding native field object.
     * @param fieldType The input field type. It should match one of the types in
     *                  {@link AutofillDialogConstants}.
     * @param placeholder The placeholder text for the input field. It can be an empty string.
     * @param value The value of the autofilled input field. It can be an empty string.
     */
    @CalledByNative
    private static void addToAutofillDialogFieldArray(AutofillDialogField[] array, int index,
            int nativePointer, int fieldType, String placeholder, String value) {
        array[index] = new AutofillDialogField(nativePointer, fieldType, placeholder, value);
    }

    @CalledByNative
    private static int getFieldNativePointer(AutofillDialogField field) {
        return field.mNativePointer;
    }

    @CalledByNative
    private static String getFieldValue(AutofillDialogField field) {
        return field.getValue();
    }

    @CalledByNative
    private static AutofillDialogMenuItem[] createAutofillDialogMenuItemArray(int size) {
        return new AutofillDialogMenuItem[size];
    }

    @CalledByNative
    private static void addToAutofillDialogMenuItemArray(AutofillDialogMenuItem[] array, int index,
            String line1, String line2, Bitmap icon) {
        array[index] = new AutofillDialogMenuItem(index, line1, line2, icon);
    }

    @CalledByNative
    private static AutofillDialogNotification[] createAutofillDialogNotificationArray(int size) {
        return new AutofillDialogNotification[size];
    }

    @CalledByNative
    private static void addToAutofillDialogNotificationArray(AutofillDialogNotification[] array,
            int index, int backgroundColor, int textColor, boolean hasArrow, boolean hasCheckbox,
            String text) {
        array[index] = new AutofillDialogNotification(backgroundColor, textColor, hasArrow,
                hasCheckbox, text);
    }

    // Calls from Java to C++ AutofillDialogViewAndroid --------------------------------------------

    private native void nativeItemSelected(int nativeAutofillDialogViewAndroid, int section,
            int index);
    private native void nativeAccountSelected(int nativeAutofillDialogViewAndroid, int index);
    private native void nativeContinueAutomaticSignin(
            int nativeAutofillDialogViewAndroid,
            String accountName, String sid, String lsid);
    private native void nativeEditingStart(int nativeAutofillDialogViewAndroid, int section);
    private native void nativeEditingComplete(int nativeAutofillDialogViewAndroid, int section);
    private native void nativeEditingCancel(int nativeAutofillDialogViewAndroid, int section);
    private native void nativeDialogSubmit(int nativeAutofillDialogViewAndroid);
    private native void nativeDialogCancel(int nativeAutofillDialogViewAndroid);
    private native String nativeGetLabelForSection(int nativeAutofillDialogViewAndroid,
            int section);
    private native Bitmap nativeGetIconForField(int nativeAutofillDialogViewAndroid,
            int fieldType, String input);
    private native String nativeGetPlaceholderForField(int nativeAutofillDialogViewAndroid,
            int section, int fieldType);
}
