LOCAL_PATH := $(call my-dir)

############################### imgui

include $(CLEAR_VARS)

LOCAL_MODULE    := imgui
FILE_LIST := imgui/imgui.cpp
FILE_LIST += imgui/imgui_draw.cpp
FILE_LIST += imgui/imgui_tables.cpp
FILE_LIST += imgui/imgui_widgets.cpp
FILE_LIST += imgui/backends/imgui_impl_android.cpp
FILE_LIST += imgui/backends/imgui_impl_opengl3.cpp

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/imgui
LOCAL_EXPORT_CFLAGS := -I$(LOCAL_PATH)/imgui

LOCAL_CFLAGS    := -Oz -fvisibility=hidden -ffunction-sections -fdata-sections -fno-stack-protector -fomit-frame-pointer -flto

include $(BUILD_STATIC_LIBRARY)

############################### audio

include $(CLEAR_VARS)

LOCAL_MODULE    := audio
FILE_LIST := $(wildcard $(LOCAL_PATH)/audio/*.cpp)
LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_EXPORT_CFLAGS := -I$(LOCAL_PATH)/audio
LOCAL_CFLAGS    := -Oz -fvisibility=hidden -ffunction-sections -fdata-sections -fno-stack-protector -fomit-frame-pointer -flto

include $(BUILD_STATIC_LIBRARY)

############################### fft

include $(CLEAR_VARS)

LOCAL_MODULE := kissfft
#LOCAL_SRC_FILES := fftw/$(TARGET_ARCH_ABI)/lib/libfftw3f.a
#LOCAL_EXPORT_CFLAGS := -I$(LOCAL_PATH)/fftw/$(TARGET_ARCH_ABI)/include
LOCAL_SRC_FILES := kissfft/$(TARGET_ARCH_ABI)/lib64/libkissfft-float.a
LOCAL_EXPORT_CFLAGS := -I$(LOCAL_PATH)/kissfft/$(TARGET_ARCH_ABI)/include/kissfft
include $(PREBUILT_STATIC_LIBRARY)

############################### spectrogrammer

include $(CLEAR_VARS)

LOCAL_MODULE    := spectrogrammer
LOCAL_LDLIBS    := -llog -landroid -lGLESv3 -lEGL -lOpenSLES -lm -u ANativeActivity_onCreate

FILE_LIST := $(wildcard $(LOCAL_PATH)/app/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/app/*.cpp)

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_STATIC_LIBRARIES := imgui audio kissfft

LOCAL_CFLAGS    := -Oz -fvisibility=hidden -ffunction-sections -fdata-sections -fno-stack-protector -fomit-frame-pointer -flto
LOCAL_LDFLAGS   := -Wl,--gc-sections -s

include $(BUILD_SHARED_LIBRARY)
