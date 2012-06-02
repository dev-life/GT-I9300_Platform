LOCAL_PATH:= $(call my-dir)

#
# libbluetoothd
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	android_bluez.c \
	adapter.c \
	agent.c \
	dbus-common.c \
	device.c \
	eir.c \
	error.c \
	event.c \
	glib-helper.c \
	log.c \
	main.c \
	manager.c \
	oob.c \
	oui.c \
	plugin.c \
	rfkill.c \
	sdpd-request.c \
	sdpd-service.c \
	sdpd-server.c \
	sdpd-database.c \
	sdp-xml.c \
	storage.c \
	textfile.c \
	attrib-server.c \
	../attrib/att.c \
	../attrib/client.c \
	../attrib/gatt.c \
	../attrib/gattrib.c \
	../attrib/utils.c \

# SS_BLUETOOTH(is80.hwang) 2012.02.10
ifeq ($(BOARD_HAVE_BLUETOOTH_CSR),true)
LOCAL_SRC_FILES += \
	../tools/csr_hci.c
endif
# SS_BLUETOOTH(is80.hwang) End
LOCAL_CFLAGS:= \
	-DVERSION=\"4.93\" \
	-DSTORAGEDIR=\"/data/misc/bluetoothd\" \
	-DCONFIGDIR=\"/etc/bluetooth\" \
	-DSERVICEDIR=\"/system/bin\" \
	-DPLUGINDIR=\"/system/lib/bluez-plugin\" \
	-DANDROID_SET_AID_AND_CAP \
	-DOUIFILE=\"/data/misc/bluetoothd/ouifile\" \
	-DNEED_G_SLIST_FREE_FULL \
	-DNEED_G_LIST_FREE_FULL \
#	-DANDROID_EXPAND_NAME \

ifeq ($(BOARD_HAVE_BLUETOOTH_BCM),true)
LOCAL_CFLAGS += \
	-DBOARD_HAVE_BLUETOOTH_BCM
endif

ifeq ($(SEC_PRODUCT_SHIP),true)
LOCAL_CFLAGS += \
	-DSEC_PRODUCT_SHIP
endif

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../attrib \
	$(LOCAL_PATH)/../btio \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../gdbus \
	$(LOCAL_PATH)/../plugins \
	$(call include-path-for, glib) \
	$(call include-path-for, glib)/glib \
	$(call include-path-for, dbus)

LOCAL_SHARED_LIBRARIES := \
	libdl \
	libbluetooth \
	libbtio \
	libdbus \
	libcutils \
	libglib \

LOCAL_STATIC_LIBRARIES := \
	libbuiltinplugin \
	libgdbus_static

LOCAL_MODULE:=libbluetoothd

include $(BUILD_SHARED_LIBRARY)

#
# bluetoothd
#

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	libbluetoothd

LOCAL_MODULE:=bluetoothd

include $(BUILD_EXECUTABLE)
