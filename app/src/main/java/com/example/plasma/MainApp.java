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
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.media.AudioManager;
import android.os.Bundle;
import android.content.Context;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.util.AttributeSet;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.view.WindowManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.Spinner;
import android.widget.Switch;

import static android.Manifest.permission.RECORD_AUDIO;

public class MainApp extends AppCompatActivity
{
    private static final int PERMISSION_REQUEST_CODE = 200;

    WaterfallView plasmaView;

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

        plasmaView = (WaterfallView)findViewById(R.id.WaterfallView);

        //load preferences
        ConfigDialog.LoadPreferences(this, plasmaView);
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
                ConfigDialog.onCreateDialog(this);
                return true;
            case R.id.measuring_tool:
                mMeasuring = !mMeasuring;
                plasmaView.SetMeasuring(mMeasuring);
                return true;
            case R.id.exit:
                finish();
                System.exit(0);
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

}
