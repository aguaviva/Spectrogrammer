package com.example.plasma;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;

import androidx.appcompat.app.AppCompatActivity;

public class ConfigDialog {

    static private int horizontalScaleSelectedId;
    static private WaterfallView mWaterfallView;

    static public void onCreateDialog(Context context) {

        LayoutInflater inflater = (LayoutInflater) context.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
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
        RadioGroup.OnCheckedChangeListener RadioButtonListener = new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                SetHorizontalAxis(checkedId);
            }
        };

        RadioGroup horizontalScaleGroup = (RadioGroup)popupView.findViewById(R.id.horizontalScale);
        int index = horizontalScaleGroup.getCheckedRadioButtonId();
        horizontalScaleGroup.setOnCheckedChangeListener(RadioButtonListener);
        RadioButton b = (RadioButton)popupView.findViewById(horizontalScaleSelectedId);
        b.setChecked(true);

        //--------------------------------
        int width = LinearLayout.LayoutParams.MATCH_PARENT;
        int height = LinearLayout.LayoutParams.WRAP_CONTENT;
        final PopupWindow popupWindow = new PopupWindow(popupView, width, height);
        popupWindow.setOutsideTouchable(true);
        popupWindow.setFocusable(true);
        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        popupWindow.showAtLocation(popupView, Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL, 0, 0);
    }

    static private void SetHorizontalAxis(int id)
    {
        switch(id)
        {
            case R.id.horizontalScaleLinear:   mWaterfallView.setLogX(false); break;
            case R.id.horizontalScaleLogarithmic:mWaterfallView.setLogX(true); break;
            case R.id.horizontalScalePiano:mWaterfallView.setLogX(true); break;
            case R.id.horizontalScaleGuitar:mWaterfallView.setLogX(true); break;
        }
        horizontalScaleSelectedId = id;
    }

    static public int GetHorizontalAxis()
    {
        return horizontalScaleSelectedId;
    }

    static public void LoadPreferences(AppCompatActivity app, WaterfallView waterfallView)
    {
        mWaterfallView = waterfallView;

        //load preferences
        SharedPreferences sharedPref = app.getPreferences(Context.MODE_PRIVATE);
        Spectrogram.SetOverlap(sharedPref.getFloat(app.getString(R.string.fft_overlap), 0.5f));
        Spectrogram.SetDecay(sharedPref.getFloat(app.getString(R.string.bars_decay), 0.9f));
        Spectrogram.SetFftLength(sharedPref.getInt(app.getString(R.string.fft_size), 4096));
        SetHorizontalAxis(sharedPref.getInt(app.getString(R.string.horizontalScaleSelectedId), R.id.horizontalScaleLogarithmic));
        Spectrogram.SetBarsHeight(200);
    }

    static public void SavePreferences(AppCompatActivity app)
    {
        SharedPreferences sharedPref = app.getPreferences(Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putFloat(app.getString(R.string.fft_overlap), Spectrogram.GetOverlap());
        editor.putFloat(app.getString(R.string.bars_decay), Spectrogram.GetDecay());
        editor.putInt(app.getString(R.string.fft_size), Spectrogram.GetFftLength());
        editor.putInt(app.getString(R.string.horizontalScaleSelectedId), horizontalScaleSelectedId);
        editor.apply();
        editor.commit();
    }


}
