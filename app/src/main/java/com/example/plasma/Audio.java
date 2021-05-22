package com.example.plasma;

import android.content.Context;
import android.graphics.Bitmap;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;

public class Audio {
    // load our native library
    static {
        System.loadLibrary("plasma");
    }

    private static native void createSLEngine(int sampleRate, int framesPerBuf);
    private static native void createAudioRecorder();
    private static native void deleteAudioRecorder();
    private static native void startPlay();
    private static native void stopPlay();
    public static native void pausePlay();
    private static native void deleteSLEngine();
    private static native float getSampleRate();

    public static void onCreate(AudioManager myAudioMgr)
    {
        int nativeSampleRate  =  Integer.parseInt(myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE));
        int  nativeSampleBufSize = Integer.parseInt(myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER));
        int recBufSize = AudioRecord.getMinBufferSize(nativeSampleRate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);

        createSLEngine(nativeSampleRate, nativeSampleBufSize*4);
        createAudioRecorder();
        startPlay();
    }

    public static void onDestroy()
    {
        stopPlay();
        deleteAudioRecorder();
        deleteSLEngine();
    }
}
