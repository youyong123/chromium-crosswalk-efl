# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This Makefile is used by debhelper, which supplies the appropriate value of
# variables not defined here, such as DESTDIR.

SRC_DIR = ../../../..
BUILD_DIR = $(SRC_DIR)/out/Release

INSTALL_DIR = $(DESTDIR)/opt/google/chrome-remote-desktop
CRON_DIR = $(DESTDIR)/etc/cron.daily
NATIVE_MESSAGING_DIR = $(DESTDIR)/etc/opt/chrome/native-messaging-hosts

ME2ME_PROGNAME = $(BUILD_DIR)/remoting_me2me_host
ME2ME_DEBUGFILE = $(ME2ME_PROGNAME).debug
START_PROGNAME = $(BUILD_DIR)/remoting_start_host
START_DEBUGFILE = $(START_PROGNAME).debug
ME2ME_NM_PROGNAME = $(BUILD_DIR)/native_messaging_host
ME2ME_NM_DEBUGFILE = $(ME2ME_NM_PROGNAME).debug
REMOTE_ASSISTANCE_PROGNAME = $(BUILD_DIR)/remote_assistance_host
REMOTE_ASSISTANCE_DEBUGFILE = $(REMOTE_ASSISTANCE_PROGNAME).debug

all:

install:
	install -d "$(INSTALL_DIR)"
	install -d "$(INSTALL_DIR)/remoting_locales"
	install -d "$(NATIVE_MESSAGING_DIR)"
	install -d "$(CRON_DIR)"

	install "$(SRC_DIR)/remoting/host/linux/linux_me2me_host.py" \
	  "$(INSTALL_DIR)/chrome-remote-desktop"
	install "$(SRC_DIR)/remoting/host/installer/linux/is-remoting-session" \
	  "$(INSTALL_DIR)"

	install -m 0644 \
	    "$(BUILD_DIR)/remoting/com.google.chrome.remote_desktop.json" \
	   "$(NATIVE_MESSAGING_DIR)/com.google.chrome.remote_desktop.json"

	install -m 0644 \
	  "$(BUILD_DIR)/remoting/com.google.chrome.remote_assistance.json" \
	  "$(NATIVE_MESSAGING_DIR)/com.google.chrome.remote_assistance.json"

	eu-strip -f "$(ME2ME_DEBUGFILE)" "$(ME2ME_PROGNAME)"
	install "$(ME2ME_PROGNAME)" \
	  "$(INSTALL_DIR)/chrome-remote-desktop-host"

	eu-strip -f "$(START_DEBUGFILE)" "$(START_PROGNAME)"
	install "$(START_PROGNAME)" "$(INSTALL_DIR)/start-host"

	eu-strip -f "$(ME2ME_NM_DEBUGFILE)" "$(ME2ME_NM_PROGNAME)"
	install "$(ME2ME_NM_PROGNAME)" "$(INSTALL_DIR)/native-messaging-host"

	eu-strip -f "$(REMOTE_ASSISTANCE_DEBUGFILE)" \
	  "$(REMOTE_ASSISTANCE_PROGNAME)"
	install "$(REMOTE_ASSISTANCE_PROGNAME)" \
	  "$(INSTALL_DIR)/remote-assistance-host"

	install -m 0644 \
          "$(BUILD_DIR)/icudtl.dat" "$(INSTALL_DIR)/icudtl.dat"

	for locale in $$(ls $(BUILD_DIR)/remoting_locales); do \
	  install "$(BUILD_DIR)/remoting_locales/$$locale" \
	    "$(INSTALL_DIR)/remoting_locales/$$locale"; \
	done

	install $(BUILD_DIR)/remoting/installer/cron/chrome-remote-desktop \
	  "$(CRON_DIR)/chrome-remote-desktop"
