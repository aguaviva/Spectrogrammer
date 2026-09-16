// dear imgui: standalone example application for Android + OpenGL ES 3

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
//#include "imgui_impl_android.h"
#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_opengl3.h"
#include <android/log.h>
#include <android/configuration.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>
#include "Spectrogrammer.h"
#include "audio/audio_main.h"

// Data
static EGLDisplay           g_EglDisplay = EGL_NO_DISPLAY;
static EGLSurface           g_EglSurface = EGL_NO_SURFACE;
static EGLContext           g_EglContext = EGL_NO_CONTEXT;
extern struct android_app*  g_App;
static bool                 g_Initialized = false;
static bool                 g_AppInitialized = false;
static bool                 g_HasFocus = false;
static bool                 g_RenderRequested = true;
static bool                 g_PermissionRequested = false;
static bool                 g_AudioPaused = false;
static char                 g_LogTag[] = "ImGuiExample";
static std::string          g_IniFilename = "";

// Forward declarations of helper functions
static void Start(struct android_app* app);
static void Stop();
static void Init(struct android_app* app);
static void Resume(struct android_app* app);
static void Shutdown(bool final);
static void MainLoopStep();
static void AndroidDisplayKeyboard(int pShow);
static int GetAssetData(const char* filename, void** out_data);
static void keep_screen_on();
static bool HasAudioPermission();
static void RequestAudioPermission();

// Main code
static void handleAppCmd(struct android_app* app, int32_t appCmd)
{
    switch (appCmd)
    {
    case APP_CMD_START:
        Start(app);
        break;
    case APP_CMD_STOP:
        Stop();
        break;
    case APP_CMD_SAVE_STATE:
        break;
    case APP_CMD_INIT_WINDOW:
        Init(app);
        break;
    case APP_CMD_TERM_WINDOW:
        Shutdown(false);
        break;
    case APP_CMD_GAINED_FOCUS:
        g_HasFocus = true;
        g_RenderRequested = true;
        if (g_AppInitialized && g_AudioPaused)
        {
            Audio_startPlay();
            g_AudioPaused = false;
        }
        break;
    case APP_CMD_LOST_FOCUS:
        g_HasFocus = false;
        if (g_AppInitialized && !g_AudioPaused)
        {
            Audio_pausePlay();
            g_AudioPaused = true;
        }
        break;
    case APP_CMD_RESUME:
        Resume(app);
        break;        
    }
}

static int32_t handleInputEvent(struct android_app* app, AInputEvent* inputEvent)
{
    g_RenderRequested = true;
    if (!g_Initialized)
        return 0;
    return ImGui_ImplAndroid_HandleInputEvent(inputEvent);
}

extern "C" void android_main(struct android_app* app)
{
    app->onAppCmd = handleAppCmd;
    app->onInputEvent = handleInputEvent;

    while (true)
    {
        int out_events;
        struct android_poll_source* out_data;

        const bool animate = g_Initialized && g_HasFocus && Spectrogrammer_ShouldAnimate();
        const int timeout = animate ? 0 : ((!g_AppInitialized && app->window) ? 100 : -1);
        int poll_timeout = timeout;
        while (ALooper_pollAll(poll_timeout, nullptr, &out_events, (void**)&out_data) >= 0)
        {
            poll_timeout = 0;
            // Process one event
            if (out_data != nullptr)
                out_data->process(app, out_data);

            // Exit the app by returning from within the infinite loop
            if (app->destroyRequested != 0)
            {
                Shutdown(true);
                return;
            }
        }

        if (!g_Initialized && app->window && HasAudioPermission())
            Init(app);

        if (g_Initialized && g_HasFocus && (animate || g_RenderRequested))
        {
            MainLoopStep();
            g_RenderRequested = false;
        }
    }
}

void Start(struct android_app* app)
{
}

void Stop(  )
{   
}

void Resume(struct android_app* app)
{
    g_App = app;
    keep_screen_on();
}

void Init(struct android_app* app)
{
    if (g_Initialized)
        return;

    g_App = app;
    if (!HasAudioPermission())
    {
        RequestAudioPermission();
        return;
    }

    ANativeWindow_acquire(g_App->window);

    keep_screen_on();

    // Initialize EGL
    // This is mostly boilerplate code for EGL...
    {
        g_EglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (g_EglDisplay == EGL_NO_DISPLAY)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglGetDisplay(EGL_DEFAULT_DISPLAY) returned EGL_NO_DISPLAY");

        if (eglInitialize(g_EglDisplay, 0, 0) != EGL_TRUE)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglInitialize() returned with an error");

        const EGLint egl_attributes[] = { EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE };
        EGLint num_configs = 0;
        if (eglChooseConfig(g_EglDisplay, egl_attributes, nullptr, 0, &num_configs) != EGL_TRUE)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglChooseConfig() returned with an error");
        if (num_configs == 0)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglChooseConfig() returned 0 matching config");

        // Get the first matching config
        EGLConfig egl_config;
        eglChooseConfig(g_EglDisplay, egl_attributes, &egl_config, 1, &num_configs);
        EGLint egl_format;
        eglGetConfigAttrib(g_EglDisplay, egl_config, EGL_NATIVE_VISUAL_ID, &egl_format);
        ANativeWindow_setBuffersGeometry(g_App->window, 0, 0, egl_format);

        const EGLint egl_context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        g_EglContext = eglCreateContext(g_EglDisplay, egl_config, EGL_NO_CONTEXT, egl_context_attributes);

        if (g_EglContext == EGL_NO_CONTEXT)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglCreateContext() returned EGL_NO_CONTEXT");

        g_EglSurface = eglCreateWindowSurface(g_EglDisplay, egl_config, g_App->window, nullptr);
        eglMakeCurrent(g_EglDisplay, g_EglSurface, g_EglSurface, g_EglContext);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Redirect loading/saving of .ini file to our location.
    // Make sure 'g_IniFilename' persists while we use Dear ImGui.
    g_IniFilename = std::string(app->activity->internalDataPath) + "/imgui.ini";
    io.IniFilename = g_IniFilename.c_str();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplAndroid_Init(g_App->window);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    int density = AConfiguration_getDensity(app->config);
    float ui_scale = density > 0 ? (float)density / 160.0f : 1.0f;
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 18.0f * ui_scale;
    io.Fonts->AddFontDefault(&font_cfg);
    ImGui::GetStyle().ScaleAllSizes(ui_scale);

    if (!g_AppInitialized)
    {
        Spectrogrammer_Init(app);
        g_AppInitialized = true;
        if (!g_HasFocus)
        {
            Audio_pausePlay();
            g_AudioPaused = true;
        }
    }

    g_Initialized = true;
    g_RenderRequested = true;
}

void MainLoopStep()
{
    ImGuiIO& io = ImGui::GetIO();
    if (g_EglDisplay == EGL_NO_DISPLAY)
        return;

    // Open on-screen (soft) input if requested by Dear ImGui
    static bool WantTextInputLast = false;
    if (io.WantTextInput && !WantTextInputLast)
        AndroidDisplayKeyboard(io.WantTextInput?1:0);
    WantTextInputLast = io.WantTextInput;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    Spectrogrammer_MainLoopStep();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    eglSwapBuffers(g_EglDisplay, g_EglSurface);
}

void Shutdown(bool final)
{
    if (g_Initialized)
    {
        if (final && g_AppInitialized)
        {
            Spectrogrammer_Shutdown();
            g_AppInitialized = false;
        }
        else
        {
            Spectrogrammer_InvalidateGraphics();
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        ImGui::DestroyContext();

        if (g_EglDisplay != EGL_NO_DISPLAY)
        {
            eglMakeCurrent(g_EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            if (g_EglContext != EGL_NO_CONTEXT)
                eglDestroyContext(g_EglDisplay, g_EglContext);

            if (g_EglSurface != EGL_NO_SURFACE)
                eglDestroySurface(g_EglDisplay, g_EglSurface);

            eglTerminate(g_EglDisplay);
        }

        g_EglDisplay = EGL_NO_DISPLAY;
        g_EglContext = EGL_NO_CONTEXT;
        g_EglSurface = EGL_NO_SURFACE;
        ANativeWindow_release(g_App->window);

        g_Initialized = false;
    }

    if (final && g_AppInitialized)
    {
        Spectrogrammer_Shutdown();
        g_AppInitialized = false;
    }
}

// Helper functions

#ifdef __cplusplus
#define SETUP_FOR_JAVA_CALL \
	JNIEnv * env = 0; \
	JNIEnv ** envptr = &env; \
	JavaVM * jniiptr = g_App->activity->vm; \
	jniiptr->AttachCurrentThread( (JNIEnv**)&env, 0 ); \
	env = (*envptr);
#define ENVCALL
#define JAVA_CALL_DETACH 	jniiptr->DetachCurrentThread();
#else
#define SETUP_FOR_JAVA_CALL \
	const struct JNINativeInterface * env = (struct JNINativeInterface*)g_App->activity->env; \
	const struct JNINativeInterface ** envptr = &env; \
	const struct JNIInvokeInterface ** jniiptr = gag_Apppp->activity->vm; \
	const struct JNIInvokeInterface * jnii = *jniiptr; \
	jnii->AttachCurrentThread( jniiptr, &envptr, NULL); \
	env = (*envptr);
#define ENVCALL envptr,
#define JAVA_CALL_DETACH       	jnii->DetachCurrentThread( jniiptr );
#endif

bool HasAudioPermission()
{
    if (g_App == nullptr)
        return false;

    SETUP_FOR_JAVA_CALL
    jclass activityClass = env->GetObjectClass(g_App->activity->clazz);
    jmethodID checkPermission = env->GetMethodID(
        activityClass, "checkSelfPermission", "(Ljava/lang/String;)I");
    jstring permission = env->NewStringUTF("android.permission.RECORD_AUDIO");
    jint result = env->CallIntMethod(g_App->activity->clazz, checkPermission, permission);
    env->DeleteLocalRef(permission);
    env->DeleteLocalRef(activityClass);
    JAVA_CALL_DETACH
    return result == 0;
}

void RequestAudioPermission()
{
    if (g_PermissionRequested)
        return;
    g_PermissionRequested = true;

    SETUP_FOR_JAVA_CALL
    jclass activityClass = env->GetObjectClass(g_App->activity->clazz);
    jmethodID requestPermissions = env->GetMethodID(
        activityClass, "requestPermissions", "([Ljava/lang/String;I)V");
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray permissions = env->NewObjectArray(1, stringClass, nullptr);
    jstring permission = env->NewStringUTF("android.permission.RECORD_AUDIO");
    env->SetObjectArrayElement(permissions, 0, permission);
    env->CallVoidMethod(g_App->activity->clazz, requestPermissions, permissions, 1);
    env->DeleteLocalRef(permission);
    env->DeleteLocalRef(permissions);
    env->DeleteLocalRef(stringClass);
    env->DeleteLocalRef(activityClass);
    JAVA_CALL_DETACH
}

void keep_screen_on() 
{
    RunCallbackOnUIThread([](void* opaque) {
        if (!g_App || !g_App->activity) return;

        JNIEnv* env = g_App->activity->env;
        if (!env) return;

        jclass activityClass = env->FindClass("android/app/NativeActivity");
        if (!activityClass) return;

        jobject lNativeActivity = g_App->activity->clazz;

        jmethodID getWindowMethod = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
        if (!getWindowMethod) return;

        jobject lWindow = env->CallObjectMethod(lNativeActivity, getWindowMethod);
        if (!lWindow) return;

        jclass ClassWindow = env->FindClass("android/view/Window");
        if (!ClassWindow) return;

        // 1. Keep Screen On Flag
        jmethodID addFlagsMethod = env->GetMethodID(ClassWindow, "addFlags", "(I)V");
        if (addFlagsMethod) {
            const int FLAG_KEEP_SCREEN_ON = 128; // 0x80
            env->CallVoidMethod(lWindow, addFlagsMethod, FLAG_KEEP_SCREEN_ON);
        }

        // 2. Clear Fullscreen & No Limits Flags (ensures status bar space is reserved)
        jmethodID clearFlagsMethod = env->GetMethodID(ClassWindow, "clearFlags", "(I)V");
        if (clearFlagsMethod) {
            const int FLAG_FULLSCREEN = 1024;      // 0x400
            const int FLAG_LAYOUT_NO_LIMITS = 512; // 0x200
            env->CallVoidMethod(lWindow, clearFlagsMethod, FLAG_FULLSCREEN | FLAG_LAYOUT_NO_LIMITS);
        }

        // 3. Prevent rendering in display cutout / camera hole (LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER)
        jmethodID getAttributesMethod = env->GetMethodID(ClassWindow, "getAttributes", "()Landroid/view/WindowManager$LayoutParams;");
        if (getAttributesMethod) {
            jobject lParams = env->CallObjectMethod(lWindow, getAttributesMethod);
            if (lParams) {
                jclass ClassLayoutParams = env->FindClass("android/view/WindowManager$LayoutParams");
                if (ClassLayoutParams) {
                    jfieldID cutoutModeField = env->GetFieldID(ClassLayoutParams, "layoutInDisplayCutoutMode", "I");
                    if (cutoutModeField) {
                        const int LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER = 2;
                        env->SetIntField(lParams, cutoutModeField, LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER);

                        jmethodID setAttributesMethod = env->GetMethodID(ClassWindow, "setAttributes", "(Landroid/view/WindowManager$LayoutParams;)V");
                        if (setAttributesMethod) {
                            env->CallVoidMethod(lWindow, setAttributesMethod, lParams);
                        }
                    }
                }
            }
        }

        // 4. Force status bar to show on Android 11+ (API 30+)
        jmethodID getInsetsControllerMethod = env->GetMethodID(ClassWindow, "getInsetsController", "()Landroid/view/WindowInsetsController;");
        if (getInsetsControllerMethod) {
            jobject lController = env->CallObjectMethod(lWindow, getInsetsControllerMethod);
            if (lController) {
                jclass ClassInsetsController = env->FindClass("android/view/WindowInsetsController");
                if (ClassInsetsController) {
                    jmethodID showMethod = env->GetMethodID(ClassInsetsController, "show", "(I)V");
                    if (showMethod) {
                        const int STATUS_BARS = 1; // WindowInsets.Type.statusBars()
                        env->CallVoidMethod(lController, showMethod, STATUS_BARS);
                    }
                }
            }
        }

    }, nullptr);
}

void AndroidDisplayKeyboard(int pShow)
{
	//Based on https://stackoverflow.com/questions/5864790/how-to-show-the-soft-keyboard-on-native-activity
	jint lFlags = 0;
	SETUP_FOR_JAVA_CALL

	jclass activityClass = env->FindClass( ENVCALL "android/app/NativeActivity");

	// Retrieves NativeActivity.
	jobject lNativeActivity = g_App->activity->clazz;


	// Retrieves Context.INPUT_METHOD_SERVICE.
	jclass ClassContext = env->FindClass( ENVCALL "android/content/Context");
	jfieldID FieldINPUT_METHOD_SERVICE = env->GetStaticFieldID( ENVCALL ClassContext, "INPUT_METHOD_SERVICE", "Ljava/lang/String;" );
	jobject INPUT_METHOD_SERVICE = env->GetStaticObjectField( ENVCALL ClassContext, FieldINPUT_METHOD_SERVICE );

	// Runs getSystemService(Context.INPUT_METHOD_SERVICE).
	jclass ClassInputMethodManager = env->FindClass( ENVCALL "android/view/inputmethod/InputMethodManager" );
	jmethodID MethodGetSystemService = env->GetMethodID( ENVCALL activityClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
	jobject lInputMethodManager = env->CallObjectMethod( ENVCALL lNativeActivity, MethodGetSystemService, INPUT_METHOD_SERVICE);

	// Runs getWindow().getDecorView().
	jmethodID MethodGetWindow = env->GetMethodID( ENVCALL activityClass, "getWindow", "()Landroid/view/Window;");
	jobject lWindow = env->CallObjectMethod( ENVCALL lNativeActivity, MethodGetWindow);
	jclass ClassWindow = env->FindClass( ENVCALL "android/view/Window");
	jmethodID MethodGetDecorView = env->GetMethodID( ENVCALL ClassWindow, "getDecorView", "()Landroid/view/View;");
	jobject lDecorView = env->CallObjectMethod( ENVCALL lWindow, MethodGetDecorView);

	if (pShow) {
		// Runs lInputMethodManager.showSoftInput(...).
		jmethodID MethodShowSoftInput = env->GetMethodID( ENVCALL ClassInputMethodManager, "showSoftInput", "(Landroid/view/View;I)Z");
		/*jboolean lResult = */env->CallBooleanMethod( ENVCALL lInputMethodManager, MethodShowSoftInput, lDecorView, lFlags);
	} else {
		// Runs lWindow.getViewToken()
		jclass ClassView = env->FindClass( ENVCALL "android/view/View");
		jmethodID MethodGetWindowToken = env->GetMethodID( ENVCALL ClassView, "getWindowToken", "()Landroid/os/IBinder;");
		jobject lBinder = env->CallObjectMethod( ENVCALL lDecorView, MethodGetWindowToken);

		// lInputMethodManager.hideSoftInput(...).
		jmethodID MethodHideSoftInput = env->GetMethodID( ENVCALL ClassInputMethodManager, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z");
		/*jboolean lRes = */env->CallBooleanMethod( ENVCALL lInputMethodManager, MethodHideSoftInput, lBinder, lFlags);
	}

	JAVA_CALL_DETACH
}


// Helper to retrieve data placed into the assets/ directory (android/app/src/main/assets)
static int GetAssetData(const char* filename, void** outData)
{
    int num_bytes = 0;
    AAsset* asset_descriptor = AAssetManager_open(g_App->activity->assetManager, filename, AASSET_MODE_BUFFER);
    if (asset_descriptor)
    {
        num_bytes = AAsset_getLength(asset_descriptor);
        *outData = IM_ALLOC(num_bytes);
        int64_t num_bytes_read = AAsset_read(asset_descriptor, *outData, num_bytes);
        AAsset_close(asset_descriptor);
        IM_ASSERT(num_bytes_read == num_bytes);
    }
    return num_bytes;
}
