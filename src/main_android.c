//Copyright (c) 2011-2020 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.
// NO WARRANTY! NO GUARANTEE OF SUPPORT! USE AT YOUR OWN RISK

#include "os_generic.h"
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android_native_app_glue.h>
#include "CNFGAndroid.h"
#include "Spectrogrammer.h"

#define CNFG_IMPLEMENTATION
#include "CNFG.h"

int lastbuttonx = 0;
int lastbuttony = 0;
int lastmotionx = 0;
int lastmotiony = 0;
int lastbid = 0;
int lastmask = 0;
int lastkey, lastkeydown;

uint8_t buttonstate[8];

void HandleKey( int keycode, int bDown )
{
	lastkey = keycode;
	lastkeydown = bDown;
}

void HandleButton( int x, int y, int button, int bDown )
{
	buttonstate[button] = bDown;
	lastbid = button;
	lastbuttonx = x;
	lastbuttony = y;
}

void HandleMotion( int x, int y, int mask )
{
	lastmask = mask;
	lastmotionx = x;
	lastmotiony = y;
}

int HandleDestroy()
{
	printf( "Destroying\n" );
	return 0;
}

volatile int suspended;

void HandleSuspend()
{
	suspended = 1;
}

void HandleResume()
{
	suspended = 0;
}

void HandleThisWindowTermination()
{
	suspended = 1;
	Spectrogrammer_Shutdown();
}

struct android_app * gapp; 

int main( int argc, char ** argv )
{
	CNFGBGColor = 0x000040ff;
	CNFGSetupFullscreen( "Test Bench", 0 );
	
	HandleWindowTermination = HandleThisWindowTermination;
	
	int hasperm = AndroidHasPermissions( "RECORD_AUDIO" );
	if( !hasperm )
	{
		AndroidRequestAppPermissions( "RECORD_AUDIO" );
	}
	
    Spectrogrammer_Init(gapp);

	while(1)
	{
		CNFGHandleInput();

		if( suspended ) { usleep(50000); continue; }

		Spectrogrammer_MainLoopStep();

		CNFGSwapBuffers();
	}

	return(0x0);
}

