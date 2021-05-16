package com.example.plasma;

import android.content.Context;
import android.graphics.PointF;
import android.graphics.RectF;
import android.util.SparseArray;
import android.view.MotionEvent;
import android.view.View;

public class Viewport {

    private SparseArray<PointF> mActivePointers = new SparseArray<PointF>();
    ;
    PointF lastPoint = new PointF();
    private PointF lastDist = new PointF();

    private View mView = null;
    private PointF mTranslate = new PointF();
    private PointF mScale  = new PointF(1,1);

    private PointF mGnome = new PointF(0,0);

    RectF mRect = new RectF();

    void Init(View view_) {
        mView = view_;
        Context context = mView.getContext();
    }

    PointF GetPos() {
        return mTranslate;
    }

    PointF GetScale() {
        return mScale;
    }

    PointF getDistance(MotionEvent event)
    {
        //distance
        float dx = event.getX(0)-event.getX(1);
        float dy = event.getY(0)-event.getY(1);

        return new PointF(dx,dy);
    }

    float toScreenSpace(float x)
    {
        return x * GetScale().x + GetPos().x;
    }

    float fromScreenSpace(float x)
    {
        return (x - GetPos().x) / GetScale().x;
    }


    public boolean onTouchEvent(MotionEvent event)
    {
        // get pointer index from the event object
        int pointerIndex = event.getActionIndex();

        // get pointer ID
        int pointerId = event.getPointerId(pointerIndex);

        // get masked (not specific to a pointer) action
        int maskedAction = event.getActionMasked();

        switch (maskedAction) {

            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN: {
                // We have a new pointer. Lets add it to the list of pointers

                PointF f = new PointF();
                f.x = event.getX(pointerIndex);
                f.y = event.getY(pointerIndex);
                mActivePointers.put(pointerId, f);

                if (mActivePointers.size()==2) {
                    // average
                    PointF midPoint = new PointF();
                    for (int i = 0; i < 2; i++) {
                        PointF point = mActivePointers.get(event.getPointerId(i));
                        if (point != null) {
                            midPoint.x += event.getX(i) / 2.0f;
                            midPoint.y += event.getY(i) / 2.0f;
                        }
                    }
                    lastPoint = midPoint;

                    //distance
                    lastDist = getDistance(event);
                }
                break;
            }

            case MotionEvent.ACTION_MOVE: { // a pointer was moved

                if (mActivePointers.size()==2)
                {
                    // scale factor
                    //
                    PointF dist = getDistance(event);

                    // midpoint
                    //
                    PointF midPoint = new PointF();
                    for (int i = 0; i < 2; i++) {
                        PointF point = mActivePointers.get(event.getPointerId(i));
                        if (point != null)
                        {
                            point.x = event.getX(i);
                            point.y = event.getY(i);
                        }

                        midPoint.x += point.x / 2.0f;
                        midPoint.y += point.y / 2.0f;
                    }

                    //-----------------------------

                    DragScale(midPoint, dist);
                }


                break;
            }

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_CANCEL: {
                mActivePointers.remove(pointerId);
                break;
            }
        }
        return true;
    }

    private void DragScale(PointF focus, PointF distance)
    {
        // compute scale
        float scaleX = distance.x/lastDist.x;
        float scaleY = distance.y/lastDist.y;
        lastDist.x = distance.x;
        lastDist.y = distance.y;

        scaleX = (float)Math.max(scaleX,0.5f);

        // compute translation
        float VelX= focus.x - lastPoint.x;
        float VelY= focus.y - lastPoint.y;
        lastPoint.x = focus.x;
        lastPoint.y = focus.y;

        // apply scaling

        mTranslate.x = (mTranslate.x -  focus.x)*scaleX + focus.x;
        mTranslate.x += VelX;

        mScale.x *= scaleX;
    }

    private void setPos(float left, float right)
    {
        mTranslate.x = left;
        mScale.x = (right-left)/mView.getWidth();
    }

    public void EnforceMinimumSize()
    {
        float left = toScreenSpace(0);
        float right = toScreenSpace(mView.getWidth());

        if (left>0)
            left +=  (0 - left)/2;

        if (right<mView.getWidth())
            right +=  (mView.getWidth() - right)/2;

        setPos(left, right);
    }

}
