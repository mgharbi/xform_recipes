LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ARCH := $(ANDROID_ABI)

NDK_APP_DST_DIR=/$(ANDROID_LIB_DIR)/${ARCH}
LOCAL_CFLAGS += -std=c++11 -O3 -DNDEBUG -march=armv7-a 
# LOCAL_LDFLAGS += -O3
NDK_DEBUG=0

LOCAL_MODULE    := $(ANDROID_MODULE)
LOCAL_SRC_FILES := $(ANDROID_SRC)

$(info $(NDK_APP_OUT))

LOCAL_ARM_MODE  := arm

LOCAL_LDLIBS    := -Ljni -ljnigraphics -lm -llog -landroid -lz
LOCAL_LDLIBS    += ${ANDROID_LD_LIBS}
LOCAL_STATIC_LIBRARIES := android_native_app_glue

LOCAL_C_INCLUDES := $(ANDROID_C_INCLUDES) $(HALIDE_INCLUDES)

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
