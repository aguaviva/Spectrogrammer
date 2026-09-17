#Copyright (c) 2019-2020 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.
# NO WARRANTY! NO GUARANTEE OF SUPPORT! USE AT YOUR OWN RISK

BUILD_ANDROID?=n
DEBUG?=n
CFLAGS?=-ffunction-sections -fdata-sections -Wall -fvisibility=hidden -fno-exceptions -fno-rtti -fno-sized-deallocation
LDFLAGS?=-Wl,--gc-sections -Wl,-Map=$(ARCH).map

ifeq ($(BUILD_ANDROID),y)
ARCH?=arm64-v8a
ANDROID_ABIS?=$(ARCH)
else
ARCH?=$(shell uname -m)
endif

# WARNING WARNING WARNING!  YOU ABSOLUTELY MUST OVERRIDE THE PROJECT NAME
# you should also override these parameters, get your own signatre file and make your own manifest.
APPNAME?=Spectrogrammer
SRC_DIR?=./src
BINFILE:=$(APPNAME)-$(ARCH)
ifeq ($(BUILD_ANDROID),y)
APKFILE?=$(APPNAME)-$(ARCH).apk
endif

ifndef INNER
ifeq ($(BUILD_ANDROID),y)
all: $(foreach abi,$(ANDROID_ABIS),$(APPNAME)-$(abi).apk)
else
all: linux_version
endif
endif
.PHONY: all clean push run linux_version test testsdk

ifeq ($(BUILD_ANDROID),y)
LABEL?=$(APPNAME)
PACKAGENAME?=org.nanoorg.$(APPNAME)
SRC=$(SRC_DIR)/main_android.cpp
SRC+=$(SRC_DIR)/audio/audio_main.cpp $(SRC_DIR)/audio/audio_recorder.cpp
GLUE_SRC=$(SRC_DIR)/android_native_app_glue.c
else
SRC=$(SRC_DIR)/main_linux.cpp
CFLAGS+= -DALSA_DRIVER
SRC+=$(SRC_DIR)/audio/audio_driver_alsa.cpp
GLUE_SRC=
endif

# Add app source files
SRC+=$(wildcard $(SRC_DIR)/app/*.cpp)

# For really tight compiles....
CFLAGS += -fvisibility=hidden
CFLAGS += -std=c++17

ifeq ($(DEBUG),y)
LDFLAGS += -g
DEBUGGABLE?=true
else
LDFLAGS += -s -Os
DEBUGGABLE?=false
endif

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
OS_NAME = linux-x86_64
endif
ifeq ($(UNAME), Darwin)
OS_NAME = darwin-x86_64
endif
ifeq ($(OS), Windows_NT)
OS_NAME = windows-x86_64
endif

#if you have a custom Android Home location you can add it to this list.  
#This makefile will select the first present folder.
ifeq ($(BUILD_ANDROID),y)

#We've tested it with android version 22, 24, 28, 29 and 30 and 32.
#You can target something like Android 28, but if you set ANDROIDVERSION to say 22, then
#Your app should (though not necessarily) support all the way back to Android 22. 
ANDROIDVERSION?=30
ANDROIDTARGET?=$(ANDROIDVERSION)
ANDROID_FULLSCREEN?=y
ADB?=adb

# Search list for where to try to find the SDK
SDK_LOCATIONS += $(ANDROID_HOME) $(ANDROID_SDK_ROOT) $(HOME)/Android/Sdk $(HOME)/Library/Android/sdk

#Just a little Makefile witchcraft to find the first SDK_LOCATION that exists
#Then find an ndk folder and build tools folder in there.
ANDROIDSDK?=$(firstword $(foreach dir, $(SDK_LOCATIONS), $(basename $(dir) ) ) )
NDK?=$(firstword $(ANDROID_NDK) $(ANDROID_NDK_HOME) $(lastword $(wildcard $(ANDROIDSDK)/ndk/*)) $(wildcard $(ANDROIDSDK)/ndk-bundle/*) )
BUILD_TOOLS?=$(lastword $(wildcard $(ANDROIDSDK)/build-tools/*) )

# fall back to default Android SDL installation location if valid NDK was not found
ifeq ($(NDK),)
ANDROIDSDK := ~/Android/Sdk
endif

# Verify if directories are detected
ifeq ($(ANDROIDSDK),)
$(error ANDROIDSDK directory not found)
endif
ifeq ($(NDK),)
$(error NDK directory not found)
endif
ifeq ($(BUILD_TOOLS),)
$(error BUILD_TOOLS directory not found)
endif

ANDROID_JAR:=$(wildcard $(ANDROIDSDK)/platforms/android-$(ANDROIDVERSION)/android.jar)
ifeq ($(ANDROID_JAR),)
ANDROID_JAR:=$(lastword $(sort $(wildcard $(ANDROIDSDK)/platforms/android-*/android.jar)))
endif
ifeq ($(ANDROID_JAR),)
$(error android.jar not found under $(ANDROIDSDK)/platforms)
endif


CFLAGS+=-Os -DANDROID -DAPPNAME=\"$(APPNAME)\"
ifeq ($(ANDROID_FULLSCREEN),y)
CFLAGS +=-DANDROID_FULLSCREEN 
endif

CFLAGS+= -I$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/include -fPIC -DANDROIDVERSION=$(ANDROIDVERSION)
endif

CFLAGS+= -I$(SRC_DIR) -I$(SRC_DIR)/app -I$(SRC_DIR)/audio 
CFLAGS+= -Isubmodules/kissfft -Isubmodules/imgui

ifeq ($(BUILD_ANDROID),y)
LDFLAGS += -landroid -lGLESv3 -lEGL  -llog -lOpenSLES 
LDFLAGS += -shared -uANativeActivity_onCreate -static-libstdc++
AAPT:=$(BUILD_TOOLS)/aapt
endif

########################### ARCHITECTURE ######################################

ifeq ($(BUILD_ANDROID),y)
AR:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/llvm-ar
endif

ifeq ($(BUILD_ANDROID),y)
ARCH_DIR:=$(ARCH)
LD:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/ld.lld
ifeq ($(ARCH),armeabi-v7a)
ANDROID_TRIPLE:=armv7a-linux-androideabi
else ifeq ($(ARCH),x86)
ANDROID_TRIPLE:=i686-linux-android
else ifeq ($(ARCH),x86_64)
ANDROID_TRIPLE:=x86_64-linux-android
else
ANDROID_TRIPLE:=aarch64-linux-android
endif
CC:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/$(ANDROID_TRIPLE)$(ANDROIDVERSION)-clang
CXX:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/$(ANDROID_TRIPLE)$(ANDROIDVERSION)-clang++
LDFLAGS += -fuse-ld=$(LD)
endif

testsdk :
	@echo "ARCH:\t\t" $(ARCH)
	@echo "ANDROID_ABIS:\t" $(ANDROID_ABIS)
	@echo "BINFILE:\t" $(BINFILE)
	@echo "APKFILE:\t" $(APKFILE)
	@echo "CXX:\t\t" $(CXX)
	@echo "LD:\t\t" $(LD)
	@echo "SDK:\t\t" $(ANDROIDSDK)
	@echo "NDK:\t\t" $(NDK)
	@echo "Build Tools:\t" $(BUILD_TOOLS)
	@echo "android.jar:\t" $(ANDROID_JAR)


ifneq ($(BUILD_ANDROID),y)
LDFLAGS += $(shell pkg-config --libs glfw3) -lGL -lasound -lpthread
ARCH_DIR=$(ARCH)
CFLAGS += $(shell pkg-config --cflags glfw3)
CFLAGS += -Wall -Wformat
endif

LDFLAGS += -lm  
LDFLAGS += -Wl,--no-undefined

STOREPASS?=password
DNAME:="CN=example.com, OU=ID, O=Example, L=Doe, S=John, C=GB"
KEYSTOREFILE:=my-release-key.keystore
ALIASNAME?=standkey

keystore : $(KEYSTOREFILE)

$(KEYSTOREFILE) :
	keytool -genkey -v -keystore $(KEYSTOREFILE) -alias $(ALIASNAME) -keyalg RSA -keysize 2048 -validity 10000 -storepass $(STOREPASS) -keypass $(STOREPASS) -dname $(DNAME)

folders:
	mkdir -p makecapk/lib/arm64-v8a
	mkdir -p makecapk/lib/armeabi-v7a
	mkdir -p makecapk/lib/x86
	mkdir -p makecapk/lib/x86_64

################## KISSFFT / IMGUI

KISSFFT_LIB := libs/$(ARCH)/kissfft/libkissfft.a
IMGUI_LIB := libs/$(ARCH)/imgui/libimgui.a

KISSFFT_SRCS := kiss_fft.c kiss_fftr.c
KISSFFT_CFLAGS := $(filter-out -std=c++17 -fno-exceptions -fno-rtti -fno-sized-deallocation,$(CFLAGS))

ifeq ($(BUILD_ANDROID),y)
GLUE_OBJ := libs/$(ARCH)/android_native_app_glue.o
libs/$(ARCH)/android_native_app_glue.o : $(GLUE_SRC)
	mkdir -p libs/$(ARCH)
	$(CC) -c $(KISSFFT_CFLAGS) -o $@ $<
else
GLUE_OBJ :=
endif

libs/$(ARCH)/kissfft/objs/%.o : submodules/kissfft/%.c
	mkdir -p libs/$(ARCH)/kissfft/objs
	$(CC) -c $(KISSFFT_CFLAGS) $(CFLAGS_2) -o $@ $<

$(KISSFFT_LIB) : $(addprefix libs/$(ARCH)/kissfft/objs/,$(subst .c,.o,$(KISSFFT_SRCS)))
	$(AR) rcs $@ $^

IMGUI_SRCS := imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp backends/imgui_impl_opengl3.cpp
ifeq ($(BUILD_ANDROID),y)
IMGUI_SRCS += backends/imgui_impl_android.cpp 
else
IMGUI_SRCS += backends/imgui_impl_glfw.cpp
endif

libs/$(ARCH)/imgui/objs/%.o : submodules/imgui/%.cpp
	mkdir -p libs/$(ARCH)/imgui/objs
	mkdir -p libs/$(ARCH)/imgui/objs/backends
	$(CXX) -c $(CFLAGS) -Isubmodules/imgui $(CFLAGS_2) $< -o $@

$(IMGUI_LIB) : $(addprefix libs/$(ARCH)/imgui/objs/,$(subst .cpp,.o,$(IMGUI_SRCS)))
	$(AR) rcs $@ $^

###############

ifndef INNER
ifeq ($(BUILD_ANDROID),y)
define BUILD_ABI_APK
$(APPNAME)-$(1).apk:
	$$(MAKE) BUILD_ANDROID=y INNER=1 ARCH=$(1) $$@
endef
$(foreach abi,$(ANDROID_ABIS),$(eval $(call BUILD_ABI_APK,$(abi))))
endif
else
STAGE := makecapk-$(ARCH)
makecapk-$(ARCH)/lib/$(ARCH_DIR)/lib$(APPNAME).so : $(SRC) $(GLUE_OBJ) $(IMGUI_LIB) $(KISSFFT_LIB)
	mkdir -p makecapk-$(ARCH)/lib/$(ARCH_DIR)
	$(CXX) $(CFLAGS) $(CFLAGS_2) -o $@ $(SRC) $(GLUE_OBJ) $(IMGUI_LIB) $(KISSFFT_LIB) $(LDFLAGS)

$(APKFILE) : makecapk-$(ARCH)/lib/$(ARCH)/lib$(APPNAME).so $(SRC_DIR)/AndroidManifest.xml
	mkdir -p $(STAGE)/assets $(STAGE)/res
	cp -a $(SRC_DIR)/res/. $(STAGE)/res/
	rm -f temp-$(ARCH).apk $(ARCH)-unsigned.apk
	$(AAPT) package -f -F temp-$(ARCH).apk -I $(ANDROID_JAR) -M $(SRC_DIR)/AndroidManifest.xml -S $(STAGE)/res -A $(STAGE)/assets -v --target-sdk-version $(ANDROIDTARGET)
	unzip -o temp-$(ARCH).apk -d $(STAGE)
	cd $(STAGE) && zip -D4r ../$(ARCH)-unsigned.apk AndroidManifest.xml resources.arsc res assets lib/$(ARCH)
	cd $(STAGE) && zip -D0r ../$(ARCH)-unsigned.apk ./resources.arsc ./AndroidManifest.xml
	rm -f $(APKFILE)
	$(BUILD_TOOLS)/zipalign -v 4 $(ARCH)-unsigned.apk $(APKFILE)
	$(BUILD_TOOLS)/apksigner sign --key-pass pass:$(STOREPASS) --ks-pass pass:$(STOREPASS) --ks $(KEYSTOREFILE) $(APKFILE)
	rm -f temp-$(ARCH).apk $(ARCH)-unsigned.apk
	@ls -l $(APKFILE)
endif

#We're really cutting corners.  You should probably use resource files.. Replace android:label="@string/app_name" and add a resource file.
#Then do this -S Sources/res on the aapt line.
#For icon support, add -S makecapk/res to the aapt line.  also,  android:icon="@mipmap/icon" to your application line in the manifest.
#If you want to strip out about 800 bytes of data you can remove the icon and strings.

#Notes for the past:  These lines used to work, but don't seem to anymore.  Switched to newer jarsigner.
#(zipalign -c -v 8 makecapk.apk)||true #This seems to not work well.
#jarsigner -verify -verbose -certs makecapk.apk


linux_version : $(SRC) $(IMGUI_LIB) $(KISSFFT_LIB)
	$(CXX) $(CFLAGS) $(CFLAGS_2) -o $(BINFILE) $^ $(LDFLAGS)

TEST_SRCS := tests/test_all.cpp \
	$(SRC_DIR)/app/auformat.cpp \
	$(SRC_DIR)/app/ChunkerProcessor.cpp \
	$(SRC_DIR)/app/fft.cpp \
	$(SRC_DIR)/app/scale.cpp \
	$(SRC_DIR)/app/ScaleBufferY.cpp \
	$(SRC_DIR)/app/SpectrumFile.cpp \
	$(SRC_DIR)/audio/audio_driver_alsa.cpp

test_runner : $(TEST_SRCS) $(KISSFFT_LIB)
	$(CXX) -g -O0 -std=c++17 -DALSA_DRIVER \
		-I$(SRC_DIR) -I$(SRC_DIR)/app -I$(SRC_DIR)/audio -Isubmodules/kissfft \
		-o $@ $(TEST_SRCS) $(KISSFFT_LIB) -lasound -lpthread -lm

test : test_runner
	./test_runner

manifest: $(SRC_DIR)/AndroidManifest.xml

$(SRC_DIR)/AndroidManifest.xml : $(SRC_DIR)/AndroidManifest.xml.template
	rm -rf $(SRC_DIR)/AndroidManifest.xml
	PACKAGENAME=$(PACKAGENAME) \
		ANDROIDVERSION=$(ANDROIDVERSION) \
		ANDROIDTARGET=$(ANDROIDTARGET) \
		APPNAME=$(APPNAME) \
		LABEL=$(LABEL) \
		DEBUGGABLE=$(DEBUGGABLE) envsubst '$$ANDROIDTARGET $$ANDROIDVERSION $$APPNAME $$PACKAGENAME $$LABEL $$DEBUGGABLE' \
		< $(SRC_DIR)/AndroidManifest.xml.template > $(SRC_DIR)/AndroidManifest.xml

uninstall : 
	($(ADB) uninstall $(PACKAGENAME))||true

push : $(APKFILE)
	@echo "Installing" $(PACKAGENAME)
	$(ADB) install -r $(APKFILE)

logcat: push
	$(ADB) logcat | $(NDK)/ndk-stack -sym makecapk-$(ARCH)/lib/$(ARCH)/lib$(APPNAME).so

run : push
	$(eval ACTIVITYNAME:=$(shell $(AAPT) dump badging $(APKFILE) | grep "launchable-activity" | cut -f 2 -d"'"))
	$(ADB) shell am start -n $(PACKAGENAME)/$(ACTIVITYNAME)

clean :
	rm -rf temp.apk temp-*.apk makecapk.apk $(APPNAME).apk
	rm -f $(APPNAME) $(APPNAME)-* $(APPNAME)-*.apk $(APPNAME)-*.apk.idsig *-unsigned.apk *.map
	rm -rf makecapk makecapk-arm64-v8a makecapk-armeabi-v7a makecapk-x86 makecapk-x86_64
	rm -rf libs
	rm -rf linux_version test_runner
