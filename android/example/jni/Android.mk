# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

TARGET_ARCH_ABI := armeabi-v7a

include $(CLEAR_VARS)

LOCAL_MODULE := liballegro-prebuilt
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/liballegro-debug.so
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)

LOCAL_MODULE := liballegro_primitives-prebuilt
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/liballegro_primitives-debug.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := liballegro_image-prebuilt
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/liballegro_image-debug.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_MODULE    := allegro-example
LOCAL_SRC_FILES := main.c
LOCAL_CFLAGS    := -I$(ANDROID_NDK_TOOLCHAIN_ROOT)/user/$(TARGET_ARCH_ABI)/include
LOCAL_CFLAGS    += -DDEBUGMODE
LOCAL_CFLAGS    += -W -Wall

LOCAL_LDLIBS    := -L$(ANDROID_NDK_TOOLCHAIN_ROOT)/user/$(TARGET_ARCH_ABI)/lib
LOCAL_LDLIBS    += -L$(LOCAL_PATH)/$(TARGET_ARCH_ABI)
LOCAL_LDLIBS    += -llog
LOCAL_LDLIBS    += libs/$(TARGET_ARCH_ABI)/liballegro-debug.so
LOCAL_LDLIBS    += libs/$(TARGET_ARCH_ABI)/liballegro_primitives-debug.so
LOCAL_LDLIBS    += libs/$(TARGET_ARCH_ABI)/liballegro_image-debug.so

include $(BUILD_SHARED_LIBRARY)

