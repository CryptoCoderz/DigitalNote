#include "cdiskblockindex.h"

#include "main.h"

CDiskBlockIndex::CDiskBlockIndex()
{
	hashPrev = 0;
	hashNext = 0;
	blockHash = 0;
}

CDiskBlockIndex::CDiskBlockIndex(CBlockIndex* pindex) : CBlockIndex(*pindex)
{
	hashPrev = (pprev ? pprev->GetBlockHash() : 0);
	hashNext = (pnext ? pnext->GetBlockHash() : 0);
}

uint256 CDiskBlockIndex::GetBlockHash() const
{
	if (fUseFastIndex && (nTime < GetAdjustedTime() - 24 * 60 * 60) && blockHash != 0)
		return blockHash;

	CBlock block;
	block.nVersion        = nVersion;
	block.hashPrevBlock   = hashPrev;
	block.hashMerkleRoot  = hashMerkleRoot;
	block.nTime           = nTime;
	block.nBits           = nBits;
	block.nNonce          = nNonce;

	const_cast<CDiskBlockIndex*>(this)->blockHash = block.GetHash();

	return blockHash;
}

std::string CDiskBlockIndex::ToString() const
{
	std::string str = "CDiskBlockIndex(";
	str += CBlockIndex::ToString();
	str += strprintf("\n                hashBlock=%s, hashPrev=%s, hashNext=%s)",
		GetBlockHash().ToString(),
		hashPrev.ToString(),
		hashNext.ToString());
	return str;
}
