#ifndef CDISKBLOCKPOS_H
#define CDISKBLOCKPOS_H

#include "serialize.h"

// Adaptive block sizing depends on this
struct CDiskBlockPos
{
    int nFile;
    unsigned int nPos;

    IMPLEMENT_SERIALIZE (
        READWRITE(VARINT(nFile));
        READWRITE(VARINT(nPos));
    )

    CDiskBlockPos();
    CDiskBlockPos(int nFileIn, unsigned int nPosIn);
    friend bool operator==(const CDiskBlockPos &a, const CDiskBlockPos &b);
    friend bool operator!=(const CDiskBlockPos &a, const CDiskBlockPos &b);
    void SetNull();
    bool IsNull() const;

    std::string ToString() const;
};

#endif // CDISKBLOCKPOS_H