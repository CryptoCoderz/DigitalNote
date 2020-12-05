#include "tinyformat.h"

#include "cdiskblockpos.h"

CDiskBlockPos::CDiskBlockPos() {
	SetNull();
}

CDiskBlockPos::CDiskBlockPos(int nFileIn, unsigned int nPosIn) {
	nFile = nFileIn;
	nPos = nPosIn;
}

bool operator==(const CDiskBlockPos &a, const CDiskBlockPos &b) {
	return (a.nFile == b.nFile && a.nPos == b.nPos);
}

bool operator!=(const CDiskBlockPos &a, const CDiskBlockPos &b) {
	return !(a == b);
}

void CDiskBlockPos::SetNull()
{
	nFile = -1;
	nPos = 0;
}

bool CDiskBlockPos::IsNull() const
{
	return (nFile == -1);
}

std::string CDiskBlockPos::ToString() const
{
	return strprintf("CBlockDiskPos(nFile=%i, nPos=%i)", nFile, nPos);
}