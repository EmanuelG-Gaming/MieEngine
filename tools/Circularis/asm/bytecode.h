#ifndef CCL_BYTECODE_H_
#define CCL_BYTECODE_H_ 1

#include "../base/types.h"

#ifndef CCL_NON_NAMESPACE
namespace ccl {
#endif

typedef struct BytecodeHeader {
    char magic[4];
    i32 labelCount;
    i32 labelOffset;

    i32 segmentCount;
    i32 segmentOffset;
} BytecodeHeader;

typedef struct BytecodeLabel {
    char name[64];
    i32 nameLen;

    i32 instrCount;
    i32 offset;
    i32 endInstrOffset;
} BytecodeLabel;

typedef struct BytecodeSegment {
    char name[64];
    i32 offset;
    i32 length;
    i32 flags;
} BytecodeSegment;





#ifndef CCL_NON_NAMESPACE
} /* namespace ccl */
#endif

#endif /* CCL_BYTECODE_H_ */
