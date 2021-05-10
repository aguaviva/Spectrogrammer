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

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.os.Bundle;
import android.content.Context;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.provider.MediaStore;
import android.util.AttributeSet;
import android.util.Log;
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
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RadioButton;
import android.widget.RadioGroup;
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

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)!= PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{RECORD_AUDIO}, PERMISSION_REQUEST_CODE);
        }

        Audio.onCreate((AudioManager) getSystemService(Context.AUDIO_SERVICE));

        setContentView(R.layout.layout);
        Toolbar myToolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(myToolbar);

        plasmaView = (PlasmaView)findViewById(R.id.plasmaView);

        final Switch switchAB = (Switch)findViewById(R.id.start_stop_switch);
        switchAB.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                Audio.pausePlay();
             }
        });

        final CheckBox measure = (CheckBox)findViewById(R.id.measureCheckBox);
        measure.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                plasmaView.SetMeasuring(isChecked);
            }
        });


        final Button preferencesButton = (Button)findViewById(R.id.preferencesButton);
        final AppCompatActivity app = this;
        preferencesButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                ConfigDialog.onCreateDialog(app);
            }
        });
        //load preferences
        ConfigDialog.LoadPreferences(this);
    }

    @Override
    protected  void onDestroy() {

        ConfigDialog.SavePreferences(this);

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
        int scaledSize = getResources().getDimensionPixelSize(R.dimen.font_size);
        white.setTextSize(scaledSize);

        gray = new Paint();
        gray.setColor(Color.GRAY);
        gray.setAlpha(128 + 64);

        mStartTime = System.currentTimeMillis();

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
            int currentRow = Spectrogram.Update(mBitmap, System.currentTimeMillis() - mStartTime);
            if (currentRow>=0) {
                // draw bars
                canvas.drawBitmap(mBitmap, bars, bars, null);

                // draw waterfall
                int topHalf = (barsHeight + 1) + mBitmap.getHeight() - currentRow;
                canvas.drawBitmap(mBitmap, new Rect(0, currentRow, mBitmap.getWidth(), mBitmap.getHeight()), new Rect(0, barsHeight + 1, mBitmap.getWidth(), topHalf), null);
                canvas.drawBitmap(mBitmap, new Rect(0, barsHeight + 1, mBitmap.getWidth(), currentRow), new Rect(0, topHalf, mBitmap.getWidth(), mBitmap.getHeight()), null);
            }

        }
        canvas.restore();

        // Draw
        if (mMeasuring)
        {
            float xx = viewport.fromScreenSpace(x);
            String str = String.format("%d Hz", (int)Spectrogram.XToFreq(xx));
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



        if (ConfigDialog.GetHorizontalAxis()==R.id.horizontalScaleLinear)
        {
            for (int i = 0; i < 48000/2; i+=1000) {
                float x = Spectrogram.FreqToX(i);
                x = viewport.toScreenSpace(x);
                canvas.drawLine(x, 0, x, height, gray);
            }
        }
        else if (ConfigDialog.GetHorizontalAxis()==R.id.horizontalScaleLogarithmic)
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
        else if (ConfigDialog.GetHorizontalAxis()==R.id.horizontalScalePiano)
        {
            Rect bounds = new Rect();
            for (int n = 1; n < 88; n++) {
                float x = Spectrogram.FreqToX(Math.pow(2.0, (n - 49) / 12.0) * 440.0);
                float x1 = Spectrogram.FreqToX(Math.pow(2.0, ((n + 1) - 49) / 12.0) * 440.0);

                x = viewport.toScreenSpace(x);
                x1 = viewport.toScreenSpace(x1);

                canvas.drawLine(x, 0, x, height, gray);

                String text = getNoteName(n);
                white.getTextBounds(text, 0, text.length(), bounds);
                if ((x1 - x) > bounds.width())
                    canvas.drawText(text, x, 10 + bounds.height(), white);
            }
        }
        else if (ConfigDialog.GetHorizontalAxis()==R.id.horizontalScaleGuitar)
        {
            Rect bounds = new Rect();
            int [] notes = {20, 25, 30, 35, 39, 44};
            for (int i = 1; i < notes.length; i++) {
                float x = Spectrogram.FreqToX(Math.pow(2.0, (notes[i] - 49) / 12.0) * 440.0);
                x = viewport.toScreenSpace(x);
                canvas.drawLine(x, 0, x, height, gray);
                String text = getNoteName(notes[i]);
                white.getTextBounds(text, 0, text.length(), bounds);
                canvas.drawText(text, x, 10 + bounds.height(), white);
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
