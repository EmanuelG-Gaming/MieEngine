#ifndef INTERP_H_
#define INTERP_H_ 1

#include "timer.h"

enum class InterpMethod {
    in2,
    in3,
    in4,
    out2,
    out3,
    out4,
    linear,
    cubicHermite,

    inout2,
    inout3,
    inout4,

    outin2,
    outin3,
    outin4,

    delayed,
    instant,
    physics,
};

template <typename T>
struct Interp {
    T initial;
    T goal;
    T bezier1;
    T bezier2;
    Timer timer;
    int endTime;
    InterpMethod method;

    constexpr float pow2(float t) { return t*t; }
    constexpr float pow3(float t) { return t*t*t; }
    constexpr float pow4(float t) { return t*t*t*t; }

    T step(Interp<T>* self);
    float interpolate(Interp<T>* self);
};





#endif /* INTERP_H_ */
