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
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
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
import static android.provider.Settings.Global.getString;

public class Plasma  extends AppCompatActivity
{
    private static final int PERMISSION_REQUEST_CODE = 200;

    PlasmaView plasmaView;

    // Called when the activity is first created.
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        if (checkSelfPermission(RECORD_AUDIO)!= PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{RECORD_AUDIO}, PERMISSION_REQUEST_CODE);
        }

        Audio.onCreate((AudioManager) getSystemService(Context.AUDIO_SERVICE));

        setContentView(R.layout.layout);
        android.support.v7.widget.Toolbar myToolbar = (android.support.v7.widget.Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(myToolbar);

        plasmaView = (PlasmaView)findViewById(R.id.plasmaView);

        final Switch switchAB = (Switch)findViewById(R.id.start_stop_switch);
        switchAB.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                Audio.pausePlay();
             }
        });

        //load preferences
        SharedPreferences sharedPref = getPreferences(Context.MODE_PRIVATE);
        Spectrogram.SetOverlap(sharedPref.getFloat(getString(R.string.fft_overlap), 0.5f));
        Spectrogram.SetDecay(sharedPref.getFloat(getString(R.string.bars_decay), 0.9f));
        Spectrogram.SetFftLength(sharedPref.getInt(getString(R.string.fft_size), 4096));
        Spectrogram.SetFrequencyLogarithmicAxis(sharedPref.getBoolean(getString(R.string.frequency_logarithmic_axis), true));
        Spectrogram.SetBarsHeight(200);
    }

    @Override
    protected  void onDestroy() {

        //save preferences
        SharedPreferences sharedPref = getPreferences(Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putFloat(getString(R.string.fft_overlap), Spectrogram.GetOverlap());
        editor.putFloat(getString(R.string.bars_decay), Spectrogram.GetDecay());
        editor.putInt(getString(R.string.fft_size), Spectrogram.GetFftLength());
        editor.putBoolean(getString(R.string.frequency_logarithmic_axis), Spectrogram.GetFrequencyLogarithmicAxis());
        editor.apply();
        editor.commit();

        Audio.onDestroy();

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

        //--------------------------------
        SeekBar.OnSeekBarChangeListener listener = new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                switch(seekBar.getId()) {
                    case R.id.fft_overlap:  Spectrogram.SetOverlap((float) progress / 100.0f); break;
                    case R.id.bars_decay: Spectrogram.SetDecay((float) progress / 100.0f); break;
                    case R.id.fft_size:Spectrogram.SetFftLength(1<<(progress+8));break;
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        };

        SeekBar seek1 = (SeekBar)popupView.findViewById(R.id.fft_overlap);
        seek1.setProgress((int)(Spectrogram.GetOverlap()*100));
        seek1.setOnSeekBarChangeListener(listener);
        SeekBar seek2 = (SeekBar)popupView.findViewById(R.id.bars_decay);
        seek2.setProgress((int)(Spectrogram.GetDecay()*100));
        seek2.setOnSeekBarChangeListener(listener);
        SeekBar seek3 = (SeekBar)popupView.findViewById(R.id.fft_size);
        seek3.setProgress((int)(Math.log(Spectrogram.GetFftLength()) / Math.log(2))-8);
        seek3.setOnSeekBarChangeListener(listener);

        //--------------------------------
        Switch.OnCheckedChangeListener SwitchListener = new Switch.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                Spectrogram.SetFrequencyLogarithmicAxis(isChecked);
            }
        };
        Switch onOffSwitch = (Switch)popupView.findViewById(R.id.x_axis_scale_logarithmic);
        onOffSwitch.setOnCheckedChangeListener(SwitchListener);
        onOffSwitch.setChecked(Spectrogram.GetFrequencyLogarithmicAxis());

        //--------------------------------
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
// Note: suppressing lint warning for ViewConstructor since it is
//       manually set from the activity and not used in any layout.
@SuppressLint("ViewConstructor")
class PlasmaView extends View {

    Spectrogram spectrogram;

    private Bitmap mBitmap = null;
    private long mStartTime;
    private Paint white, gray;
    private int barsHeight = 200;
    Rect bars;

    Viewport viewport = new Viewport();

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

        viewport.Init(this);
    }

    @Override
    protected void onSizeChanged (int w, int h, int oldw, int oldh)
    {
        if (w>0 && h>0)
        {
            mBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.RGB_565);
            bars = new Rect(0, 0, mBitmap.getWidth(), barsHeight);
            Spectrogram.ConnectWithAudio();
        }
        else
        {
            mBitmap = null;
        }
    }

    float x=0,y=0;

    public boolean onTouchEvent(MotionEvent event)
    {
        //  no other handlers, then drag & scale
        boolean b = viewport.onTouchEvent(event);
        if (b == false)
        {
            x = event.getX(0);
            y = event.getY(0);
        }

        return true;
    }


    @Override
    protected void onDraw(Canvas canvas) {
        float height = (float) getHeight();
        float width = (float) getWidth();

        canvas.save();
        canvas.translate(viewport.GetPosX(), 0);
        canvas.scale(viewport.GetScaleX(), 1);

        if (mBitmap != null)
        {
            int currentRow = Spectrogram.Update(mBitmap, System.currentTimeMillis() - mStartTime);
            if (currentRow>=0) {
                // draw bars
                canvas.drawBitmap(mBitmap, bars, bars, null);

                // draw waterfall
                if (currentRow > barsHeight)
                    canvas.drawBitmap(mBitmap, new Rect(0, currentRow, mBitmap.getWidth(), barsHeight + 1), new Rect(0, barsHeight, mBitmap.getWidth(), currentRow), null);
                canvas.drawBitmap(mBitmap, new Rect(0, mBitmap.getHeight(), mBitmap.getWidth(), currentRow), new Rect(0, currentRow, mBitmap.getWidth(), mBitmap.getHeight()), null);
            }

            String str = String.format("%d Hz", (int)Spectrogram.XToFreq(x));
            canvas.drawText(str, x, y-200, white);
            canvas.drawLine(x,0,x, height, white);

            // draw time graphs
            float delta = (48000.0f/(Spectrogram.GetFftLength()*Spectrogram.GetOverlap()));
            for(int i=0;i<50;i++)
            {
                float yy = y - i*delta;
                canvas.drawLine(x-20, yy, x+20, yy, white);
            }

            // draw freq marks
            {
                float x;
                x = Spectrogram.FreqToX(10);
                canvas.drawLine(x, 0, x, height, gray);
                x = Spectrogram.FreqToX(100);
                canvas.drawLine(x, 0, x, height, gray);
                x = Spectrogram.FreqToX(1000);
                canvas.drawLine(x, 0, x, height, gray);
                x = Spectrogram.FreqToX(10000);
                canvas.drawLine(x, 0, x, height, gray);
            }
        }

        invalidate();
    }
}
