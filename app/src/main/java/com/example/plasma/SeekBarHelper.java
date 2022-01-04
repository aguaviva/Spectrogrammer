package com.example.plasma;

import android.content.SharedPreferences;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

public class SeekBarHelper
{
    interface Listener
    {
        public void setValue(int v);
        public int getValue();
        public String getString(int v);
    }

    private Listener m_listener;
    private int m_defaultValue;
    private String m_string;

    public SeekBarHelper(String string, int defaultValue, Listener listener)
    {
        m_string = string;
        m_defaultValue = defaultValue;
        this.m_listener = listener;
    }

    public void Init(View popupView, int SeekBarId, int TextViewId)
    {
        SeekBar seek = popupView.findViewById(SeekBarId);
        final TextView textView = popupView.findViewById(TextViewId);

        seek.setOnSeekBarChangeListener( new SeekBar.OnSeekBarChangeListener()
        {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
            {
                m_listener.setValue(progress);
                textView.setText(m_string + "   " + m_listener.getString(progress));
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        int v = m_listener.getValue();
        seek.setProgress(v);
        textView.setText(m_string+ "   " + m_listener.getString(v));
    }

    public void savePreference(SharedPreferences.Editor editor)
    {
        editor.putInt(m_string, m_listener.getValue());
    }

    public void loadPreference(SharedPreferences sharedPref)
    {
        m_listener.setValue(sharedPref.getInt(m_string, m_defaultValue));
    }

}
