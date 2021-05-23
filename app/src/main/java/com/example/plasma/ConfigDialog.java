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

    static private WaterfallView mWaterfallView;

    static public void onCreateDialog(Context context) {

        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View popupView = inflater.inflate(R.layout.settings_menu, null);

        //--------------------------------
        SeekBar.OnSeekBarChangeListener listener = new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                switch (seekBar.getId()) {
                    case R.id.fft_overlap:
                        Spectrogram.SetOverlap((float) progress / 100.0f);
                        break;
                    case R.id.bars_decay:
                        Spectrogram.SetDecay((float) progress / 100.0f);
                        break;
                    case R.id.fft_size:
                        Spectrogram.SetFftLength(1 << (progress + 8));
                        break;
                    case R.id.volume:
                        Spectrogram.SetVolume((float) progress / 100.0f);
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        };

        SeekBar seek4 = (SeekBar) popupView.findViewById(R.id.volume);
        seek4.setProgress((int) (Spectrogram.GetVolume() * 100));
        seek4.setOnSeekBarChangeListener(listener);
        SeekBar seek1 = (SeekBar) popupView.findViewById(R.id.fft_overlap);
        seek1.setProgress((int) (Spectrogram.GetOverlap() * 100));
        seek1.setOnSeekBarChangeListener(listener);
        SeekBar seek2 = (SeekBar) popupView.findViewById(R.id.bars_decay);
        seek2.setProgress((int) (Spectrogram.GetDecay() * 100));
        seek2.setOnSeekBarChangeListener(listener);
        SeekBar seek3 = (SeekBar) popupView.findViewById(R.id.fft_size);
        seek3.setProgress((int) (Math.log(Spectrogram.GetFftLength()) / Math.log(2)) - 8);
        seek3.setOnSeekBarChangeListener(listener);

        //--------------------------------
        RadioGroup.OnCheckedChangeListener RadioButtonListener = new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                View radioButton = radioGroup.findViewById(checkedId);
                int index = radioGroup.indexOfChild(radioButton);

                switch (checkedId) {
                    case R.id.horizontalScaleLogarithmic: mWaterfallView.setHorizontalScale(WaterfallView.HorizontalScale.LOGARITHMIC); break;
                    case R.id.horizontalScaleLinear: mWaterfallView.setHorizontalScale(WaterfallView.HorizontalScale.LINEAR); break;
                    case R.id.horizontalScalePiano:mWaterfallView.setHorizontalScale(WaterfallView.HorizontalScale.PIANO); break;
                    case R.id.horizontalScaleGuitar:mWaterfallView.setHorizontalScale(WaterfallView.HorizontalScale.GUITAR); break;
                    case R.id.verticalScaleLogarithmic: mWaterfallView.setLogY(true); break;
                    case R.id.verticalScaleLinear: mWaterfallView.setLogY(false); break;
                }
            }
        };
        {
            RadioGroup axisScaleGroup = (RadioGroup) popupView.findViewById(R.id.horizontalScale);
            int index = axisScaleGroup.getCheckedRadioButtonId();
            axisScaleGroup.setOnCheckedChangeListener(RadioButtonListener);
            int state = -1;
            switch (mWaterfallView.getHorizontalScale())
            {
                case LOGARITHMIC: state = R.id.horizontalScaleLogarithmic; break;
                case LINEAR: state = R.id.horizontalScaleLinear; break;
                case PIANO: state = R.id.horizontalScalePiano; break;
                case GUITAR: state = R.id.horizontalScaleGuitar; break;
                default: assert(false);
            }
            RadioButton radioButton = (RadioButton)popupView.findViewById(state);
            radioButton.setChecked(true);
        }
        {
            RadioGroup axisScaleGroup = (RadioGroup) popupView.findViewById(R.id.verticalScale);
            int index = axisScaleGroup.getCheckedRadioButtonId();
            axisScaleGroup.setOnCheckedChangeListener(RadioButtonListener);
            RadioButton radioButton = (RadioButton)popupView.findViewById(R.id.verticalScaleLogarithmic);
            radioButton.setChecked(mWaterfallView.getLogY());
            radioButton = (RadioButton)popupView.findViewById(R.id.verticalScaleLinear);
            radioButton.setChecked(!mWaterfallView.getLogY());
        }

        //RadioButton bh = (RadioButton)popupView.findViewById(R.id.horizontalScale);
        //

        //--------------------------------
        int width = LinearLayout.LayoutParams.MATCH_PARENT;
        int height = LinearLayout.LayoutParams.WRAP_CONTENT;
        final PopupWindow popupWindow = new PopupWindow(popupView, width, height);
        popupWindow.setOutsideTouchable(true);
        popupWindow.setFocusable(true);
        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        popupWindow.showAtLocation(popupView, Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL, 0, 0);
    }

    static public void LoadPreferences(AppCompatActivity app, WaterfallView waterfallView)
    {
        mWaterfallView = waterfallView;

        //load preferences
        SharedPreferences sharedPref = app.getPreferences(Context.MODE_PRIVATE);
        Spectrogram.SetOverlap(sharedPref.getFloat(app.getString(R.string.fft_overlap), 0.5f));
        Spectrogram.SetDecay(sharedPref.getFloat(app.getString(R.string.bars_decay), 0.9f));
        Spectrogram.SetFftLength(sharedPref.getInt(app.getString(R.string.fft_size), 4096));
        mWaterfallView.setHorizontalScale(WaterfallView.HorizontalScale.toMyEnum(sharedPref.getString(app.getString(R.string.horizontalScaleSelected), WaterfallView.HorizontalScale.LOGARITHMIC.toString())));
        mWaterfallView.setLogY(sharedPref.getBoolean(app.getString(R.string.verticalScaleSelected), true));
        Spectrogram.SetBarsHeight(200);
    }

    static public void SavePreferences(AppCompatActivity app)
    {
        SharedPreferences sharedPref = app.getPreferences(Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putFloat(app.getString(R.string.fft_overlap), Spectrogram.GetOverlap());
        editor.putFloat(app.getString(R.string.bars_decay), Spectrogram.GetDecay());
        editor.putInt(app.getString(R.string.fft_size), Spectrogram.GetFftLength());
        editor.putString(app.getString(R.string.horizontalScaleSelected), mWaterfallView.getHorizontalScale().toString());
        editor.putBoolean(app.getString(R.string.verticalScaleSelected), mWaterfallView.getLogY());
        editor.apply();
        editor.commit();
    }


}
