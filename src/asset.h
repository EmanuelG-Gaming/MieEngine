#ifndef ASSET_H_
#define ASSET_H_ 1

#include "base/base_string.h"

// TODO: Current asset load
// doesn't use file archives.
// It just loads a file from binary.
extern STRING8 AssetLoad(const char* filePath);

#endif /* ASSET_H_ */
