package com.example.plasma;

import android.content.Context;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;

public class Viewport {

    private View view = null;
    private float PosX = 0;
    private float ScaleX = 1;
    private GestureDetector mGestureDetector;
    private ScaleGestureDetector mScaleGestureDetector;

    void Init(View view_)
    {
        view = view_;
        Context context = view.getContext();
        mScaleGestureDetector = new ScaleGestureDetector(context, mScaleGestureListener);
        mGestureDetector = new GestureDetector(context, mGestureDetectorListener);
    }

    float GetPosX() { return PosX; };
    float GetScaleX() { return ScaleX; };

    public boolean onTouchEvent(MotionEvent event)
    {
        return false;
        //boolean b1 =  mScaleGestureDetector.onTouchEvent(event);
        //boolean b2 = mGestureDetector.onTouchEvent(event);
        //return  b1 || b2;
    }

    // The scale listener, used for handling multi-finger scale gestures.
    //
    private final ScaleGestureDetector.OnScaleGestureListener mScaleGestureListener = new ScaleGestureDetector.SimpleOnScaleGestureListener()
    {
        @Override
        public boolean onScale(ScaleGestureDetector detector) {
            float scale = detector.getScaleFactor();
            float currSpanX = detector.getCurrentSpanX();
            float currSpanY = detector.getCurrentSpanY();

            float oldScaleFactorX = ScaleX;
            ScaleX *= scale*scale;

            float dx = detector.getFocusX()- PosX;
            float dxSc = dx * ScaleX / oldScaleFactorX;

            PosX = detector.getFocusX() - dxSc;

            view.invalidate();
            return true;
        }
    };

    private final GestureDetector.SimpleOnGestureListener mGestureDetectorListener = new GestureDetector.SimpleOnGestureListener()
    {
        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            PosX -= distanceX;
            view.invalidate();
            return true;
        }
    };
}
