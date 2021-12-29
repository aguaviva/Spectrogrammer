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


import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.media.AudioManager;
import android.os.Bundle;
import android.content.Context;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.Spinner;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class WaterfallApp extends AppCompatActivity
{
    public static final String EXTRA_MODE = "com.example.plasma.MODE";
    public static final String [] modes = { "FFT", "Music"};

    SeekBarHelper m_Volume;
    SeekBarHelper m_FFTOverlap;
    SeekBarHelper m_BarsDecay;
    SeekBarHelper m_AverageCount;
    SeekBarHelper m_FFTSize;

    WaterfallView mWaterfallView;

    View popupView;

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

        // inflate settings preferences now to avoid hiccups
        LayoutInflater inflater = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        popupView = inflater.inflate(R.layout.settings_menu, null);

        // init preferences

        m_Volume = new SeekBarHelper(getString(R.string.volume), 100, new SeekBarHelper.Listener() {
            @Override
            public void setValue(int v) {
                Spectrogram.SetVolume( (float) v /100.0f);
            }

            @Override
            public int getValue() {
                return (int)(Spectrogram.GetVolume() * 100);
            }
        });

        m_FFTOverlap = new SeekBarHelper(getString(R.string.fft_overlap), 50, new SeekBarHelper.Listener() {
            @Override
            public void setValue(int v) {
                Spectrogram.SetOverlap((float) v / 100.0f);
            }

            @Override
            public int getValue() {
                return (int)(Spectrogram.GetOverlap() * 100);
            }
        });

        m_BarsDecay = new SeekBarHelper(getString(R.string.bars_decay), 50, new SeekBarHelper.Listener() {
            @Override
            public void setValue(int v) {
                Spectrogram.SetDecay((float) v / 100.0f);
            }

            @Override
            public int getValue() {
                return (int)(Spectrogram.GetDecay() * 100);
            }
        });

        m_AverageCount = new SeekBarHelper(getString(R.string.set_average_count), 1, new SeekBarHelper.Listener() {
            @Override
            public void setValue(int v) {
                Spectrogram.SetAverageCount(v);
            }

            @Override
            public int getValue() {
                return Spectrogram.GetAverageCount();
            }
        });

        m_FFTSize = new SeekBarHelper(getString(R.string.fft_size), 4 /*4096*/, new SeekBarHelper.Listener() {
            @Override
            public void setValue(int v) {
                Spectrogram.SetProcessorFFT(1 << (v + 8));
            }

            @Override
            public int getValue() {
                return (int) (Math.log(Spectrogram.GetFftLength()) / Math.log(2)) - 8;
            }
        });

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
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    public void ShowSettingsDialog() {

        //-------------------------------- sliders
        m_Volume.Init(popupView, R.id.volume);
        m_FFTOverlap.Init(popupView, R.id.fft_overlap);
        m_BarsDecay.Init(popupView, R.id.bars_decay);

        if (mWaterfallView.getProcessor()==WaterfallView.ProcessorMode.FFT) {
            m_AverageCount.Init(popupView, R.id.average_count);
            m_FFTSize.Init(popupView, R.id.fft_size);
        }

        //--------------------------------  axis radio buttons
        if (mWaterfallView.getProcessor()==WaterfallView.ProcessorMode.FFT)
        {
            String[] plants = new String[]{
                    "Log",
                    "Linear"
            };

            final List<String> plantsList = new ArrayList<>(Arrays.asList(plants));

            // Initializing an ArrayAdapter
            final ArrayAdapter<String> spinnerArrayAdapter = new ArrayAdapter<String>(this,R.layout.spinner_item,plantsList);
            spinnerArrayAdapter.setDropDownViewResource(R.layout.spinner_item);

            AdapterView.OnItemSelectedListener ItemSelectedListener = new AdapterView.OnItemSelectedListener() {
                public void onItemSelected(AdapterView<?> parent, View view, int position, long id)
                {
                    switch (parent.getId())
                    {
                        case R.id.x_axis:
                            if (position==0)
                            {
                                mWaterfallView.setLogX(true);
                            }
                            else
                            {
                                mWaterfallView.setLogX(false);
                            }
                            break;
                        case R.id.y_axis:
                            if (position==0)
                            {
                                mWaterfallView.setLogY(true);
                            }
                            else
                            {
                                mWaterfallView.setLogY(false);
                            }
                            break;
                    }
                }
                public void onNothingSelected(AdapterView<?> parent)
                {

                }
            };

            Spinner spinnerX = (Spinner) popupView.findViewById(R.id.x_axis);
            spinnerX.setAdapter(spinnerArrayAdapter);
            spinnerX.setOnItemSelectedListener(ItemSelectedListener);
            spinnerX.setSelection(mWaterfallView.getLogX()?0:1);

            Spinner spinnerY = (Spinner) popupView.findViewById(R.id.y_axis);
            spinnerY.setAdapter(spinnerArrayAdapter);
            spinnerY.setOnItemSelectedListener(ItemSelectedListener);
            spinnerY.setSelection(mWaterfallView.getLogY()?0:1);

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

        m_FFTOverlap.loadPreference(sharedPref);
        m_Volume.loadPreference(sharedPref);
        m_BarsDecay.loadPreference(sharedPref);

        if (mWaterfallView.getProcessor()==WaterfallView.ProcessorMode.FFT)
        {
            m_AverageCount.loadPreference(sharedPref);
            m_FFTSize.loadPreference(sharedPref);
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

        m_FFTOverlap.savePreference(editor);
        m_Volume.savePreference(editor);
        m_BarsDecay.savePreference(editor);

        if (mWaterfallView.getProcessor()==WaterfallView.ProcessorMode.FFT)
        {
            m_AverageCount.savePreference(editor);
            m_FFTSize.savePreference(editor);
            editor.putBoolean(getString(R.string.horizontalScaleSelected), mWaterfallView.getLogX());
            editor.putBoolean(getString(R.string.verticalScaleSelected), mWaterfallView.getLogY());
        }

        editor.putBoolean(getString(R.string.showDebugInfo), mWaterfallView.getShowDebugInfo());

        editor.apply();
        editor.commit();
    }

}
