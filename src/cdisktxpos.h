#ifndef CDISKTXPOS_H
#define CDISKTXPOS_H

#include <string>

#include "serialize.h"

/** Position on disk for a particular transaction. */
class CDiskTxPos
{
public:
    unsigned int nFile;
    unsigned int nBlockPos;
    unsigned int nTxPos;

    CDiskTxPos();
    CDiskTxPos(unsigned int nFileIn, unsigned int nBlockPosIn, unsigned int nTxPosIn)
		: nFile(nFileIn), nBlockPos(nBlockPosIn), nTxPos(nTxPosIn) {};

    IMPLEMENT_SERIALIZE(READWRITE(FLATDATA(*this));)
    
	void SetNull();
    bool IsNull() const;
	friend bool operator==(const CDiskTxPos& a, const CDiskTxPos& b);
    friend bool operator!=(const CDiskTxPos& a, const CDiskTxPos& b);
    std::string ToString() const;
};

#endif // CDISKTXPOS_H