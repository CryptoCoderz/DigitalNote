#include "cblocklocator.h"

#include <map>

#include "uint256.h"
#include "main.h"

CBlockLocator::CBlockLocator()
{
}

CBlockLocator::CBlockLocator(const CBlockIndex* pindex)
{
	Set(pindex);
}

CBlockLocator::CBlockLocator(uint256 hashBlock)
{
	std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
	if (mi != mapBlockIndex.end())
		Set((*mi).second);
}

CBlockLocator::CBlockLocator(const std::vector<uint256>& vHaveIn)
{
	vHave = vHaveIn;
}

void CBlockLocator::SetNull()
{
	vHave.clear();
}

bool CBlockLocator::IsNull()
{
	return vHave.empty();
}

void CBlockLocator::Set(const CBlockIndex* pindex)
{
	vHave.clear();
	int nStep = 1;
	while (pindex)
	{
		vHave.push_back(pindex->GetBlockHash());

		// Exponentially larger steps back
		for (int i = 0; pindex && i < nStep; i++)
			pindex = pindex->pprev;
		if (vHave.size() > 10)
			nStep *= 2;
	}
	vHave.push_back(Params().HashGenesisBlock());
}

int CBlockLocator::GetDistanceBack()
{
	// Retrace how far back it was in the sender's branch
	int nDistance = 0;
	int nStep = 1;
	BOOST_FOREACH(const uint256& hash, vHave)
	{
		std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
		if (mi != mapBlockIndex.end())
		{
			CBlockIndex* pindex = (*mi).second;
			if (pindex->IsInMainChain())
				return nDistance;
		}
		nDistance += nStep;
		if (nDistance > 10)
			nStep *= 2;
	}
	return nDistance;
}

CBlockIndex* CBlockLocator::GetBlockIndex()
{
	// Find the first block the caller has in the main chain
	BOOST_FOREACH(const uint256& hash, vHave)
	{
		std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
		if (mi != mapBlockIndex.end())
		{
			CBlockIndex* pindex = (*mi).second;
			if (pindex->IsInMainChain())
				return pindex;
		}
	}
	return pindexGenesisBlock;
}

uint256 CBlockLocator::GetBlockHash()
{
	// Find the first block the caller has in the main chain
	BOOST_FOREACH(const uint256& hash, vHave)
	{
		std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
		if (mi != mapBlockIndex.end())
		{
			CBlockIndex* pindex = (*mi).second;
			if (pindex->IsInMainChain())
				return hash;
		}
	}
	return Params().HashGenesisBlock();
}

int CBlockLocator::GetHeight()
{
	CBlockIndex* pindex = GetBlockIndex();
	if (!pindex)
		return 0;
	return pindex->nHeight;
}
