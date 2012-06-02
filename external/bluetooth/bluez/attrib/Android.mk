LOCAL_PATH:= $(call my-dir)
#
# gatttool
#

include $(CLEAR_VARS)

LOCAL_CFLAGS:= \
	-DVERSION=\"4.93\" \
       
LOCAL_SRC_FILES:= \
	gatttool.c \
       interactive.c \
       utils.c

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../src \
	$(LOCAL_PATH)/../btio \
	$(call include-path-for, glib)

LOCAL_SHARED_LIBRARIES := \
	libbluetoothd \
	libbluetooth \
	libbtio \
	libglib \

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE:=gatttool

include $(BUILD_EXECUTABLE)
