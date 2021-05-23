package com.example.plasma;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class WaterfallView extends View {

    public enum HorizontalScale {
        LINEAR,
        LOGARITHMIC,
        PIANO,
        GUITAR;

        public static HorizontalScale toMyEnum (String myEnumString) {
            try {
                return valueOf(myEnumString);
            } catch (Exception ex) {
                // For error cases
                assert(false);
                return LINEAR;
            }
        }
    }

    Spectrogram spectrogram;

    private Bitmap mBitmap = null;
    private long mStartTime;
    private Paint white, gray, drawPaint;
    private int barsHeight = 200;
    private boolean mLogX = false;
    private boolean mLogY = true;
    private HorizontalScale mHorizontalScale = HorizontalScale.LINEAR;
    Rect bars;

    int delay = 0;

    Viewport viewport = new Viewport();

    public WaterfallView(Context context, AttributeSet attrs) {
        super(context, attrs);

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

        mStartTime = System.currentTimeMillis();

        viewport.Init(this);
    }

    public void setHorizontalScale(HorizontalScale b) { mHorizontalScale = b; updateScaler(); }
    public HorizontalScale getHorizontalScale() { return mHorizontalScale; }
    public void setLogY(boolean b) { mLogY = b; updateScaler(); }
    public boolean getLogY() { return mLogY; }

    public void updateScaler() {
        if (mBitmap!=null) {
            switch(mHorizontalScale) {
                case LINEAR:
                    Spectrogram.SetProcessor(0);
                    Spectrogram.SetScaler(mBitmap.getWidth(), 100, 48000 / 2, false, mLogY);
                    break;
                case LOGARITHMIC:
                    Spectrogram.SetProcessor(0);
                    Spectrogram.SetScaler(mBitmap.getWidth(), 100, 48000 / 2, true, mLogY);
                    break;
                case PIANO:
                    Spectrogram.SetProcessor(1);
                    Spectrogram.SetScaler(mBitmap.getWidth(), 1, 88, false, false);
                    break;
                case GUITAR:
                    Spectrogram.SetProcessor(1);
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
            bars = new Rect(0, 0, mBitmap.getWidth(), barsHeight);
            updateScaler();
            Spectrogram.ConnectWithAudioMT(mBitmap);
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
        }
        else {
            viewport.onTouchEvent(event);
        }
        return true;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        float height = (float) getHeight();
        float width = (float) getWidth();

        canvas.save();
        canvas.translate(viewport.GetPos().x, 0);
        canvas.scale(viewport.GetScale().x, 1);

        if (mBitmap != null)
        {
            int currentRow = Spectrogram.Lock(mBitmap);
            //int currentRow = Spectrogram.Update(mBitmap, System.currentTimeMillis() - mStartTime);
            if (currentRow>=0) {
                // draw bars
                canvas.drawBitmap(mBitmap, bars, bars, drawPaint);

                // draw waterfall
                int topHalf = (barsHeight + 1) + mBitmap.getHeight() - currentRow;
                canvas.drawBitmap(mBitmap, new Rect(0, currentRow, mBitmap.getWidth(), mBitmap.getHeight()), new Rect(0, barsHeight + 1, mBitmap.getWidth(), topHalf), null);
                canvas.drawBitmap(mBitmap, new Rect(0, barsHeight + 1, mBitmap.getWidth(), currentRow), new Rect(0, topHalf, mBitmap.getWidth(), mBitmap.getHeight()), null);
            }
            Spectrogram.Unlock(mBitmap);
        }
        canvas.restore();

        {
            int dp = Spectrogram.GetDroppedFrames();
            if (dp>0)
                delay = 10;
            if (delay>0) {
                canvas.drawText("Overload!! Dropping audio frames", 10, 250, white);
                delay--;
            }
        }

        if (mHorizontalScale == HorizontalScale.LINEAR)
        {
            for (int i = 0; i < 48000/2; i+=1000) {
                float x = Spectrogram.FreqToX(i);
                x = viewport.toScreenSpace(x);
                canvas.drawLine(x, 0, x, height, gray);
            }
        }
        else if (mHorizontalScale == HorizontalScale.LOGARITHMIC)
        {
            float d = 1;
            for (int j=0;j<5;j++) {
                for (int i = 0; i < 10; i++) {
                    float x;
                    x = Spectrogram.FreqToX(i*d);
                    x = viewport.toScreenSpace(x);
                    canvas.drawLine(x, 0, x, height, gray);
                }
                d*=10;
            }
        }
        else if (mHorizontalScale == HorizontalScale.PIANO)
        {
            Rect bounds = new Rect();
            for (int n = 4; n < 88; n+=12) {
                float x = Spectrogram.FreqToX(n);
                float x1 = Spectrogram.FreqToX(n+1);

                x = viewport.toScreenSpace(x);
                x1 = viewport.toScreenSpace(x1);

                canvas.drawLine(x, 0, x, height, gray);

                String text = getNoteName(n);
                white.getTextBounds(text, 0, text.length(), bounds);
                if ((x1 - x) > bounds.width())
                    canvas.drawText(text, x, 10 + bounds.height(), white);
            }
        }
        else if (mHorizontalScale == HorizontalScale.GUITAR)
        {
            Rect bounds = new Rect();
            int [] notes = {20, 25, 30, 35, 39, 44};
            for (int i = 0; i < notes.length; i++) {

                float x = Spectrogram.FreqToX(notes[i]);
                x = viewport.toScreenSpace(x);
                canvas.drawLine(x, 0, x, height, gray);

                float x1 = Spectrogram.FreqToX(notes[i]+1);
                x1 = viewport.toScreenSpace(x1);
                canvas.drawLine(x1, 0, x1, height,
                        gray);


                String text = getNoteName(notes[i]);
                white.getTextBounds(text, 0, text.length(), bounds);
                canvas.drawText(text, x, 10 + bounds.height(), white);
            }
        }

        // Draw
        if (mMeasuring)
        {
            float xx = viewport.fromScreenSpace(x);
            String str = "none";
            switch(mHorizontalScale)
            {
                case LINEAR:
                case LOGARITHMIC:
                    str = String.format("%d Hz", (int)Spectrogram.XToFreq(xx)); break;
                case PIANO:
                case GUITAR:
                    str = String.format("%s", getNoteName((int)Spectrogram.XToFreq(xx))); break;
            }


            canvas.drawText(str, x, y-200, white);
            canvas.drawLine(x,0,x, height, white);

            // draw time graphs
            float delta = (48000.0f/(Spectrogram.GetFftLength()*Spectrogram.GetOverlap()));
            for(int i=0;i<50;i++)
            {
                float yy = y - i*delta;
                canvas.drawLine(x-20, yy, x+20, yy, white);
            }

        }


        viewport.EnforceMinimumSize();

        invalidate();
    }

    public static String getNoteName(int n) {
        String[] notes = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};
        int octave = (n + 8) / 12;
        int noteIdx = (n - 1) % 12;
        return notes[noteIdx] + String.valueOf(octave);
    }
}
