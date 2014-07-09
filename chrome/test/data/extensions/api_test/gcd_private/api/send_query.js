// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

onload = function() {
  chrome.test.runTests([
    function receiveNewDevice() {
      chrome.gcdPrivate.onCloudDeviceStateChanged.addListener(
        function(available, device) {});

      chrome.gcdPrivate.queryForNewLocalDevices();
      chrome.test.notifyPass();
  }]);
};
