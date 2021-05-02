package com.example.plasma;

import android.graphics.Bitmap;

public class Spectrogram {
    public static native int Update(Bitmap bitmap, long time_ms);
    public static native void SetFftLength(int fftLength);
    public static native int GetFftLength();
    public static native void SetBarsHeight(int barsHeight);
    public static native void SetOverlap(float timeOverlap);
    public static native void SetDecay(float decay);
    public static native float GetOverlap();
    public static native float GetDecay();
    public static native float XToFreq(double x);
    public static native float FreqToX(double freq);
    public static native void ConnectWithAudio();
    public static native void SetFrequencyLogarithmicAxis(boolean bLogarithmic);
    public static native boolean GetFrequencyLogarithmicAxis();
    public static native void SetMinMaxFreqs(double min, double max);
}
