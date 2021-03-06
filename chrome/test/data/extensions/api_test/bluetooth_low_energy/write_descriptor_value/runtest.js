// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testWriteDescriptorValue() {
  chrome.test.assertTrue(descriptor != null, '\'descriptor\' is null');
  chrome.test.assertEq(descId, descriptor.instanceId);

  chrome.test.assertEq(writeValue.byteLength, descriptor.value.byteLength);

  var receivedValueBytes = new Uint8Array(descriptor.value);
  for (var i = 0; i < writeValue.byteLength; i++) {
    chrome.test.assertEq(valueBytes[i], receivedValueBytes[i]);
  }

  chrome.test.succeed();
}

var writeDescriptorValue = chrome.bluetoothLowEnergy.writeDescriptorValue;
var descId = 'desc_id0';
var badDescId = 'desc_id1';

var descriptor = null;

var bytes = [0x43, 0x68, 0x72, 0x6F, 0x6D, 0x65];
var writeValue = new ArrayBuffer(bytes.length);
var valueBytes = new Uint8Array(writeValue);
valueBytes.set(bytes);

// 1. Unknown descriptor instanceId.
writeDescriptorValue(badDescId, writeValue, function (result) {
  if (result || !chrome.runtime.lastError) {
    chrome.test.fail('\'badDescId\' did not cause failure');
  }

  // 2. Known descriptor instanceId, but call failure.
  writeDescriptorValue(descId, writeValue, function (result) {
    if (result || !chrome.runtime.lastError) {
      chrome.test.fail('writeDescriptorValue should have failed');
    }

    // 3. Call should succeed.
    writeDescriptorValue(descId, writeValue, function (result) {
      if (chrome.runtime.lastError) {
        chrome.test.fail(chrome.runtime.lastError.message);
      }

      chrome.bluetoothLowEnergy.getDescriptor(descId, function (result) {
        descriptor = result;

        chrome.test.sendMessage('ready', function (message) {
          chrome.test.runTests([testWriteDescriptorValue]);
        });
      });
    });
  });
});
