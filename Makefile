#Copyright (c) 2019-2020 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.
# NO WARRANTY! NO GUARANTEE OF SUPPORT! USE AT YOUR OWN RISK
BUILD_ANDROID:=y
DEBUG:=n

all : makecapk.apk
#all: linux_version
.PHONY : push run

# WARNING WARNING WARNING!  YOU ABSOLUTELY MUST OVERRIDE THE PROJECT NAME
# you should also override these parameters, get your own signatre file and make your own manifest.
APPNAME?=Spectrogrammer
SRC_DIR?=./src

ifeq ($(BUILD_ANDROID),y)
LABEL?=$(APPNAME)
APKFILE?=$(APPNAME).apk
PACKAGENAME?=org.nanoorg.$(APPNAME)
SRC=$(SRC_DIR)/main_android.c $(SRC_DIR)/android_native_app_glue.c
SRC+=$(shell find $(SRC_DIR)/audio -name '*.cpp')
else
SRC=$(SRC)/main_linux.c
endif

# Add app source files
SRC+=$(shell find $(SRC_DIR)/app -name '*.cpp')

CFLAGS?=-ffunction-sections  -fdata-sections -Wall -fvisibility=hidden -fno-exceptions -fno-rtti -fno-sized-deallocation
# For really tight compiles....
CFLAGS += -fvisibility=hidden


LDFLAGS?=-Wl,--gc-sections -Wl,-Map=output.map 
ifeq ($(DEBUG),y)
LDFLAGS += -g  
else
LDFLAGS += -s -Os
endif

#if you have a custom Android Home location you can add it to this list.  
#This makefile will select the first present folder.
ifeq ($(BUILD_ANDROID),y)

#We've tested it with android version 22, 24, 28, 29 and 30 and 32.
#You can target something like Android 28, but if you set ANDROIDVERSION to say 22, then
#Your app should (though not necessarily) support all the way back to Android 22. 
ANDROIDVERSION?=29
ANDROIDTARGET?=$(ANDROIDVERSION)
ANDROID_FULLSCREEN?=y
ANDROIDSRCS:= $(SRC) 
ADB?=adb

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

# For C++
# LDFLAGS += -static-libstdc++
# $(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/lib/aarch64-linux-android/libc.a

# Search list for where to try to find the SDK
SDK_LOCATIONS += $(ANDROID_HOME) $(ANDROID_SDK_ROOT) ~/Android/Sdk $(HOME)/Library/Android/sdk

#Just a little Makefile witchcraft to find the first SDK_LOCATION that exists
#Then find an ndk folder and build tools folder in there.
ANDROIDSDK?=$(firstword $(foreach dir, $(SDK_LOCATIONS), $(basename $(dir) ) ) )
NDK?=$(firstword $(ANDROID_NDK) $(ANDROID_NDK_HOME) $(wildcard $(ANDROIDSDK)/ndk/*) $(wildcard $(ANDROIDSDK)/ndk-bundle/*) )
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

testsdk :
	@echo "SDK:\t\t" $(ANDROIDSDK)
	@echo "NDK:\t\t" $(NDK)
	@echo "Build Tools:\t" $(BUILD_TOOLS)

CFLAGS+=-Os -DANDROID -DAPPNAME=\"$(APPNAME)\"
ifeq (ANDROID_FULLSCREEN,y)
CFLAGS +=-DANDROID_FULLSCREEN 
endif

CFLAGS+= -I$(NDK)/sysroot/usr/include -I$(NDK)/sysroot/usr/include/android -I$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/include -I$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/include/android -fPIC -I$(RAWDRAWANDROID) -DANDROIDVERSION=$(ANDROIDVERSION) 
endif

CFLAGS+= -I$(SRC_DIR) -I$(SRC_DIR)/app -I$(SRC_DIR)/audio 
CFLAGS+= -Isubmodules/kissfft -Isubmodules/imgui -Isubmodules/rawdraw 

ifeq ($(BUILD_ANDROID),y)
LDFLAGS += -landroid -lGLESv3 -lEGL  -llog -lOpenSLES 
LDFLAGS += -shared -uANativeActivity_onCreate
LDFLAGS += -L$(NDK)/sources/cxx-stl/llvm-libc++/libs/arm64-v8a/ -lc++abi 
LDFLAGS += `pkg-config --libs submodules/kissfft/arm64-v8a/lib64/pkgconfig/kissfft-float.pc`
else
LDFLAGS += -lGL `pkg-config --static --libs glfw3` 
LDFLAGS += -lX11 -lpthread -lXinerama -lXext -lGL -lm -ldl -lstdc++
LDFLAGS += `pkg-config --libs submodules/kissfft/x86_64/lib64/pkgconfig/kissfft-float.pc`

CFLAGS += `pkg-config --cflags glfw3` 
CFLAGS += -std=c++11
CFLAGS += -Wall -Wformat
endif

LDFLAGS += -lm  
LDFLAGS += -Wl,--no-undefined

AR_ARM64:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/aarch64-linux-android-ar
AR_ARM32:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/armv7a-linux-android-ar

LD_ARM64:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/aarch64-linux-android-ld
LD_ARM64:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/armv7a-linux-android-ld

CC_ARM64:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/aarch64-linux-android$(ANDROIDVERSION)-clang
CC_ARM32:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/armv7a-linux-androideabi$(ANDROIDVERSION)-clang
CC_x86:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/i686-linux-android$(ANDROIDVERSION)-clang
CC_x86_64:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/x86_64-linux-android$(ANDROIDVERSION)-clang
AAPT:=$(BUILD_TOOLS)/aapt

# Which binaries to build? Just comment/uncomment these lines:
TARGETS += makecapk/lib/arm64-v8a/lib$(APPNAME).so
#TARGETS += makecapk/lib/armeabi-v7a/lib$(APPNAME).so
#TARGETS += makecapk/lib/x86/lib$(APPNAME).so
#TARGETS += makecapk/lib/x86_64/lib$(APPNAME).so

CFLAGS_ARM64:=-m64
CFLAGS_ARM32:=-mfloat-abi=softfp -m32
CFLAGS_x86:=-march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32
CFLAGS_x86_64:=-march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel
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


kissfft:
	make -C submodules/kissfft LD=$(LD_ARM64) CC=$(CC_ARM64) AR=$(AR_ARM64) PREFIX=arm64-v8a KISSFFT_DATATYPE=float KISSFFT_STATIC=1 KISSFFT_TOOLS=0 KISSFFT_OPENMP=0 CFLAGS=-DNDEBUG=1 install 

################## IMGUI

IMGUI_SRCS := imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp backends/imgui_impl_opengl3.cpp
ifeq ($(BUILD_ANDROID),y)
IMGUI_SRCS += backends/imgui_impl_android.cpp 
else
IMGUI_SRCS += backends/imgui_impl_glfw.cpp
endif

submodules/imgui/obj/%.o : submodules/imgui/%.cpp
	mkdir -p submodules/imgui/obj
	mkdir -p submodules/imgui/obj/backends
	#$(CC) -c $(CFLAGS) -Isubmodules/imgui $(CFLAGS_x86_64) $^ -o $@
	$(CC_ARM64) -c $(CFLAGS) -Isubmodules/imgui $(CFLAGS_ARM64) $^ -o $@  

submodules/imgui/lib/libimgui.a : $(addprefix submodules/imgui/obj/,$(subst .cpp,.o,$(IMGUI_SRCS)))
	mkdir -p submodules/imgui/lib
	ar ru $@ $^

###############

makecapk/lib/arm64-v8a/lib$(APPNAME).so : $(ANDROIDSRCS) submodules/imgui/lib/libimgui.a 
	mkdir -p makecapk/lib/arm64-v8a	
	$(CC_ARM64) $(CFLAGS) $(LDFLAGS) $(CFLAGS_ARM64) -o $@ $(ANDROIDSRCS) submodules/imgui/lib/libimgui.a -L$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/lib/aarch64-linux-android/$(ANDROIDVERSION) $(LDFLAGS)

makecapk/lib/armeabi-v7a/lib$(APPNAME).so : $(ANDROIDSRCS)
	mkdir -p makecapk/lib/armeabi-v7a
	$(CC_ARM32) $(CFLAGS) $(CFLAGS_ARM32) -o $@ $^ -L$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/lib/arm-linux-androideabi/$(ANDROIDVERSION) $(LDFLAGS)

makecapk/lib/x86/lib$(APPNAME).so : $(ANDROIDSRCS)
	mkdir -p makecapk/lib/x86
	$(CC_x86) $(CFLAGS) $(CFLAGS_x86) -o $@ $^ -L$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/lib/i686-linux-android/$(ANDROIDVERSION) $(LDFLAGS)

makecapk/lib/x86_64/lib$(APPNAME).so : $(ANDROIDSRCS)
	mkdir -p makecapk/lib/x86_64	
	$(CC_x86) $(CFLAGS) $(CFLAGS_x86_64) -o $@ $^ -L$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/lib/x86_64-linux-android/$(ANDROIDVERSION) $(LDFLAGS)

#We're really cutting corners.  You should probably use resource files.. Replace android:label="@string/app_name" and add a resource file.
#Then do this -S Sources/res on the aapt line.
#For icon support, add -S makecapk/res to the aapt line.  also,  android:icon="@mipmap/icon" to your application line in the manifest.
#If you want to strip out about 800 bytes of data you can remove the icon and strings.

#Notes for the past:  These lines used to work, but don't seem to anymore.  Switched to newer jarsigner.
#(zipalign -c -v 8 makecapk.apk)||true #This seems to not work well.
#jarsigner -verify -verbose -certs makecapk.apk


linux_version : $(SRC) imgui/lib/libimgui.a
	$(CC) $(CFLAGS) $(CFLAGS_x86_64) -o $@ $^  $(LDFLAGS)

makecapk.apk : $(TARGETS) $(EXTRA_ASSETS_TRIGGER) $(SRC_DIR)/AndroidManifest.xml 
	mkdir -p makecapk/assets
	#cp -r $(SRC_DIR)/assets/* makecapk/assets
	rm -rf temp.apk
	$(AAPT) package -f -F temp.apk -I $(ANDROIDSDK)/platforms/android-$(ANDROIDVERSION)/android.jar -M $(SRC_DIR)/AndroidManifest.xml -S $(SRC_DIR)/res -A makecapk/assets -v --target-sdk-version $(ANDROIDTARGET)
	unzip -o temp.apk -d makecapk
	rm -rf makecapk.apk
	# We use -4 here for the compression ratio, as it's a good balance of speed and size. -9 will make a slightly smaller executable but takes longer to build
	cd makecapk && zip -D4r ../makecapk.apk . && zip -D0r ../makecapk.apk ./resources.arsc $(SRC_DIR)/AndroidManifest.xml
	# jarsigner is only necessary when targetting Android < 7.0
	#jarsigner -sigalg SHA1withRSA -digestalg SHA1 -verbose -keystore $(KEYSTOREFILE) -storepass $(STOREPASS) makecapk.apk $(ALIASNAME)
	rm -rf $(APKFILE)
	$(BUILD_TOOLS)/zipalign -v 4 makecapk.apk $(APKFILE)
	#Using the apksigner in this way is only required on Android 30+
	$(BUILD_TOOLS)/apksigner sign --key-pass pass:$(STOREPASS) --ks-pass pass:$(STOREPASS) --ks $(KEYSTOREFILE) $(APKFILE)
	rm -rf temp.apk
	rm -rf makecapk.apk
	@ls -l $(APKFILE)

manifest: $(SRC_DIR)/AndroidManifest.xml

$(SRC_DIR)/AndroidManifest.xml :
	rm -rf $(SRC_DIR)/AndroidManifest.xml
	PACKAGENAME=$(PACKAGENAME) \
		ANDROIDVERSION=$(ANDROIDVERSION) \
		ANDROIDTARGET=$(ANDROIDTARGET) \
		APPNAME=$(APPNAME) \
		LABEL=$(LABEL) envsubst '$$ANDROIDTARGET $$ANDROIDVERSION $$APPNAME $$PACKAGENAME $$LABEL' \
		< $(SRC_DIR)/AndroidManifest.xml.template > $(SRC_DIR)/AndroidManifest.xml

uninstall : 
	($(ADB) uninstall $(PACKAGENAME))||true

push : makecapk.apk
	@echo "Installing" $(PACKAGENAME)
	$(ADB) install -r $(APKFILE)

logcat: push
	$(ADB) logcat | $(NDK)/ndk-stack -sym makecapk/lib/arm64-v8a/libSpectrogrammer.so

run : push
	$(eval ACTIVITYNAME:=$(shell $(AAPT) dump badging $(APKFILE) | grep "launchable-activity" | cut -f 2 -d"'"))
	$(ADB) shell am start -n $(PACKAGENAME)/$(ACTIVITYNAME)

clean :
	rm -rf AndroidManifest.xml temp.apk makecapk.apk makecapk $(APKFILE)
	rm -rf submodules/imgui/obj submodules/imgui/lib
	rm -rf submodules/kissfft/arm64-v8a 
	rm linux_version