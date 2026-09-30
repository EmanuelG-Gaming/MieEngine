#ifndef TIMER_H_
#define TIMER_H_ 1

#include "../base/base_defs.h"

struct Timer {
public:
    int previous;
    int current;
    float currentF;
    float* gameSpeed;
    u32 isInitialized;

    static void addf(Timer* self, float amount);
    static int increment(Timer* self);

    static void set(Timer* self, int time);
    static int isTickMultipleOf(Timer* self, int i);
};

extern float gGameSpeed;


#endif /* TIMER_H_ */
