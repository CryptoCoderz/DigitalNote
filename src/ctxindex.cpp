#include "main.h"
#include "cblock.h"
#include "cblockindex.h"

#include "ctxindex.h"

CTxIndex::CTxIndex()
{
	SetNull();
}

CTxIndex::CTxIndex(const CDiskTxPos& posIn, unsigned int nOutputs)
{
	pos = posIn;
	vSpent.resize(nOutputs);
}

void CTxIndex::SetNull()
{
	pos.SetNull();
	vSpent.clear();
}

bool CTxIndex::IsNull()
{
	return pos.IsNull();
}

bool operator==(const CTxIndex& a, const CTxIndex& b)
{
	return (a.pos    == b.pos &&
			a.vSpent == b.vSpent);
}

bool operator!=(const CTxIndex& a, const CTxIndex& b)
{
	return !(a == b);
}

int CTxIndex::GetDepthInMainChain() const
{
    // Read block header
    CBlock block;
    if (!block.ReadFromDisk(pos.nFile, pos.nBlockPos, false))
        return 0;
    // Find the block in the index
    std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(block.GetHash());
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !pindex->IsInMainChain())
        return 0;
    return 1 + nBestHeight - pindex->nHeight;
}
