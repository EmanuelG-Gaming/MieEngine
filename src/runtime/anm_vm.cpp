#include "anm_vm.h"
#include "../base/math/base_rng.h"
#include "anm_vars.h"
#include "anm_manager.h"

#include <string.h>
#include <math.h>

#include <stdio.h>


static AnmVM defaultParent;
static int defaultParentInit = 0;

AnmVM::AnmVM(void)
{
    memset(this, 0, sizeof(AnmVM));
    this->spriteNumber = -1;
}

AnmVM::~AnmVM(void)
{
    // TODO: Currently does nothing.
}

void AnmVM::init(AnmVM* self)
{
    self->drawMode = ANM_DRAW_RECT;
    self->colorMode = COLOR_LR;
    self->rotationMode = ANM_ROT_XYZ;

    self->rotation = { 0, 0, 0 };
    self->scale = { 1.0f, 1.0f };
    self->spriteSize = { 0.1f, 0.1f };
    self->uvScrollPos = { 0.0f, 0.0f };
    self->scrollVel = { 0.0f, 0.0f };
    self->offsetPos = { 0.0f, 0.0f, 0.0f };

    self->scaleGrowth = { 0.0f, 0.0f };
    self->angularVelocity = { 0.0f, 0.0f, 0.0f };

    self->radius1 = 0.5f;
    self->radius2 = 1.0f;
    self->thickness = -0.1f;
    self->nPoints = 3;


    self->pendingInterrupt = 0;
    self->interruptReturnInstr = NULL;

    setColor(self, 1, 1, 1, 1);
    setColor2(self, 1, 1, 1, 1);

    Mat4Ident(&self->matrix, 1.0f);

    // Interpolations.
    self->timeInScript.current = 0;
    self->timeInScript.currentF = 0.0f;

    self->posInterp.endTime = 0;
    self->rotationInterp.endTime = 0;
    self->scaleInterp.endTime = 0;

    self->rgbInterp.endTime = 0;
    self->alphaInterp.endTime = 0;

    self->rgb2Interp.endTime = 0;
    self->alpha2Interp.endTime = 0;

    self->flags = 0;
}


int AnmVM::getIntVar(AnmVM *self, int id)
{
    switch (id)
    {
        case ANM_I1:
            return self->intVars[0];
        case ANM_I2:
            return self->intVars[1];
        case ANM_I3:
            return self->intVars[2];
        case ANM_I4:
            return self->intVars[3];

        case ANM_F1:
            return static_cast<int> (self->floatVars[0]);
        case ANM_F2:
            return static_cast<int> (self->floatVars[1]);
        case ANM_F3:
            return static_cast<int> (self->floatVars[2]);
        case ANM_F4:
            return static_cast<int> (self->floatVars[3]);

        case ANM_IRAND:
            // Get RNG number.
            return RNG_boundedRand(2100000);

        default:
            return id;
    }
}

int* AnmVM::getIntVarPtr(AnmVM *self, int *id)
{
    switch (*id)
    {
        case ANM_I1:
            return &self->intVars[0];
        case ANM_I2:
            return &self->intVars[1];
        case ANM_I3:
            return &self->intVars[2];
        case ANM_I4:
            return &self->intVars[3];

        case ANM_F1:
            return reinterpret_cast<int*> (&self->floatVars[0]);
        case ANM_F2:
            return reinterpret_cast<int*> (&self->floatVars[1]);
        case ANM_F3:
            return reinterpret_cast<int*> (&self->floatVars[2]);
        case ANM_F4:
            return reinterpret_cast<int*> (&self->floatVars[3]);

        default:
            fprintf(stderr, "UNREACHEABLE INT VAR\n");
            return id;
    }
}

float AnmVM::getFloatVar(AnmVM* self, float id)
{
    int roundedId = static_cast<int> (id);
    //int index = roundedId - 10000;

    switch (static_cast<int>(roundedId))
    {
        case ANM_I1:
            return static_cast<float> (self->intVars[0]);
        case ANM_I2:
            return static_cast<float> (self->intVars[1]);
        case ANM_I3:
            return static_cast<float> (self->intVars[2]);
        case ANM_I4:
            return static_cast<float> (self->intVars[3]);

        case ANM_F1:
            return self->floatVars[0];
        case ANM_F2:
            return self->floatVars[1];
        case ANM_F3:
            return self->floatVars[2];
        case ANM_F4:
            return self->floatVars[3];

        case ANM_POSX:
            return self->pos.x;
        case ANM_POSY:
            return self->pos.y;
        case ANM_POSZ:
            return self->pos.z;

        // Random number.
        case ANM_FRAND:
            return RNG_randf32_range(-1.0f, 1.0f);

        default:
            return id;
    }
}

float* AnmVM::getFloatVarPtr(AnmVM *self, float* id)
{
    switch (static_cast<int>(*id))
    {
        case ANM_F1:
            return &self->floatVars[0];
        case ANM_F2:
            return &self->floatVars[1];
        case ANM_F3:
            return &self->floatVars[2];
        case ANM_F4:
            return &self->floatVars[3];

        case ANM_POSX:
            return &self->pos.x;
        case ANM_POSY:
            return &self->pos.y;
        case ANM_POSZ:
            return &self->pos.z;

        default:
            return id;
    }
}

void AnmVM::chooseRotations(AnmVM *self, float* rx, float* ry, float* rz)
{
    static int rotPerms[(ANM_ROT_ZYX+1)*3] = {
        0, 1, 2,
        1, 0, 2,
        1, 2, 0,
        0, 2, 1,
        2, 0, 1,
        2, 1, 0,
    };

    int idx = self->rotationMode * 3;

    *rx = *(&self->rotation.x + rotPerms[idx+0]);
    *ry = *(&self->rotation.x + rotPerms[idx+1]);
    *rz = *(&self->rotation.x + rotPerms[idx+2]);
}

float AnmVM::wrapAngleSum(float f1, float f2)
{
    float sum = f1 + f2;
    int count = 0;

    // Handle positive overflow (sum > PI).
    if (sum > PI)
    {
        do
        {
            sum -= PI2;
            count++;
            if (count > 32)
            {
                break;
            }
        } while (sum > PI);
    }

    // If we are already above -PI, we are done.
    if (sum >= -PI)
    {
        return sum;
    }

    // Handle negative overflow (sum < -PI).
    do
    {
        sum += PI2;
        if (count > 32)
        {
            return sum;
        }
        count++;
    } while (sum < -PI);

    return sum;
}


void AnmVM::setColor(AnmVM *self, float r, float g, float b, float a)
{
    self->color1[0] = r;
    self->color1[1] = g;
    self->color1[2] = b;
    self->color1[3] = a;
}
void AnmVM::setColor2(AnmVM *self, float r, float g, float b, float a)
{
    self->color2[0] = r;
    self->color2[1] = g;
    self->color2[2] = b;
    self->color2[3] = a;
}


void AnmVM::loadIntoAnmVM(AnmVM *self, AnmLoaded *anmLoaded, int scriptNumber)
{
    int anmIndex;
    AnmVM_rawInstr* instr;

    if ((anmLoaded->spriteData[scriptNumber]) && !anmLoaded->anmsLoading)
    {
        init(self);
        self->scriptNumber = (uint16_t) scriptNumber;
        anmIndex = anmLoaded->anmSlotIndex;
        self->flags = self->flags & 0xfffff9ff;
        self->anmFileIndex = (uint16_t) anmIndex;
        self->anmLoaded = anmLoaded;
        instr = (AnmVM_rawInstr *) &anmLoaded->spriteData[scriptNumber];
        self->startOfScript = instr;
        self->currentInstr = instr;

        self->timeInScript.set(&self->timeInScript, 0);
        self->flags &= 0xfffffffe;

        ++g_anmManager->allocatedVmCountMaybe;
        return;
    }

    _MEMSET(self, 0, sizeof(AnmVM));
    return;
}

void AnmVM::loadSingleAnmScript(AnmVM *self, AnmLoaded *anmLoaded, uint32_t scriptNumber)
{
    if (!anmLoaded->spriteData[scriptNumber] || anmLoaded->anmsLoading)
    {
        _MEMSET(self, 0, sizeof(AnmVM));
        return;
    }

    self->scriptNumber = scriptNumber;
    self->flags = 0xfffff9ff;
    self->anmFileIndex = anmLoaded->anmSlotIndex;
    self->anmLoaded = anmLoaded;
    AnmVM_rawInstr* startInstr = (AnmVM_rawInstr *) &anmLoaded->spriteData[scriptNumber];
    self->startOfScript = startInstr;
    self->currentInstr = startInstr;
    self->timeInScript.set(&self->timeInScript, 0);
    self->flags &= ~1;

    ++g_anmManager->allocatedVmCountMaybe;
}

void AnmVM::loadScript(AnmVM* self, AnmVM_rawInstr* instr)
{
    self->startOfScript = instr;
    self->currentInstr = self->startOfScript;
    self->drawMode = ANM_DRAW_RECT;

    /*
    fprintf(stderr, "%s: Current instr: %p\n", __func__, self->currentInstr);

    //self->init(self);

    fprintf(stderr, "REGDUMP:\n");
    for (int i = 0; i < 4; ++i)
    {
        fprintf(stderr, "intVars[%d] = %d\n", i, self->intVars[i]);
    }
    for (int i = 0; i < 4; ++i)
    {
        fprintf(stderr, "floatVars[%d] = %f\n", i, self->floatVars[i]);
    }
    */
}


void AnmVM::update(AnmVM* self)
{
    // UV scrolling?

    float t = self->posInterp.timer.currentF / (float) self->posInterp.endTime;

    // Interpolating.
    //self->pos.x = (self->posInterp.goal.x - self->posInterp.initial.x) * t + self->posInterp.initial.x;
    //self->pos.y = (self->posInterp.goal.y - self->posInterp.initial.y) * t + self->posInterp.initial.y;
    //self->pos.z = (self->posInterp.goal.z - self->posInterp.initial.z) * t + self->posInterp.initial.z;

    self->posInterp.step(&self->posInterp);

    self->scale.x += self->scaleGrowth.x;
    self->scale.y += self->scaleGrowth.y;

    self->rotation.x += self->angularVelocity.x;
    self->rotation.y += self->angularVelocity.y;
    self->rotation.z += self->angularVelocity.z;

    //fprintf(stderr, "scaleGrowth x:%f y:%f\n", self->scaleGrowth.x, self->scaleGrowth.y);

    //fprintf(stderr, "scmonkeys\n");
    if (0)
    {
        fprintf(stderr, "INTEPOLATION DEBUG!\n");
        fprintf(stderr, "t: %f\n", t);
        fprintf(stderr, "posInterpGoal(%f, %f, %f)\n", self->posInterp.goal.x, self->posInterp.goal.y, self->posInterp.goal.z);
        fprintf(stderr, "posInterpInitial(%f, %f, %f)\n", self->posInterp.initial.x, self->posInterp.initial.y, self->posInterp.initial.z);
        fprintf(stderr, "pos(%f, %f, %f)\n", self->pos.x, self->pos.y, self->pos.z);
        fprintf(stderr, "\n");
    }

}

void AnmVM::draw(AnmVM* self, AnmVM* parent)
{
    if (self->drawMode == ANM_DRAW_NONE)
    {
        return;
    }

    if (parent == NULL)
    {
        // TODO: this might be an issue, maybe?
        if (!defaultParentInit)
        {
            defaultParent = AnmVM{};
            defaultParent.init(&defaultParent);

            defaultParentInit = TRUE;
        }

        return draw(self, &defaultParent);
    }

    float totalX = (self->entityPos.x + self->pos.x);
    float totalY = (self->entityPos.y + self->pos.y);
    float totalZ = (self->entityPos.z + self->pos.z);

    float ptx = parent->entityPos.x + parent->pos.x;
    float pty = parent->entityPos.y + parent->pos.y;
    float ptz = parent->entityPos.z + parent->pos.z;


    // Update.
    float c1[4], c2[4];
    c1[0] = self->color1[0] * parent->color1[0];
    c1[1] = self->color1[1] * parent->color1[1];
    c1[2] = self->color1[2] * parent->color1[2];
    c1[3] = self->color1[3] * parent->color1[3];

    c2[0] = self->color2[0] * parent->color2[0];
    c2[1] = self->color2[1] * parent->color2[1];
    c2[2] = self->color2[2] * parent->color2[2];
    c2[3] = self->color2[3] * parent->color2[3];

    DrawColorMode(self->colorMode);
    DrawColor(c1[0], c1[1], c1[2], c1[3]);
    DrawColor2(c2[0], c2[1], c2[2], c2[3]);

    float rot[3];
    chooseRotations(parent, &rot[0], &rot[1], &rot[2]);

    DrawMatTranslate3D(ptx, pty, ptz);
    DrawMatRotate3D(rot[0], rot[1], rot[2]);
    DrawMatScale3D(parent->scale.x, parent->scale.y, 1.0f);


    chooseRotations(self, &rot[0], &rot[1], &rot[2]);

    DrawMatTranslate3D(totalX, totalY, totalZ);
    DrawMatRotate3D(rot[0], rot[1], rot[2]);
    DrawMulMatrix(&self->matrix);

    switch (self->drawMode)
    {
        case ANM_DRAW_RECT:
        {
            if (self->flags & ANM_FLAG_LINE_THROUGH)
            {
                // Do nothing here.


            }
            else if (self->flags & ANM_FLAG_LINE)
            {
                DrawMatScale3D(self->spriteSize.x*self->scale.x, self->spriteSize.y*self->scale.y, 1.0f);
                DrawArcSector(4, 0, PI2, self->radius1, self->radius2);
            }
            else
            {
                DrawRect(self->spriteSize.x*self->scale.x, self->spriteSize.y*self->scale.y);
            }
        } break;

        case ANM_DRAW_ELLIPSE:
        {
            if (self->flags & ANM_FLAG_LINE_THROUGH)
            {
                // Do nothing here.


            }
            else if (self->flags & ANM_FLAG_LINE)
            {
                DrawMatScale3D(self->spriteSize.x*self->scale.x, self->spriteSize.y*self->scale.y, 1.0f);
                DrawArcSector(self->nPoints, 0, PI2, self->radius1, self->radius2);
            }
            else
            {
                DrawEllipse(self->nPoints, self->spriteSize.x*self->scale.x*self->radius1, self->spriteSize.y*self->scale.y*self->radius1);
            }
        } break;


        case ANM_DRAW_STAR:
        {
            puts("to implement draw star");
            if (self->flags & ANM_FLAG_LINE_THROUGH)
            {

            }
            else if (self->flags & ANM_FLAG_LINE)
            {
            }
            else
            {
            }
        } break;

        default:
        {
        } break;
    }

    // Reset matrix transformations.
    DrawMatIdentity();
    DrawColor(1, 1, 1, 1);
    DrawColor2(1, 1, 1, 1);
    DrawColorMode(COLOR_LR);
}


/*
   NOTE: This is compatible with Touhou 11: Subterranean Animism's ANM VM implementation,
   at least for the basic operations.
   Also credits to Touhou 7 decompilation.
*/

void AnmVM::run(AnmVM* self)
{
    while (self->currentInstr != NULL)
    {
        float gameSpeed = gGameSpeed;
        gGameSpeed = 1.0f;

        AnmVM_rawInstr* instrInterrupt = NULL;
        uint32_t opcode = self->currentInstr->opcode;

        if (self->pendingInterrupt != 0)
        {
            goto interrupt;
        }

        if (self->currentInstr->time > self->timeInScript.current)
        {
            // Skip instruction if it's greater than time.
            return;
        }


        switch (opcode)
        {
            case ANM_NOP: // nop
            {
            } break;

            // Destroys the VM.
            case ANM_DESTROY: // destroy()
            {
                self->flags &= ~1;
                self->currentInstr = NULL;
                gGameSpeed = gameSpeed;
            } break;

            // Freezes the graphics until it is destroyed.
            case ANM_FREEZE: // freeze()
            {
                self->currentInstr = NULL;
                gGameSpeed = gameSpeed;
            } break;

            // Sets up a sprite.
            case ANM_SPRITE: // sprite(int id)
            {
                int spriteNumber = self->getIntArg(0);
                self->flags |= 1;

                // Draw a certain sprite here.
            } break;

            case ANM_JMP: // jmp(int dest, int t)
            {
                int dest = self->currentInstr->args[0].i;
                int t = self->currentInstr->args[1].i;
                self->timeInScript.set(&self->timeInScript, t);
                self->jumpToInstr(dest);
                continue;
            } break;

            // Decrement count and then jump if count < 0.
            // You can use this to repeat a loop a fixed amount of times.
            case ANM_JMP_DEC: // jmpDec(int dest, int t)
            {
                int* count = self->getIntArgPtr(0);
                int dest = self->getIntArg(1);
                int t = self->getIntArg(2);
                int originalCount = *count;

                *count -= 1;

                if (originalCount > 0)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;

            // Does a = b.
            case ANM_ISET: // iset(int& a, int b)
            {
                int src = self->getIntArg(1);
                int* dest = self->getIntArgPtr(0);

                *dest = src;
            } break;

            // Does a = b.
            case ANM_FSET: // fset(float& a, float b)
            {
                float src = self->getFloatArg(1);
                float* dest = self->getFloatArgPtr(0);

                *dest = src;
            }

            // Does a += b.
            case ANM_IADD: // iadd(int& a, int b)
            {
                int* a = self->getIntArgPtr(0);
                int b = self->getIntArg(1);
                *a += b;
            } break;

            // Does a += b.
            case ANM_FADD: // fadd(float& a, float b)
            {
                float* a = self->getFloatArgPtr(0);
                float b = self->getFloatArg(1);
                *a += b;
            } break;


            // Does a -= b.
            case ANM_ISUB: // isub(int& a, int b)
            {
                int* a = self->getIntArgPtr(0);
                int b = self->getIntArg(1);
                *a -= b;
            } break;

            // Does a -= b.
            case ANM_FSUB: // fsub(float& a, float b)
            {
                float* a = self->getFloatArgPtr(0);
                float b = self->getFloatArg(1);
                *a -= b;
            } break;

            // Does a *= b.
            case ANM_IMUL: // imul(int& a, int b)
            {
                int* a = self->getIntArgPtr(0);
                int b = self->getIntArg(1);
                *a *= b;
            } break;

            // Does a *= b.
            case ANM_FMUL: // fmul(float& a, float b)
            {
                float* a = self->getFloatArgPtr(0);
                float b = self->getFloatArg(1);
                *a *= b;
            } break;

            // Does a /= b.
            case ANM_IDIV: // idiv(int& a, int b)
            {
                int* a = self->getIntArgPtr(0);
                int b = self->getIntArg(1);
                *a /= b;
            } break;

            // Does a /= b.
            case ANM_FDIV: // fdiv(float& a, float b)
            {
                float* a = self->getFloatArgPtr(0);
                float b = self->getFloatArg(1);
                *a /= b;
            } break;

            // Does a %= b.
            case ANM_IMOD: // imod(int& a, int b)
            {
                int* a = self->getIntArgPtr(0);
                int b = self->getIntArg(1);
                *a %= b;
            } break;

            // Does a &= b.
            case ANM_FMOD: // fmod(float& a, float b)
            {
                float* a = self->getFloatArgPtr(0);
                float b = self->getFloatArg(1);
                *a = fmod(*a, b);
            } break;

            // Does a = b+c.
            case ANM_ISETADD: // isetadd(int& a, int b, int c)
            {
                int* a = self->getIntArgPtr(0);
                int b = self->getIntArg(1);
                int c = self->getIntArg(2);

                *a = b + c;
            } break;

            // Does a = b+c.
            case ANM_FSETADD: // fsetadd(float& a, float b, float c)
            {
                float* a = self->getFloatArgPtr(0);
                float b = self->getFloatArg(1);
                float c = self->getFloatArg(2);
                *a = b + c;
            } break;


            // Does a = b-c.
            case ANM_ISETSUB: // isetsub(int& a, int b, int c)
            {
                int* a = self->getIntArgPtr(0);
                int b = self->getIntArg(1);
                int c = self->getIntArg(2);
                *a = b - c;
            } break;

            // Does a = b-c.
            case ANM_FSETSUB: // fsetsub(float& a, float b, float c)
            {
                float* a = self->getFloatArgPtr(0);
                float b = self->getFloatArg(1);
                float c = self->getFloatArg(2);
                *a = b - c;
            } break;

            // Does a = b*c.
            case ANM_ISETMUL: // isetmul(int& a, int b, int c)
            {
                int* a = self->getIntArgPtr(0);
                int b = self->getIntArg(1);
                int c = self->getIntArg(2);
                *a = b * c;
            } break;

            // Does a = b*c.
            case ANM_FSETMUL: // fsetmul(float& a, float b, float c)
            {
                float* a = self->getFloatArgPtr(0);
                float b = self->getFloatArg(1);
                float c = self->getFloatArg(2);
                *a = b * c;
            } break;

            // Does a = b/c.
            case ANM_ISETDIV: // isetdiv(int& a, int b, int c)
            {
                int* a = self->getIntArgPtr(0);
                int b = self->getIntArg(1);
                int c = self->getIntArg(2);
                *a = b / c;
            } break;

            // Does a = b/c.
            case ANM_FSETDIV: // fsetdiv(float& a, float b, float c)
            {
                float* a = self->getFloatArgPtr(0);
                float b = self->getFloatArg(1);
                float c = self->getFloatArg(2);
                *a = b / c;
            } break;

            // Does a = b%c.
            case ANM_ISETMOD: // isetmod(int& a, int b, int c)
            {
                int* a = self->getIntArgPtr(0);
                int b = self->getIntArg(1);
                int c = self->getIntArg(2);
                *a = b % c;
            } break;

            // Does a = b%c.
            case ANM_FSETMOD: // fsetmod(float& a, float b, float c)
            {
                float* a = self->getFloatArgPtr(0);
                float b = self->getFloatArg(1);
                float c = self->getFloatArg(2);
                *a = fmod(b, c);
            } break;

            // Jumps if a == b.
            case ANM_IJE: // ije(int a, int b, int dest, int t)
            {
                int a = self->getIntArg(0);
                int b = self->getIntArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a == b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;

            // Jumps if a == b.
            case ANM_FJE: // fje(float a, float b, int dest, int t)
            {
                float a = self->getFloatArg(0);
                float b = self->getFloatArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a == b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;

            // Jumps if a != b.
            case ANM_IJNE: // ijne(int a, int b, int dest, int t)
            {
                int a = self->getIntArg(0);
                int b = self->getIntArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a != b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;

            // Jumps if a != b.
            case ANM_FJNE: // fjne(float a, float b, int dest, int t)
            {
                float a = self->getFloatArg(0);
                float b = self->getFloatArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a != b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;

            // Jumps if a < b.
            case ANM_IJL: // ijl(int a, int b, int dest, int t)
            {
                int a = self->getIntArg(0);
                int b = self->getIntArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a < b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;

            // Jumps if a < b.
            case ANM_FJL: // fjl(float a, float b, int dest, int t)
            {
                float a = self->getFloatArg(0);
                float b = self->getFloatArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a < b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;


            // Jumps if a <= b.
            case ANM_IJLE: // ijle(int a, int b, int dest, int t)
            {
                int a = self->getIntArg(0);
                int b = self->getIntArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a <= b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;

            // Jumps if a <= b.
            case ANM_FJLE: // fjle(float a, float b, int dest, int t)
            {
                float a = self->getFloatArg(0);
                float b = self->getFloatArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a <= b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;



            // Jumps if a > b.
            case ANM_IJG: // ijg(int a, int b, int dest, int t)
            {
                int a = self->getIntArg(0);
                int b = self->getIntArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a > b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;

            // Jumps if a > b.
            case ANM_FJG: // fjg(float a, float b, int dest, int t)
            {
                float a = self->getFloatArg(0);
                float b = self->getFloatArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a > b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;


            // Jumps if a >= b.
            case ANM_IJGE: // ijge(int a, int b, int dest, int t)
            {
                int a = self->getIntArg(0);
                int b = self->getIntArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a >= b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;

            // Jumps if a >= b.
            case ANM_FJGE: // fjge(float a, float b, int dest, int t)
            {
                float a = self->getFloatArg(0);
                float b = self->getFloatArg(1);
                int dest = self->getIntArg(2);
                int t = self->getIntArg(3);

                if (a >= b)
                {
                    self->timeInScript.set(&self->timeInScript, t);
                    self->jumpToInstr(dest);
                    continue;
                }
            } break;

            // Draw a random integer x in [0, n).
            case ANM_ISETRAND: // isetRand(int& x, int n)
            {
                int* x = self->getIntArgPtr(0);
                int n = self->getIntArg(0);

                *x = RNG_boundedRand(n);
            } break;

            // Draw a random float x int [0, r].
            case ANM_FSETRAND: // fsetRand(float& x, float r)
            {
                float* x = self->getFloatArgPtr(0);
                float r = self->getFloatArg(1);

                *x = RNG_randf32_range(0, r);
            } break;


            // Compute sin theta in radians.
            case ANM_FSIN: // fsin(float& dest, float theta)
            {
                float* dest = self->getFloatArgPtr(0);
                float theta = self->getFloatArg(1);

                *dest = sinf(theta);
            } break;

            // Compute cos theta in radians.
            case ANM_FCOS: // fcos(float& dest, float theta)
            {
                float* dest = self->getFloatArgPtr(0);
                float theta = self->getFloatArg(1);

                *dest = cosf(theta);
            } break;

            // Compute tan theta in radians.
            case ANM_FTAN: // ftan(float& dest, float theta)
            {
                float* dest = self->getFloatArgPtr(0);
                float theta = self->getFloatArg(1);

                *dest = tanf(theta);
            } break;

            // Compute acos theta in radians.
            case ANM_FACOS: // facos(float& dest, float theta)
            {
                float* dest = self->getFloatArgPtr(0);
                float theta = self->getFloatArg(1);

                *dest = acosf(theta);
            } break;

            // Compute atan theta in radians.
            case ANM_FATAN: // fatan(float& dest, float theta)
            {
                float* dest = self->getFloatArgPtr(0);
                float theta = self->getFloatArg(1);

                *dest = atanf(theta);
            } break;

            // Reduce an angle modulo 2*PI into the range [-PI, +PI].
            case ANM_WRAPANGLE: // wrapAngle(theta)
            {
                float* theta = self->getFloatArgPtr(0);
                *theta = wrapAngleSum(*theta, 0.0f);
            } break;

            // Sets the position of the graphics.
            case ANM_POS: // pos(float x, float y, float z)
            {
                float px = self->getFloatArg(0);
                float py = self->getFloatArg(1);
                float pz = self->getFloatArg(2);

                if ((self->flags & 0x100) == 0) {
                    self->pos = { px, py, pz };
                } else {
                    self->offsetPos = { px, py, pz };
                }
            } break;

            // Sets the graphics' rotation. For 2D objects, only the Z axis rotation
            // should've been used.
            case ANM_ROTATE: // rotate(float rx, float ry, float rz)
            {
                float rx = self->getFloatArg(0);
                float ry = self->getFloatArg(1);
                float rz = self->getFloatArg(2);

                //self->flags |= 4;
                self->rotation = { rx, ry, rz };
            } break;

            case ANM_SCALE: // scale(float sx, float sy)
            {
                float sx = self->getFloatArg(0);
                float sy = self->getFloatArg(1);

                //self->flags |= 8;
                self->scale = { sx, sy };
            } break;

            case ANM_ALPHA: // alpha(int alpha)
            {
                int alpha = self->getIntArg(0);
                self->color1[3] = alpha / 255.0f;
            } break;

            case ANM_COLOR: // color(int r, int g, int b)
            {
                int r = self->getIntArg(0);
                int g = self->getIntArg(1);
                int b = self->getIntArg(2);

                self->color1[0] = r / 255.0f;
                self->color1[1] = g / 255.0f;
                self->color1[2] = b / 255.0f;
            } break;

            // Set a constant angular velocity, in rads per frame.
            case ANM_ANGLE_VEL: // angleVel(float x, float y, float z)
            {
                float x = self->getFloatArg(0);
                float y = self->getFloatArg(1);
                float z = self->getFloatArg(2);

                self->angularVelocity = { x, y, z };
                //self->flags |= 4;
            } break;

            case ANM_SCALE_GROWTH: //scaleGrowth(float gx, float gy)
            {
                float gx = self->getFloatArg(0);
                float gy = self->getFloatArg(1);
                self->scaleGrowth = { gx, gy };
            } break;

            // This one here is used like a wrapper function with more assumed (default) parameters.
            case ANM_ALPHATIME_LINEAR: // alphaTimeLinear(int alpha, int t)
            {
                int alpha = self->getIntArg(0);
                int t = self->getIntArg(0);

                self->alphaInterp.endTime = 1;
                self->alphaInterp.method = InterpMethod::in2;
                self->alphaInterp.initial = self->color1[3];
                self->alphaInterp.bezier1 = 0;
                self->alphaInterp.bezier2 = 0;
                self->alphaInterp.goal = alpha / 255.0f;
                self->alphaInterp.timer.set(&self->alphaInterp.timer, 0);
            } break;

            case ANM_POSTIME: // posTime(int t, int mode, float x, float y, float z)
            {
                int t = self->getIntArg(0);
                int mode = self->getIntArg(1);
                float x = self->getFloatArg(2);
                float y = self->getFloatArg(3);
                float z = self->getFloatArg(4);

                uint32_t flagsLow = self->flags;
                self->posInterp.endTime = t;
                self->posInterp.method = static_cast<InterpMethod>(mode);

                if ((flagsLow & 0x100) == 0)
                {
                    // Initial position that we want to interpolate with.
                    self->posInterp.initial = self->pos;
                    fprintf(stderr, "posTime: holy shit (global pos)\n");
                }
                else
                {
                    self->posInterp.initial = self->offsetPos;
                    fprintf(stderr, "posTime: no. (offset pos)\n");
                }

                self->posInterp.goal = { x, y, z };
                fprintf(stderr, "This is the goal: (%f %f %f)\n", x, y, z);


                self->posInterp.timer.set(&self->posInterp.timer, 0);
            } break;

            // Over the next t frames, it changes color to a given value using interp mode.
            case ANM_COLORTIME1: // colorTime1(int t, int mode, int r, int g, int b)
            {
                int t = self->getIntArg(0);
                int mode = self->getIntArg(1);
                int r = self->getIntArg(2);
                int g = self->getIntArg(3);
                int b = self->getIntArg(4);

                self->rgbInterp.endTime = t;
                self->rgbInterp.method = static_cast<InterpMethod>(mode);

                self->rgbInterp.goal = { r / 255.0f, g / 255.0f, b / 255.0f };
                self->rgbInterp.timer.set(&self->rgbInterp.timer, 0);
            } break;

            case ANM_ALPHATIME1: // alphaTime1(int t, int mode, int a)
            {
                int t = self->getIntArg(0);
                int mode = self->getIntArg(1);
                int a = self->getIntArg(2);

                self->alphaInterp.endTime = t;
                self->alphaInterp.method = static_cast<InterpMethod>(mode);

                self->alphaInterp.goal = { a / 255.0f };
                self->alphaInterp.timer.set(&self->alphaInterp.timer, 0);
            } break;

            case ANM_COLORTIME2: // colorTime2(int t, int mode, int r, int g, int b)
            {
                int t = self->getIntArg(0);
                int mode = self->getIntArg(1);
                int r = self->getIntArg(2);
                int g = self->getIntArg(3);
                int b = self->getIntArg(4);

                self->rgb2Interp.endTime = t;
                self->rgb2Interp.method = static_cast<InterpMethod>(mode);

                self->rgb2Interp.goal = { r / 255.0f, g / 255.0f, b / 255.0f };
                self->rgb2Interp.timer.set(&self->rgb2Interp.timer, 0);
            } break;

            case ANM_ALPHATIME2: // alphaTime2(int t, int mode, int a)
            {
                int t = self->getIntArg(0);
                int mode = self->getIntArg(1);
                int a = self->getIntArg(2);

                self->alpha2Interp.endTime = t;
                self->alpha2Interp.method = static_cast<InterpMethod>(mode);

                self->alpha2Interp.goal = { a / 255.0f };
                self->alpha2Interp.timer.set(&self->alpha2Interp.timer, 0);
            } break;

            case ANM_FLIPX: // flipX
            {
            } break;

            case ANM_FLIPY: // flipY
            {
            } break;

            case ANM_STOP: // stop
            {
                goto stmt;
            }

            // A label for an interrupt. When executed, it is a no-op.
            case ANM_INTERRUPT_LABEL: // interruptLabel(int n)
            {
                // No-op.
            } break;

            case ANM_UNKNOWN_65:
            {
            } break;

            // Set the color blending mode.
            case ANM_BLENDMODE: // blendMode(int mode)
            {
                int mode = self->getIntArg(0);
                self->blend = static_cast<BlendMode>(mode);
            } break;

            // Determines how the ANM is rendered:
            // mode 0: 2D sprites, no rotation.
            // mode 1: 2D sprites, Z-axis rotation.
            // mode 8: 3D rotation.
            case ANM_TYPE: // type(int mode)
            {
                int mode = self->getIntArg(0);
            } break;


            // Sets the layer from which the ANM is drawn.
            case ANM_LAYER: // layer(int n)
            {
            } break;

            // This is like stop, except it also hides the graphics by clearing the visibility flag.
            // Interpolation instructions like posTime will continue to advance, and interrupts can be triggered at any time.
            // Successful interrupts will automatically re-enable the visibility flag.
            case ANM_STOPHIDE: // stopHide
            {
                self->flags &= 0xfffffffe;

stmt:
                if (self->pendingInterrupt == 0)
                {
                    self->flags |= 0x1000;
                }
                else
                {
                    // We call interrupt now.
interrupt:
                    self->currentInstr = self->startOfScript;
                    instrInterrupt = NULL;

                    // Selects which interrupt to run.
                    while (1)
                    {
                        int16_t currentOpcode = self->currentInstr->opcode;
                        opcode = 0; // Nop.

                        // '64' is the labelInterrupt.
                        if ((currentOpcode == 64 && self->pendingInterrupt == self->currentInstr->args[0].i)
                           || currentOpcode == -1)
                        {
                            break;
                        }

                        if (currentOpcode == 64 && self->currentInstr->args[0].i == -1)
                        {
                            instrInterrupt = self->currentInstr;
                        }
                        break;
                    }

                    if (self->currentInstr->opcode == 64 ||
                        (self->currentInstr = instrInterrupt, instrInterrupt != NULL))
                    {
                        // Do something with the timer.
                        self->interruptReturnTime.previous = self->timeInScript.previous;
                        self->interruptReturnTime.current = self->timeInScript.current;
                        self->interruptReturnTime.currentF = self->timeInScript.currentF;
                        self->interruptReturnTime.gameSpeed = self->timeInScript.gameSpeed;
                        self->interruptReturnTime.isInitialized = self->timeInScript.isInitialized;
                        self->interruptReturnInstr = self->currentInstr;
                        self->timeInScript.set(&self->timeInScript, self->currentInstr->time);
                        self->pendingInterrupt = self->currentInstr->offsetToNextInstr;
                        //self->flags |= 1;

                        break;
                    }
                }

                self->timeInScript.addf(&self->timeInScript, -1.0f);
            } break;

            case ANM_DRAWMODE: // drawMode(int mode)
            {
                int mode = self->getIntArg(0);
                self->drawMode = static_cast<AnmVM_drawMode>(mode);
            } break;

            case ANM_REGULAR_POLY: // regularPoly(int sides, float r1, float r2)
            {
                int sides = self->getIntArg(0);
                float r1 = self->getFloatArg(1);
                float r2 = self->getFloatArg(2);

                self->nPoints = sides;
                self->radius1 = r1;
                self->radius2 = r2;
            } break;

            default:
            {
                fprintf(stderr, "this sucks\n");
            } break;
        }

        // Print instruction debug information.
        if (self->currentInstr != NULL && 0)
        {
            fprintf(stderr, "\nopcode=%d (%s)\n", self->currentInstr->opcode, ANM_getInstrName(self->currentInstr->opcode));

            fprintf(stderr, "REGDUMP:\n");
            for (int i = 0; i < 4; ++i)
            {
                fprintf(stderr, "intVars[%d] = %d\n", i, self->intVars[i]);
            }
            for (int i = 0; i < 4; ++i)
            {
                fprintf(stderr, "floatVars[%d] = %f\n", i, self->floatVars[i]);
            }
        }

        if (self->currentInstr != NULL)
        {
            self->loadNextInstr();
        }
    }
}
