/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.example.plasma;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.Rect;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.os.Bundle;
import android.content.Context;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.view.Display;
import android.view.WindowManager;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.SeekBar;
import android.widget.Switch;

import static android.Manifest.permission.RECORD_AUDIO;

public class Plasma  extends AppCompatActivity
{
    // load our native library
    static {
        System.loadLibrary("plasma");
    }

    private static native void createSLEngine(int sampleRate, int framesPerBuf);
    private static native void createAudioRecorder();
    private static native void deleteAudioRecorder();
    private static native void startPlay();
    private static native void stopPlay();
    private static native void pausePlay();
    private static native void deleteSLEngine();

    private static native void renderPlasmaSetOverlap(float percentage);
    private static native void renderPlasmaDecay(float percentage);

    private static final int PERMISSION_REQUEST_CODE = 200;

    float fft_overlap = -1;
    float bars_decay = -1;

    // Called when the activity is first created.
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        if (checkSelfPermission(RECORD_AUDIO)!= PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{RECORD_AUDIO}, PERMISSION_REQUEST_CODE);
        }

        AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        int nativeSampleRate  =  Integer.parseInt(myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE));
        int  nativeSampleBufSize = Integer.parseInt(myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER));
        int recBufSize = AudioRecord.getMinBufferSize(nativeSampleRate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);

        createSLEngine(nativeSampleRate, nativeSampleBufSize*4);
        createAudioRecorder();
        startPlay();

        setContentView(R.layout.layout);
        android.support.v7.widget.Toolbar myToolbar = (android.support.v7.widget.Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(myToolbar);

        final Switch switchAB = (Switch)findViewById(R.id.start_stop_switch);
        switchAB.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                pausePlay();
             }
        });

        //load preferences
        SharedPreferences sharedPref = getPreferences(Context.MODE_PRIVATE);
        fft_overlap = sharedPref.getFloat(getString(R.string.fft_overlap), 0.5f);
        bars_decay = sharedPref.getFloat(getString(R.string.bars_decay), 0.9f);
        renderPlasmaSetOverlap(fft_overlap);
        renderPlasmaDecay(bars_decay);
    }

    @Override
    protected  void onDestroy() {
        //save preferences
        SharedPreferences sharedPref = getPreferences(Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putFloat(getString(R.string.fft_overlap), fft_overlap);
        editor.putFloat(getString(R.string.bars_decay), bars_decay);
        editor.commit();

        stopPlay();
        deleteAudioRecorder();
        deleteSLEngine();
        super.onDestroy();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case PERMISSION_REQUEST_CODE:
                if (grantResults.length > 0) {
                    boolean recordingAccepted = grantResults[0] == PackageManager.PERMISSION_GRANTED;
                }
        }
    }

                    //@Override
    public void onCreateDialog(View view) {

        LayoutInflater inflater = getLayoutInflater();
        View popupView =inflater.inflate(R.layout.settings_menu, null);

        SeekBar.OnSeekBarChangeListener listener = new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                switch(seekBar.getId()) {
                    case R.id.fft_overlap:  fft_overlap = (float) progress / 100.0f; break;
                    case R.id.bars_decay: bars_decay = (float) progress / 100.0f; break;
                }

                renderPlasmaSetOverlap(fft_overlap);
                renderPlasmaDecay(bars_decay);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        };

        SeekBar seek1 = (SeekBar)popupView.findViewById(R.id.fft_overlap);
        seek1.setProgress((int)(fft_overlap*100));
        seek1.setOnSeekBarChangeListener(listener);
        SeekBar seek2 = (SeekBar)popupView.findViewById(R.id.bars_decay);
        seek2.setProgress((int)(bars_decay*100));
        seek2.setOnSeekBarChangeListener(listener);

        boolean focusable = true;
        int width = LinearLayout.LayoutParams.MATCH_PARENT;
        int height = LinearLayout.LayoutParams.WRAP_CONTENT;
        final PopupWindow popupWindow = new PopupWindow(popupView, width, height, focusable);
        popupWindow.showAtLocation(popupView, Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL, 0, 0);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.options_menu, menu);
        return true;
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.settings:

                onCreateDialog(item.getActionView());
                return true;
        }

        return (super.onOptionsItemSelected(item));
    }

}


// Custom view for rendering plasma.
//
// Note: suppressing lint wrarning for ViewConstructor since it is
//       manually set from the activity and not used in any layout.
@SuppressLint("ViewConstructor")
class PlasmaView extends View {
    private Bitmap mBitmap = null;
    private long mStartTime;
    private Paint white, gray;
    private int barsHeight = 200;

    // implementend by libplasma.so

    private static native int renderPlasma(Bitmap  bitmap, long time_ms);
    private static native void renderPlasmaInit(int fftLength, int sampleRate, int barsHeight, float timeOverlap);
    private static native float renderPlasmaXToFreq(double x);
    private static native float renderPlasmaFreqToX(double freq);
    private static native void ConnectWithAudio();

    public PlasmaView(Context context, AttributeSet attrs) {
        super(context, attrs);

        white = new Paint();
        white.setColor(Color.WHITE);
        gray = new Paint();
        gray.setColor(Color.GRAY);
        gray.setAlpha(128+64);

        mStartTime = System.currentTimeMillis();
        int scaledSize = getResources().getDimensionPixelSize(R.dimen.font_size);
        white.setTextSize(scaledSize);
    }

    @Override
    protected void onSizeChanged (int w, int h, int oldw, int oldh)
    {
        if (w>0 && h>0)
        {
            mBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.RGB_565);
            renderPlasmaInit(1024*2*2, 48000, barsHeight, 0.1f);
            ConnectWithAudio();

        }
        else
        {
            mBitmap = null;
        }
    }

    float x=0,y=0;

    public boolean onTouchEvent(MotionEvent e) {
        x = e.getX(0);
        y = e.getY(0);
        return true;
    }

    @Override protected void onDraw(Canvas canvas) {

        if (mBitmap != null) {
                int currentRow = renderPlasma(mBitmap, System.currentTimeMillis() - mStartTime);
                if (currentRow>=0) {
                    // draw bars
                    canvas.drawBitmap(mBitmap, new Rect(0, 0, mBitmap.getWidth(), barsHeight), new Rect(0, 0, mBitmap.getWidth(), barsHeight), null);

                    // draw waterfall
                    if (currentRow > barsHeight)
                        canvas.drawBitmap(mBitmap, new Rect(0, currentRow, mBitmap.getWidth(), barsHeight + 1), new Rect(0, barsHeight, mBitmap.getWidth(), currentRow), null);
                    canvas.drawBitmap(mBitmap, new Rect(0, mBitmap.getHeight(), mBitmap.getWidth(), currentRow), new Rect(0, currentRow, mBitmap.getWidth(), mBitmap.getHeight()), null);
                }


            canvas.drawLine(0, 0, (float) canvas.getWidth(), (float) canvas.getHeight(), gray);

            String str = String.format("%d Hz", (int)renderPlasmaXToFreq(x));
            canvas.drawText(str, x, y-200, white);
            canvas.drawLine(x,0,x, (float)canvas.getHeight(), white);

            {
                float x;
                x = renderPlasmaFreqToX(10);
                canvas.drawLine(x, 0, x, (float) canvas.getHeight(), gray);
                x = renderPlasmaFreqToX(100);
                canvas.drawLine(x, 0, x, (float) canvas.getHeight(), gray);
                x = renderPlasmaFreqToX(1000);
                canvas.drawLine(x, 0, x, (float) canvas.getHeight(), gray);
                x = renderPlasmaFreqToX(10000);
                canvas.drawLine(x, 0, x, (float) canvas.getHeight(), gray);
            }
        }

        invalidate();
    }
}
