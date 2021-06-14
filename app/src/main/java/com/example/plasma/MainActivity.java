package com.example.plasma;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

import static android.Manifest.permission.RECORD_AUDIO;
import static android.Manifest.permission.RECORD_AUDIO;

public class MainActivity extends AppCompatActivity {
    private static final int PERMISSION_REQUEST_CODE = 200;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)!= PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{RECORD_AUDIO}, PERMISSION_REQUEST_CODE);
        }

        View.OnClickListener buttonListener = new View.OnClickListener() {
            public void onClick(View v) {
                switch(v.getId())
                {
                    case R.id.fft_activity: {
                        Intent intent = new Intent(MainActivity.this, WaterfallApp.class);
                        intent.putExtra(WaterfallApp.EXTRA_MODE, "FFT");
                        startActivity(intent);
                        break;
                    }

                    case R.id.music_activity: {
                        Intent intent = new Intent(MainActivity.this, WaterfallApp.class);
                        intent.putExtra(WaterfallApp.EXTRA_MODE, "Music");
                        startActivity(intent);
                        break;
                    }

                }
            }
        };

        Button fft_activity = (Button)findViewById(R.id.fft_activity);
        fft_activity.setOnClickListener(buttonListener);
        Button music_activity = (Button)findViewById(R.id.music_activity);
        music_activity.setOnClickListener(buttonListener);
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