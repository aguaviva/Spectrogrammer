//Copyright (c) 2011-2020 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.
// NO WARRANTY! NO GUARANTEE OF SUPPORT! USE AT YOUR OWN RISK

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
//#include <cstdarg>
#include <stdarg.h>
#include "os_generic.h"
#include <GLES3/gl3.h>
#ifdef ANDROID
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android_native_app_glue.h>
#include <android/sensor.h>
#else
#include <GLFW/glfw3.h> 
#endif
#include <byteswap.h>
#include <errno.h>
#include <fcntl.h>
#include "CNFGAndroid.h"
#include "Spectrogrammer.h"

//#define CNFA_IMPLEMENTATION
#define CNFG_IMPLEMENTATION
#include "CNFG.h"


#ifdef ANDROID
ALooper * l;
#endif
const uint32_t SAMPLE_RATE = 44100;
const uint16_t SAMPLE_COUNT = 512;
uint32_t stream_offset = 0;
uint16_t audio_frequency;




//writes the text to a file to path (example): /storage/emulated/0/Android/data/org.yourorg.cnfgtest/files
// You would not normally want to do this, but it's an example of how to do local storage.
void Log(const char *fmt, ...)
{
#if ANDROID	
	const char* getpath = AndroidGetExternalFilesDir();
#else
	const char* getpath = ".";
#endif	
	char buffer[2048];
	snprintf(buffer, sizeof(buffer), "%s/log.txt", getpath);
	FILE *f = fopen(buffer, "w");
	if (f == NULL)
	{
		exit(1);
	}

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);	

	fprintf(f, "%s\n", buffer);

	fclose(f);
}
short screenx, screeny;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main( int argc, char ** argv )
{
	int frames = 0;
	double ThisTime;
	double LastFPSTime = OGGetAbsoluteTime();

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1080/2, 1920/2, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    Spectrogrammer_Init(window);

	Log( "Startup Complete" );

    while (!glfwWindowShouldClose(window))
	{

		glfwPollEvents();

		Spectrogrammer_MainLoopStep();

		frames++;
		
		glfwSwapBuffers(window);

		ThisTime = OGGetAbsoluteTime();
		if( ThisTime > LastFPSTime + 1 )
		{
			printf( "FPS: %d\n", frames );
			frames = 0;
			LastFPSTime+=1;
		}

	}

	Spectrogrammer_Shutdown();

	return(0x0);
}

