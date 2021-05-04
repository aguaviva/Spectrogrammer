#ifndef SCALE_H
#define SCALE_H

double lerp( double t, double min, double max);
double unlerp( double min, double max, double x);
double clamp(double v, double min, double max);

class Scale {
    bool bLogarithmic = true;

    //log stuff
    double a,b, inva, invb;

    // lin stuff
    double maxIdx, minFreq, maxFreq;

public:
    void setLogarithmic(bool bLogarithmic_) { bLogarithmic = bLogarithmic_;}
    bool getLogarithmic() const { return bLogarithmic; }
    void init(double maxIdx,double minFreq, double maxFreq);
    double forward(double x) const;
    double backward(double v) const;
};


#endif