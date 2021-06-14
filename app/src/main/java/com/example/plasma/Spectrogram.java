package com.example.plasma;

import android.graphics.Bitmap;

public class Spectrogram {
    public static native int GetFftLength();
    public static native void SetBarsHeight(int barsHeight);
    public static native void SetOverlap(float timeOverlap);
    public static native void SetVolume(float volume);
    public static native float GetVolume();
    public static native void SetDecay(float decay);
    public static native float GetOverlap();
    public static native float GetDecay();
    public static native float XToFreq(double x);
    public static native float FreqToX(double freq);
    public static native void SetScaler(int width, double min, double max, boolean bLogX, boolean bLogY);
    public static native void SetProcessorFFT(int length);
    public static native void SetProcessorGoertzel(int length);

    public static native void HoldData();
    public static native void ClearHeldData();

    public static native void Init(Bitmap bitmap);
    public static native void ConnectWithAudioMT();
    public static native void Disconnect();
    public static native int Lock(Bitmap bitmap);
    public static native void Unlock(Bitmap bitmap);
    public static native int GetDroppedFrames();
}
