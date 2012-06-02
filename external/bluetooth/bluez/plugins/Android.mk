LOCAL_PATH:= $(call my-dir)

#
# libplugin
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        adaptername.c \
	mgmtops.c \
	dbusoob.c \

LOCAL_CFLAGS:= \
	-DVERSION=\"4.93\" \
	-DBLUETOOTH_PLUGIN_BUILTIN \
	-DSTORAGEDIR=\"/data/misc/bluetoothd\" \
	-DANDROID_EXPAND_NAME \

ifeq ($(BOARD_HAVE_BLUETOOTH_BCM),true)
LOCAL_CFLAGS += -DBOARD_HAVE_BLUETOOTH_BCM
endif

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../btio \
	$(LOCAL_PATH)/../lib \
        $(LOCAL_PATH)/../gdbus \
        $(LOCAL_PATH)/../src \
        $(call include-path-for, glib) \
        $(call include-path-for, dbus) \

LOCAL_SHARED_LIBRARIES := \
	libbluetoothd \
	libbluetooth \
	libcutils \
	libdbus \
	libglib \

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/bluez-plugin
LOCAL_UNSTRIPPED_PATH := $(TARGET_OUT_SHARED_LIBRARIES_UNSTRIPPED)/bluez-plugin
LOCAL_MODULE:=libbuiltinplugin

include $(BUILD_STATIC_LIBRARY)
