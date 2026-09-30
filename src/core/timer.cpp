#include "timer.h"

float gGameSpeed = 1.0f;

void Timer::set(Timer *self, int time)
{
    if ((self->isInitialized & 1U) == 0)
    {
        self->currentF = 0.0f;
        self->current = 0;
        self->previous = -99999;
        self->gameSpeed = &gGameSpeed;
        self->isInitialized |= 1;
    }
    self->current = time;
    self->previous = time - 1;
    self->currentF = time;
}

void Timer::addf(Timer *self, float amount)
{
    self->previous = self->current;
    float gameSpeed = *self->gameSpeed;
    if (0.99f < gameSpeed && gameSpeed < 1.01f)
    {
        // Approximation.
        self->currentF += amount;
        self->current = static_cast<int>(self->currentF);
        return;
    }
    self->currentF = gameSpeed*amount + self->currentF;
    self->current = static_cast<int>(self->currentF);
}

int Timer::increment(Timer* self)
{
    float gameSpeed = *self->gameSpeed;
    self->previous = self->current;
    if (0.99f < gameSpeed && gameSpeed < 1.01f)
    {
        // Approximation.
        self->current += 1;
        self->currentF += 1.0f;
        return self->current;
    }
    self->currentF += gameSpeed;
    self->current = static_cast<int>(self->currentF);
    return self->current;
}

int Timer::isTickMultipleOf(Timer *self, int i)
{
    if ((self->current != self->previous) && (self->current % i == 0))
    {
        return TRUE;
    }
    return FALSE;
}
