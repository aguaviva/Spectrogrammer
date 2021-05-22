package com.example.plasma;

import android.graphics.Bitmap;

public class Spectrogram {
    public static native void SetFftLength(int fftLength);
    public static native int GetFftLength();
    public static native void SetBarsHeight(int barsHeight);
    public static native void SetOverlap(float timeOverlap);
    public static native void SetDecay(float decay);
    public static native float GetOverlap();
    public static native float GetDecay();
    public static native float XToFreq(double x);
    public static native float FreqToX(double freq);
    public static native void SetScaler(int width, double min, double max, boolean bLogX, boolean bLogY);
    public static native void SetProcessor(int processor);

    public static native void ConnectWithAudioMT(Bitmap bitmap);
    public static native int Lock(Bitmap bitmap);
    public static native void Unlock(Bitmap bitmap);
    public static native int GetDroppedFrames();
}
