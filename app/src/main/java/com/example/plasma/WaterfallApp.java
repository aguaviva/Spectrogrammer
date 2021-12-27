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
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.media.AudioManager;
import android.os.Bundle;
import android.content.Context;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;

public class WaterfallApp extends AppCompatActivity
{
    public static final String EXTRA_MODE = "com.example.plasma.MODE";
    public static final String [] modes = { "FFT", "Music"};

    WaterfallView mWaterfallView;

    // Called when the activity is first created.
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.layout);

        Toolbar myToolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(myToolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mWaterfallView = (WaterfallView) findViewById(R.id.WaterfallView);

        Intent intent = getIntent();
        String message = intent.getStringExtra(EXTRA_MODE);
        if (message.compareTo("FFT") == 0) {
            mWaterfallView.setProcessor(WaterfallView.ProcessorMode.FFT);
        } else if (message.compareTo("Music") == 0) {
            mWaterfallView.setProcessor(WaterfallView.ProcessorMode.MUSIC);
        }
        {
            Audio.onCreate((AudioManager) getSystemService(Context.AUDIO_SERVICE));
            LoadPreferences();
            Spectrogram.ConnectWithAudioMT();
            Audio.startPlay();
        }
    }

    @Override
    protected  void onDestroy() {
        Audio.stopPlay();
        SavePreferences();
        Spectrogram.Disconnect();
        Audio.onDestroy();

        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu){
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.toolbar_options_menu, menu);
        final MenuItem logItem = menu.findItem(R.id.play_pause2);
        CheckBox cb = (CheckBox)logItem.getActionView().findViewById(R.id.action_layout_styled_checkbox);
        if (cb != null)
        {
            cb.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Audio.pausePlay();
                }
            });
        }

        return true;
    }

    boolean mMeasuring = false;
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        switch (item.getItemId()) {
            case R.id.settings:
                ShowSettingsDialog();
                return true;
            case R.id.measuring_tool:
                mMeasuring = !mMeasuring;
                mWaterfallView.SetMeasuring(mMeasuring);
                return true;
            case R.id.exit:
                finish();
                System.exit(0);
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    void SetSeekBar(View popupView, int id, int value, SeekBar.OnSeekBarChangeListener listener)
    {
        SeekBar seek = popupView.findViewById(id);
        seek.setProgress(value);
        seek.setOnSeekBarChangeListener(listener);
    }

    public void ShowSettingsDialog() {

        LayoutInflater inflater = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View popupView = inflater.inflate(R.layout.settings_menu, null);

        //-------------------------------- sliders
        SeekBar.OnSeekBarChangeListener listener = new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                switch (seekBar.getId()) {
                    case R.id.fft_overlap: Spectrogram.SetOverlap((float) progress / 100.0f); break;
                    case R.id.bars_decay: Spectrogram.SetDecay((float) progress / 100.0f); break;
                    case R.id.fft_size: Spectrogram.SetProcessorFFT(1 << (progress + 8)); break;
                    case R.id.volume: Spectrogram.SetVolume((float) progress / 100.0f); break;
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        };

        SetSeekBar(popupView, R.id.volume, (int) (Spectrogram.GetVolume() * 100), listener);
        SetSeekBar(popupView, R.id.fft_overlap, (int) (Spectrogram.GetOverlap() * 100), listener);
        SetSeekBar(popupView, R.id.bars_decay, (int) (Spectrogram.GetDecay() * 100), listener);
        if (mWaterfallView.getProcessor()==WaterfallView.ProcessorMode.FFT) {
            SetSeekBar(popupView, R.id.fft_size, (int) (Math.log(Spectrogram.GetFftLength()) / Math.log(2)) - 8, listener);
        }

        //--------------------------------  axis radio buttons
        if (mWaterfallView.getProcessor()==WaterfallView.ProcessorMode.FFT)
        {
            RadioGroup.OnCheckedChangeListener RadioButtonListener = new RadioGroup.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                    switch (checkedId) {
                        case R.id.horizontalScaleLogarithmic: mWaterfallView.setLogX(true); break;
                        case R.id.horizontalScaleLinear: mWaterfallView.setLogX(false); break;
                        case R.id.verticalScaleLogarithmic: mWaterfallView.setLogY(true); break;
                        case R.id.verticalScaleLinear: mWaterfallView.setLogY(false); break;
                    }
                }
            };

            {
                RadioGroup axisScaleGroup = (RadioGroup) popupView.findViewById(R.id.horizontalScale);
                axisScaleGroup.setOnCheckedChangeListener(RadioButtonListener);
                RadioButton radioButton = (RadioButton) popupView.findViewById(R.id.horizontalScaleLogarithmic);
                radioButton.setChecked(mWaterfallView.getLogX());
                radioButton = (RadioButton) popupView.findViewById(R.id.horizontalScaleLinear);
                radioButton.setChecked(!mWaterfallView.getLogX());
            }

            {
                RadioGroup axisScaleGroup = (RadioGroup) popupView.findViewById(R.id.verticalScale);
                axisScaleGroup.setOnCheckedChangeListener(RadioButtonListener);
                RadioButton radioButton = (RadioButton) popupView.findViewById(R.id.verticalScaleLogarithmic);
                radioButton.setChecked(mWaterfallView.getLogY());
                radioButton = (RadioButton) popupView.findViewById(R.id.verticalScaleLinear);
                radioButton.setChecked(!mWaterfallView.getLogY());
            }

            popupView.findViewById(R.id.FFT_stuff).setVisibility( View.VISIBLE);
        }
        else
        {
            popupView.findViewById(R.id.FFT_stuff).setVisibility( View.GONE);
        }

        //--------------------------------  check buttons
        View.OnClickListener CheckBoxListener = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                boolean checked = ((CheckBox) view).isChecked();
                switch (view.getId()) {
                    case R.id.debug: mWaterfallView.setShowDebugInfo(checked); break;
                }
            }
        };

        CheckBox debug = popupView.findViewById(R.id.debug);
        debug.setOnClickListener(CheckBoxListener);

        //--------------------------------
        int width = LinearLayout.LayoutParams.MATCH_PARENT;
        int height = LinearLayout.LayoutParams.WRAP_CONTENT;
        final PopupWindow popupWindow = new PopupWindow(popupView, width, height);
        popupWindow.setOutsideTouchable(true);
        popupWindow.setFocusable(true);
        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        popupWindow.showAtLocation(popupView, Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL, 0, 0);
    }

    public void LoadPreferences()
    {
        SharedPreferences sharedPref = getPreferences(Context.MODE_PRIVATE);
        Spectrogram.SetOverlap(sharedPref.getFloat(getString(R.string.fft_overlap), 0.5f));
        Spectrogram.SetDecay(sharedPref.getFloat(getString(R.string.bars_decay), 0.9f));

        if (mWaterfallView.getProcessor()==WaterfallView.ProcessorMode.FFT)
        {
            Spectrogram.SetProcessorFFT(sharedPref.getInt(getString(R.string.fft_size), 4096));
            mWaterfallView.setLogX(sharedPref.getBoolean(getString(R.string.horizontalScaleSelected), true));
            mWaterfallView.setLogY(sharedPref.getBoolean(getString(R.string.verticalScaleSelected), true));
        }
        else
        {
            Spectrogram.SetProcessorGoertzel(4096);
            mWaterfallView.setLogX(false);
            mWaterfallView.setLogY(false);
        }

        mWaterfallView.setShowDebugInfo(sharedPref.getBoolean(getString(R.string.showDebugInfo), false));
        Spectrogram.SetBarsHeight(200);
    }

    public void SavePreferences()
    {
        SharedPreferences sharedPref = getPreferences(Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();

        editor.putFloat(getString(R.string.fft_overlap), Spectrogram.GetOverlap());
        editor.putFloat(getString(R.string.bars_decay), Spectrogram.GetDecay());

        if (mWaterfallView.getProcessor()==WaterfallView.ProcessorMode.FFT)
        {
            editor.putInt(getString(R.string.fft_size), Spectrogram.GetFftLength());
            editor.putBoolean(getString(R.string.horizontalScaleSelected), mWaterfallView.getLogX());
            editor.putBoolean(getString(R.string.verticalScaleSelected), mWaterfallView.getLogY());
        }

        editor.putBoolean(getString(R.string.showDebugInfo), mWaterfallView.getShowDebugInfo());

        editor.apply();
        editor.commit();
    }

}
