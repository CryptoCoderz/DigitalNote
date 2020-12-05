#include "tinyformat.h"

#include "cdisktxpos.h"

CDiskTxPos::CDiskTxPos()
{
	this->SetNull();
}

void CDiskTxPos::SetNull() {
	this->nFile = (unsigned int) -1;
	this->nBlockPos = 0;
	this->nTxPos = 0;
}

bool CDiskTxPos::IsNull() const {
	return (this->nFile == (unsigned int) -1);
}

bool operator==(const CDiskTxPos& a, const CDiskTxPos& b)
{
	return (a.nFile     == b.nFile &&
			a.nBlockPos == b.nBlockPos &&
			a.nTxPos    == b.nTxPos);
}

bool operator!=(const CDiskTxPos& a, const CDiskTxPos& b)
{
	return !(a == b);
}
	
std::string CDiskTxPos::ToString() const
{
	if (IsNull())
		return "null";
	else
		return strprintf("(nFile=%u, nBlockPos=%u, nTxPos=%u)", nFile, nBlockPos, nTxPos);
}