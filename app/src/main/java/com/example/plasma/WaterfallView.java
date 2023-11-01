package com.example.plasma;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.icu.util.TimeZone;
import android.os.Build;
import android.os.Environment;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

import java.io.File;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.ZoneOffset;
import java.time.format.DateTimeFormatter;
import java.time.format.FormatStyle;
import java.util.Date;

import androidx.annotation.RequiresApi;

import static java.lang.Math.ceil;

public class WaterfallView extends View {

    interface Listener
    {
        public void OnHitBottom(Bitmap bitmap);
    }

    public enum ProcessorMode {
        NONE,
        FFT,
        MUSIC,
    }

    public enum Overlay {
        NONE,
        PIANO,
        GUITAR,
    }

    private final Paint white;
    private final Paint gray;
    private final Paint drawPaint;
    private Bitmap mBitmap = null;
    private int mBarsHeight = 200;
    private boolean mLogX = false;
    private boolean mLogY = true;
    private boolean mShowDebugInfo = false;
    private float m_seconds_per_tick = 0;
    private float m_old_lines_per_second = -1;

    private DateTimeFormatter mFormatter = null;
    private long mScaleTimeAtTop = 0;
    private float mSecondsPerScreen = 0;

    private int mScaleType = 0;
    private ProcessorMode mProcessorMode = ProcessorMode.NONE;
    private Overlay mOverlay = Overlay.NONE;
    private Rect bars;
    private long delay = 0;
    private Viewport viewport = new Viewport();
    private Listener mCallbacks = null;

    public WaterfallView(Context context, AttributeSet attrs) {
        super(context, attrs);

        mFormatter = DateTimeFormatter.ofPattern("HH:mm:ss");
        white = new Paint();
        white.setColor(Color.WHITE);
        int scaledSize = getResources().getDimensionPixelSize(R.dimen.font_size);
        white.setTextSize(scaledSize);

        gray = new Paint();
        gray.setColor(Color.GRAY);
        gray.setAlpha(128 + 64);

        drawPaint = new Paint();
        drawPaint.setAntiAlias(false);
        drawPaint.setFilterBitmap(false);

        viewport.Init(this);
    }

    public void setListener(Listener l) { mCallbacks = l; }
    public void setProcessor(ProcessorMode processorMode)
    {
        mProcessorMode = processorMode;
    }
    public ProcessorMode getProcessor() { return mProcessorMode; }
    public void setLogX(boolean b) { mLogX = b; updateScaler(); }
    public boolean getLogX() { return mLogX; }
    public void setLogY(boolean b) { mLogY = b; updateScaler(); }
    public boolean getLogY() { return mLogY; }
    public void setShowDebugInfo(boolean checked) { mShowDebugInfo = checked; }
    public boolean getShowDebugInfo() { return mShowDebugInfo; }
    @RequiresApi(api = Build.VERSION_CODES.O)
    public int getScaleType() { return mScaleType; }
    public void setScaleType(int type) { mScaleType = type; }

    public void clearWaterfall()
    {
        Spectrogram.ResetScanline();

        int waterfallLines = getHeight()-mBarsHeight;
        mSecondsPerScreen = waterfallLines / LinesPerSecond();
        mScaleTimeAtTop = System.currentTimeMillis();

        if (mBitmap!=null)
            mBitmap.eraseColor(Color.BLACK);
    }


    public void updateScaler()
    {
        if (mBitmap==null)
            return;

        switch(mProcessorMode) {
            case FFT: {
                Spectrogram.SetScaler(mBitmap.getWidth(), 100, 48000 / 2, mLogX, mLogY);
                break;
            }
            case MUSIC: {
                Spectrogram.SetScaler(mBitmap.getWidth(), 1, 88, false, false);
                break;
            }
        }
    }
    @Override
    protected void onSizeChanged (int w, int h, int oldw, int oldh)
    {
        if (w>0 && h>0)
        {
            mBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.RGB_565);
            bars = new Rect(0, 0, mBitmap.getWidth(), mBarsHeight);
            updateScaler();
            Spectrogram.Init(mBitmap);
        }
        else
        {
            mBitmap = null;
        }
    }

    float x=0,y=0;
    boolean mMeasuring = false;

    void SetMeasuring(boolean measuring)
    {
        mMeasuring = measuring;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        if (mMeasuring) {
            x = event.getX(0);
            y = event.getY(0);

            if (y<= mBarsHeight)
            {
                Spectrogram.HoldData();
            }

        }
        else {
            viewport.onTouchEvent(event);
        }
        return true;
    }

    private void drawWaterfall(Canvas canvas, int currentRow, int barsHeight)
    {
        // draw waterfall
        int topHalf = (barsHeight + 1) + mBitmap.getHeight() - currentRow;
        canvas.drawBitmap(mBitmap, new Rect(0, currentRow, mBitmap.getWidth(), mBitmap.getHeight()), new Rect(0, barsHeight + 1, mBitmap.getWidth(), topHalf), null);
        canvas.drawBitmap(mBitmap, new Rect(0, barsHeight + 1, mBitmap.getWidth(), currentRow), new Rect(0, topHalf, mBitmap.getWidth(), mBitmap.getHeight()), null);
    }

    float lerpf(float t, float a, float b) { return a + (float)(((float)(b-a))*t); }
    long lerpu(float t, long a, long b) { return a + (long)(((float)(b-a))*t); }
    float unlerpu(long a, long b, long t) { return (float)(((double)(t-a))/((double)(b-a))); }

    @RequiresApi(api = Build.VERSION_CODES.O)
    @Override
    protected void onDraw(Canvas canvas) {

        canvas.save();
        canvas.translate(viewport.GetPos().x, 0);
        canvas.scale(viewport.GetScale().x, 1);

        if (mBitmap != null) {
            int currentRow = Spectrogram.Lock(mBitmap);
            if (currentRow >= 0) {
                // draw bars
                canvas.drawBitmap(mBitmap, bars, bars, drawPaint);
                drawWaterfall(canvas, currentRow, mBarsHeight);

                if (currentRow==mBitmap.getHeight())
                {
                    if (mCallbacks!=null)
                    {
                        Bitmap bitmap = Bitmap.createBitmap(getWidth(),getHeight(), Bitmap.Config.RGB_565);
                        Canvas canvas2 = new Canvas(bitmap);
                        drawWaterfall(canvas2, currentRow, mBarsHeight);

                        //mCallbacks.OnHitBottom(bitmap);
                    }
                }
            }
            Spectrogram.Unlock(mBitmap);
        }
        canvas.restore();

        {
            int dp = Spectrogram.GetDroppedFrames();
            if (dp > 0)
                delay = System.currentTimeMillis();
            if (delay > 0) {
                canvas.drawText("Overload!! Dropping audio frames", 10, 250, white);
                if (System.currentTimeMillis()-delay > 500)
                    delay = 0;
            }
        }

        if (mShowDebugInfo) {
            int x = 10;
            int y = (int) (250 + white.descent() - white.ascent());
            String text = Spectrogram.GetDebugInfo();
            for (String line : text.split("\n")) {
                canvas.drawText(line, x, y, white);
                y += white.descent() - white.ascent();
            }
        }

        if (mProcessorMode == ProcessorMode.FFT)
        {
            if (mLogX)
            {
                DrawLogarithmicX(canvas);
            }
            else
            {
                DrawLinearX(canvas);
            }
        }
        else if (mProcessorMode == ProcessorMode.MUSIC)
        {
            if (mOverlay == Overlay.PIANO)
            {
                DrawPianoOverlay(canvas);
            }
            else if (mOverlay == Overlay.GUITAR)
            {
                DrawGuitarOverlay(canvas);
            }
        }

        // Draw scale
        //
        if (mScaleType==2)
        {
            //int seconds = mScaleTimeAtBottom.minus(mScaleTimeAtTop).getSecond();
            {
                LocalDateTime ldt = Instant.ofEpochMilli(mScaleTimeAtTop).atZone(ZoneId.systemDefault()).toLocalDateTime();
                String s = mFormatter.format(ldt);
                canvas.drawText(s, getWidth() - 200, mBarsHeight, white);
            }

            long mScaleTimeAtBottom = mScaleTimeAtTop + (long)(mSecondsPerScreen*1000);

            long rd = 2*60*1000;

            for(int i=1;i<=20;i++)
            {
                float t = (float)i /    20.0f;

                long rt = lerpu(t, RoundEpoch(mScaleTimeAtTop+rd,rd) , RoundEpoch(mScaleTimeAtBottom+rd, rd));

                float rtt = unlerpu(mScaleTimeAtTop, mScaleTimeAtBottom, rt);
                float y =  lerpf(rtt, mBarsHeight, getHeight());

                String s = mFormatter.format(Instant.ofEpochMilli(rt).atZone(ZoneId.systemDefault()).toLocalDateTime());
                canvas.drawText(s, getWidth() - 200, y, white);
                canvas.drawLine(getWidth() - 50,y,getWidth(), y, white);
            }

        }
        else
        {

            Paint.FontMetrics fm = white.getFontMetrics();
            float fontHeight = fm.descent - fm.ascent;

            int lines_per_page = getHeight() - mBarsHeight;
            float lines_per_second = LinesPerSecond();

            if (lines_per_second!=m_old_lines_per_second) {
                m_old_lines_per_second = lines_per_second;

                float seconds_per_page = lines_per_page / lines_per_second;

                float lines_per_tick = (fontHeight * 2);
                float seconds_per_tick = lines_per_tick / lines_per_second;
                float ticks_per_page = lines_per_page / lines_per_tick;
                float desired_ticks_per_page = lines_per_page / lines_per_tick;

                // round seconds per tick

                float[] anchors = {0.001f, 0.005f, 0.01f, 0.05f, 0.1f, 0.5f, 1.0f, 5.0f, 10.0f, 15.0f, 30.0f, 60.0f, 5 * 60.0f, 10 * 60.0f, 20 * 60.0f, 30 * 60.0f, 60 * 60.0f};

                int min_err = 10000;
                for (int i = 0; i < anchors.length; i++) {
                    float t = 0;
                    float a = anchors[i];
                    if (a < 1)
                        t = Math.round(seconds_per_tick * (a * 100)) / (a * 100);
                    else
                        t = Math.round(seconds_per_tick * a) / a;
                    if (t > 0) {
                        int ticks = (int) Math.ceil(seconds_per_page / a);
                        int err = (int) Math.abs(desired_ticks_per_page - ticks);
                        if (err < min_err) {
                            min_err = err;
                            m_seconds_per_tick = a;
                        }
                    }
                }
            }

            float lines_per_tick = m_seconds_per_tick * lines_per_second;

            float t = 0;
            for(int i = mBarsHeight+(int)lines_per_tick; i<=(int)getHeight(); i+=lines_per_tick)
            {
                float y = i;
                canvas.drawLine(getWidth()-50,y,getWidth(), y, white);
                t += m_seconds_per_tick;
                if (t < 1)
                    canvas.drawText(String.format("%.2fs", t), getWidth() - 200, y, white);
                else
                    canvas.drawText(FormatTime((int)t), getWidth() - 200, y, white);
            }
        }

        // Draw measurements
        //
        if (mMeasuring)
        {
            float xx = viewport.fromScreenSpace(x);
            String str = "none";

            int freq = (int)Spectrogram.XToFreq(xx);

            switch(mProcessorMode)
            {
                case FFT:
                    str = String.format("%d Hz", freq); break;
                case MUSIC:
                    str = getNoteName(freq); break;
            }

            canvas.drawText(str, x, y - mBarsHeight, white);
            canvas.drawLine(x,0, x, getHeight(), white);

            // draw time graphs
            float delta = LinesPerSecond();
            if (delta*60<100)
            {
                delta*=60;
            }

            for(int i=1;i<60;i++)
            {
                float yy = y - i*delta;
                canvas.drawLine(x-20, yy, x+20, yy, white);
            }

        }

        viewport.EnforceMinimumSize();

        invalidate();
    }


    long RoundEpoch(long t, long millis)
    {
        return t - (t % millis);
    }

    float LinesPerSecond()
    {
        float delta = (48000.0f/(Spectrogram.GetFftLength()* (1.0f - Spectrogram.GetOverlap())));
        delta /= Spectrogram.GetAverageCount();
        return delta;
    }

    String FormatTime(int t)
    {
        int h  = (int) (t / (60 * 60));
        int mm = (int) (t % (60 * 60));
        int m = (int) (mm / 60);
        int s = (int) (mm % 60);

        if (h>0)
            if (s>0)
                return String.format("%dh%02dm%02ds",h,m,s);
            else
                return String.format("%dh%02dm",h,m);
        else if (m>0)
            if (s>0)
                return String.format("%02dm%02ds",m,s);
            else
                return String.format("%02dm",m);
        else if (s>0)
                return String.format("%02ds",s);

        return String.format("err %d",t);
    }

    private void DrawLogarithmicX(Canvas canvas)
    {
        float d = 1;
        for (int j = 0; j < 5; j++) {
            for (int i = 0; i < 10; i++) {
                float x;
                x = Spectrogram.FreqToX(i * d);
                x = viewport.toScreenSpace(x);
                canvas.drawLine(x, 0, x, canvas.getHeight(), gray);
            }
            d *= 10;
        }
    }

    private void DrawLinearX(Canvas canvas) {
        for (int i = 0; i < 48000 / 2; i += 1000) {
            float x = Spectrogram.FreqToX(i);
            x = viewport.toScreenSpace(x);
            canvas.drawLine(x, 0, x, canvas.getHeight(), gray);
        }
    }

    private void DrawPianoOverlay(Canvas canvas) {
        Rect bounds = new Rect();
        for (int n = 4; n < 88; n += 12) {
            float x = Spectrogram.FreqToX(n);
            float x1 = Spectrogram.FreqToX(n + 1);

            x = viewport.toScreenSpace(x);
            x1 = viewport.toScreenSpace(x1);

            canvas.drawLine(x, 0, x, canvas.getHeight(), gray);

            String text = getNoteName(n);
            white.getTextBounds(text, 0, text.length(), bounds);
            if ((x1 - x) > bounds.width())
                canvas.drawText(text, x, 10 + bounds.height(), white);
        }
    }

    private void DrawGuitarOverlay(Canvas canvas) {
        Rect bounds = new Rect();
        int[] notes = {20, 25, 30, 35, 39, 44};
        for (int i = 0; i < notes.length; i++) {

            float x = Spectrogram.FreqToX(notes[i]);
            x = viewport.toScreenSpace(x);
            canvas.drawLine(x, 0, x, canvas.getHeight(), gray);

            float x1 = Spectrogram.FreqToX(notes[i] + 1);
            x1 = viewport.toScreenSpace(x1);
            canvas.drawLine(x1, 0, x1, canvas.getHeight(),
                    gray);


            String text = getNoteName(notes[i]);
            white.getTextBounds(text, 0, text.length(), bounds);
            canvas.drawText(text, x, 10 + bounds.height(), white);
        }
    }

    public static String getNoteName(int n) {
        String[] notes = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};
        int octave = (n + 8) / 12;
        int noteIdx = (n - 1) % 12;
        return notes[noteIdx] + String.valueOf(octave);
    }
}
