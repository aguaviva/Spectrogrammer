#ifndef SCALE_H
#define SCALE_H

float lerp( float t, float min, float max);
float unlerp( float min, float max, float x);
float clamp(float v, float min, float max);

class Scale {
    bool bLogarithmic = true;

    //log stuff
    float a,b, inva, invb;

    // lin stuff
    float maxIdx, minFreq, maxFreq;

public:
    void setLogarithmic(bool bLogarithmic_) { bLogarithmic = bLogarithmic_;}
    bool getLogarithmic() const { return bLogarithmic; }
    void init(float maxIdx,float minFreq, float maxFreq);
    float forward(float x) const;
    float backward(float v) const;
};


#endif