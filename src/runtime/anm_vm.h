#ifndef ANM_VM_H_
#define ANM_VM_H_ 1

#include "../base/math/mathf.h"
#include "../core/timer.h"
#include "../core/interp.h"
#include "../graphics/draw.h"
#include "anm_loaded.h"

class AnmVM;


enum class TextDrawMode {
    none = 0,
    successive,
    jzboy,
};

// The decision system would
// have to have less parameters (enums).
typedef enum AnmVM_drawMode {
    ANM_DRAW_NONE = 0,
    ANM_DRAW_RECT,
    ANM_DRAW_ELLIPSE,
    ANM_DRAW_STAR,
    //ANM_DRAW_TEXT,
} AnmVM_drawMode;

typedef enum AnmVM_rotMode {
    ANM_ROT_XYZ = 0, // Base rotation mode.
    ANM_ROT_YXZ,
    ANM_ROT_YZX,
    ANM_ROT_XZY,
    ANM_ROT_ZXY,
    ANM_ROT_ZYX,
} AnmVM_rotMode;

typedef enum AnmVM_flags {
    ANM_FLAG_NONE = 0,
    ANM_FLAG_LINE = (1 << 0),
    ANM_FLAG_LINE_THROUGH = (1 << 1),
} AnmVM_flags;



typedef enum AnmVM_opcodes {
    ANM_NOP = 0,
    ANM_DESTROY = 1,
    ANM_FREEZE = 2,

    ANM_SPRITE = 3,
    ANM_JMP = 4,
    ANM_JMP_DEC = 5,

    ANM_ISET = 6,
    ANM_FSET = 7,

    ANM_IADD = 8,
    ANM_FADD = 9,
    ANM_ISUB = 10,
    ANM_FSUB = 11,
    ANM_IMUL = 12,
    ANM_FMUL = 13,
    ANM_IDIV = 14,
    ANM_FDIV = 15,
    ANM_IMOD = 16,
    ANM_FMOD = 17,

    ANM_ISETADD = 18,
    ANM_FSETADD = 19,
    ANM_ISETSUB = 20,
    ANM_FSETSUB = 21,
    ANM_ISETMUL = 22,
    ANM_FSETMUL = 23,
    ANM_ISETDIV = 24,
    ANM_FSETDIV = 25,
    ANM_ISETMOD = 26,
    ANM_FSETMOD = 27,

    ANM_IJE = 28,
    ANM_FJE = 29,
    ANM_IJNE = 30,
    ANM_FJNE = 31,
    ANM_IJL = 32,
    ANM_FJL = 33,
    ANM_IJLE = 34,
    ANM_FJLE = 35,
    ANM_IJG = 36,
    ANM_FJG = 37,
    ANM_IJGE = 38,
    ANM_FJGE = 39,

    ANM_ISETRAND = 40,
    ANM_FSETRAND = 41,

    ANM_FSIN = 42,
    ANM_FCOS = 43,
    ANM_FTAN = 44,
    ANM_FACOS = 45,
    ANM_FATAN = 46,
    ANM_WRAPANGLE = 47,

    ANM_POS = 48,
    ANM_ROTATE = 49,
    ANM_SCALE = 50,
    ANM_ALPHA = 51,
    ANM_COLOR = 52,

    ANM_ANGLE_VEL = 53,
    ANM_SCALE_GROWTH = 54,

    ANM_ALPHATIME_LINEAR = 55,
    ANM_POSTIME = 56,

    ANM_COLORTIME1 = 57,
    ANM_ALPHATIME1 = 58,

    ANM_COLORTIME2 = 59,
    ANM_ALPHATIME2 = 60,

    ANM_FLIPX = 61,
    ANM_FLIPY = 62,

    ANM_STOP = 63,

    ANM_INTERRUPT_LABEL = 64,
    ANM_UNKNOWN_65 = 65,

    ANM_BLENDMODE = 66,
    ANM_TYPE = 67,

    ANM_LAYER = 68,
    ANM_STOPHIDE = 69,

    ANM_COLORMODE = 70,
    ANM_DRAWMODE = 71,
    ANM_REGULAR_POLY = 72,
} AnmVM_opcodes;


/*
typedef enum AnmVM_registers {
    ANM_I1 = 10000,
    ANM_I2 = 10001,
    ANM_I3 = 10002,
    ANM_I4 = 10003,

    ANM_F1 = 10004,
    ANM_F2 = 10005,
    ANM_F3 = 10006,
    ANM_F4 = 10007,

    ANM_IRAND = 10010,
    ANM_POSX = 10013,
    ANM_POSY = 10014,
    ANM_POSZ = 10015,

    ANM_FRAND = 10022,
} AnmVM_registers;
*/


/*
*/

/*
typedef enum AnmValueType {
    ANM_VALUE_INT,
    ANM_VALUE_FLOAT,
} AnmValueType;
*/

typedef union AnmAnyVar {
    i32 i;
    f32 f;
} AnmAnyVar;

static inline AnmAnyVar I32LIT(i32 v) { AnmAnyVar r; r.i = v; return r; }
static inline AnmAnyVar F32LIT(f32 v) { AnmAnyVar r; r.f = v; return r; }

typedef struct AnmVM_rawInstr {
    int16_t opcode;
    int16_t offsetToNextInstr;
    short time;
    int16_t varMask;
    // Previously int args[10]
    AnmAnyVar args[10];
} AnmVM_rawInstr;
// Size is 48 bytes.

typedef struct AnmVM_sprite {
    GFX_texture* texture;
    float x, y, xs, ys;
} AnmVM_sprite;




typedef struct AnmID {
    int id;
} AnmID;

typedef struct AnmVM_listNode {
    AnmVM* entry;
    struct AnmVM_listNode* prev;
    struct AnmVM_listNode* next;
} AnmVM_listNode;

class AnmVM {
public:
    AnmVM_listNode globalListNode;
    AnmVM_listNode familyListNode;

    AnmVM* nextInLayerList;
    void (*onDraw)(struct AnmVM*);
    void (*onTick)(struct AnmVM*);

    AnmLoaded* anmLoaded;

    AnmVM_rawInstr* startOfScript;
    AnmVM_rawInstr* currentInstr;

    AnmVM_rawInstr* interruptReturnInstr;
    int pendingInterrupt;
    Timer interruptReturnTime;

    BlendMode blend;
    ColorMode colorMode;
    AnmVM_rotMode rotationMode;

    //vec3 position1Interp, position2Interp;
    vec3 pos;
    vec3 offsetPos;
    vec3 entityPos;
    vec3 rotation;
    vec2 scale;
    vec2 spriteSize;

    vec2 uvScrollPos;
    vec2 scrollVel;

    vec2 scaleGrowth;
    vec3 angularVelocity;

    float color1[4];
    float color2[4];

    mat4 matrix;

    Interp<vec3> posInterp;
    Interp<vec3> rotationInterp;
    Interp<vec3> scaleInterp;

    // Color interps (uses a pallete of 2 colors).
    Interp<vec3> rgbInterp;
    Interp<float> alphaInterp;
    Interp<vec3> rgb2Interp;
    Interp<float> alpha2Interp;

    int spriteNumber;
    int layer;

    int intVars[4];
    float floatVars[4];

    Timer timeInScript;

    AnmVM_drawMode drawMode;
    int32_t flags;

    AnmID id;
    uint16_t scriptNumber;
    uint16_t anmFileIndex;


    float thickness;
    float radius1, radius2;
    int nPoints;


    //float color1[4];
    //float color2[4];

    //const char* s;
    //int sLen;

    AnmVM(void);
    ~AnmVM(void);

    static void init(AnmVM* self);

    // Probably gonna put the raw instr array in a "loaded" struct.
    static void loadScript(AnmVM* self, AnmVM_rawInstr* instr);
    static void loadIntoAnmVM(AnmVM* self, AnmLoaded* anmLoaded, int scriptNumber);
    void loadSingleAnmScript(AnmVM* self, AnmLoaded* anmLoaded, uint32_t scriptNumber);

    static int getIntVar(AnmVM* self, int id);
    static int* getIntVarPtr(AnmVM* self, int* id);

    static void chooseRotations(AnmVM* self, float* rx, float* ry, float* rz);

    static float getFloatVar(AnmVM* self, float id);
    static float* getFloatVarPtr(AnmVM* self, float* id);

    static float wrapAngleSum(float f1, float f2);
    static void setColor(AnmVM* self, float r, float g, float b, float a);
    static void setColor2(AnmVM* self, float r, float g, float b, float a);

    static void update(AnmVM* self);
    static void draw(AnmVM* self, AnmVM* parent);
    static void run(AnmVM* self);


    int getIntArg(uint8_t idx)
    {
        int x = currentInstr->args[idx].i;

        // The varMask is checked with a 1 bit that is
        // slid across with a bitshift left operation,
        // based on an index.
        //
        // The variable mask is intentionally inverted, so that
        // the arguments can act as variables rather than literals,
        // if you do something like 0x00.
        // So if the bit is 1, then it's a direct constant.
        if ((currentInstr->varMask & (1 << idx)) == 0)
        {
            // Represents the index of a variable.
            x = getIntVar(this, x);
        }
        // Is a constant (literal).
        return x;
    }
    float getFloatArg(uint8_t idx)
    {
        // Gets the literal.
        //float x = *(reinterpret_cast<float *>(&currentInstr->args[idx]));
        // TODO: Fix this.
        float x = currentInstr->args[idx].f;

        if ((currentInstr->varMask & (1 << idx)) == 0)
        {
            // Gets the register at float.
            x = getFloatVar(this, x);
        }

        return x;
    }

    int* getIntArgPtr(uint8_t idx)
    {
        int* x = reinterpret_cast<int *>(&currentInstr->args[idx]);
        if ((currentInstr->varMask & (1 << idx)) == 0)
        {
            x = getIntVarPtr(this, x);
        }
        return x;
    }
    float* getFloatArgPtr(uint8_t idx)
    {
        float* x = reinterpret_cast<float *>(&currentInstr->args[idx]);

        if ((currentInstr->varMask & (1 << idx)) == 0)
        {
            x = getFloatVarPtr(this, x);
        }
        return x;
    }



    void loadNextInstr(void)
    {
        currentInstr = reinterpret_cast<AnmVM_rawInstr *>
            (reinterpret_cast<char *>(currentInstr) + currentInstr->offsetToNextInstr);
    }

    void jumpToInstr(int offset)
    {
        currentInstr = reinterpret_cast<AnmVM_rawInstr *>
            (reinterpret_cast<char *>(currentInstr) + offset);
    }
};




#endif /* ANM_VM_H_ */
