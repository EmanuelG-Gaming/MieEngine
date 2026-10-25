#include "core/interp.h"
#include "base/math/mathf.h"

#include <stdio.h>

template <typename T>
void Interp<T>::step(Interp<T>* self)
{
    // EndTime should be positive,
    // because otherwise if it's zero, calculating 't' would be a division error.
    if (self->endTime > 0)
    {
        // Either this
        Timer::increment(&self->timer);

        float t = self->timer.currentF / (float) self->endTime;
        /*
        if (t >= 1.0f)
        {
            t = 1.0f;
        }
        */

        switch (self->method)
        {
            // Some special interpolation methods.
            case InterpMethod::linear:
            {
                self->currentVal = self->initial + self->goal;
            } break;

            case InterpMethod::physics:
            {
                // Overshoot?
                self->currentVal = self->initial + self->bezier2;
                self->bezier2 = self->bezier2 + self->goal;
            } break;

            case InterpMethod::cubicHermite:
            {
                float t = self->timer.currentF / (float) self->endTime;

                float t2 = t*t;
                float t3 = t2*t;
                float h00 = (2.0f*t3) - (3.0f*t2) + 1.0f;
                float h10 = t3 - (2.0f*t2) + t;
                float h01 = (-2.0f*t3) + (3.0f*t2);
                float h11 = t3 - t2;

                T term1 = self->initial * h00;
                T term2 = self->bezier1 * h10;
                T term3 = self->goal * h01;
                T term4 = self->bezier2 * h11;
                self->currentVal = term1+term2+term3+term4;
            } break;

            default:
            {
                float alpha = interpolate(self, t);
                T delta = self->goal - self->initial;
                T offset = delta*alpha;

                // Use some better approximations.
                // A(t) = A0 + (A1-A0)*t
                // A(t) = A0 + A1*t - A0*t.
                // A(t) = A0*(1-t) + A1*t.
                self->currentVal = self->goal*alpha + self->initial*(1 - alpha);
            } break;
        }


        if (self->timer.current >= self->endTime)
        {
            // We reset the counter once the interpolation ends.
            self->timer.set(&self->timer, self->endTime);
            self->currentVal = self->goal;
            self->endTime = 0;

            if (self->method != InterpMethod::cubicHermite)
            {
                return;
            }
        }

        // Or this.
        //Timer::increment(&self->timer);
    }
}

template <typename T>
float Interp<T>::interpolate(Interp<T>* self, float t)
{
    switch (self->method)
    {
        case InterpMethod::in2:
            return pow2(t);
        case InterpMethod::in3:
            return pow3(t);
        case InterpMethod::in4:
            return pow4(t);
        case InterpMethod::out2:
            return 1.0f - pow2(1.0f - t);
        case InterpMethod::out3:
            return 1.0f - pow3(1.0f - t);
        case InterpMethod::out4:
            return 1.0f - pow4(1.0f - t);

        case InterpMethod::inout2:
            t *= 2.0f;
            if (t < 1.0f) return pow2(t)/2.0f;
            else return (2.0f - pow2(2.0f - t)) / 2.0f;
        case InterpMethod::inout3:
            t *= 2.0f;
            if (t < 1.0f) return pow3(t)/2.0f;
            else return (2.0f - pow3(2.0f - t)) / 2.0f;
        case InterpMethod::inout4:
            t *= 2.0f;
            if (t < 1.0f) return pow4(t)/2.0f;
            else return (2.0f - pow4(2.0f - t)) / 2.0f;

        case InterpMethod::outin2:
            t *= 2.0f;
            if (t < 1.0f) return 0.5f - pow2(1.0f - t) / 2.0f;
            else return 0.5f + pow2(t - 1.0f) / 2.0f;
        case InterpMethod::outin3:
            t *= 2.0f;
            if (t < 1.0f) return 0.5f - pow3(1.0f - t) / 2.0f;
            else return 0.5f + pow3(t - 1.0f) / 2.0f;
        case InterpMethod::outin4:
            t *= 2.0f;
            if (t < 1.0f) return 0.5f - pow4(1.0f - t) / 2.0f;
            else return 0.5f + pow4(t - 1.0f) / 2.0f;

        case InterpMethod::delayed:
            return 0.0f;
        case InterpMethod::instant:
            return 1.0f;

        default: return t;
    }
}

/*
   Explicit template instantiations.
*/

template void Interp<float>::step(Interp<float>* self);
template void Interp<vec3>::step(Interp<vec3>* self);
