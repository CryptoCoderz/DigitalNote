#ifndef CMERKLETX_H
#define CMERKLETX_H

#include <vector>

#include "uint256.h"
#include "ctransaction.h"

class CBlockIndex;
class CBlock;

/** A transaction with a merkle branch linking it to the block chain. */
class CMerkleTx : public CTransaction
{
private:
    int GetDepthInMainChainINTERNAL(CBlockIndex* &pindexRet) const;

public:
    uint256 hashBlock;
    std::vector<uint256> vMerkleBranch;
    int nIndex;

    // memory only
    mutable bool fMerkleVerified;
	
    CMerkleTx();
    CMerkleTx(const CTransaction& txIn);
    
	void Init();

    IMPLEMENT_SERIALIZE
    (
        nSerSize += SerReadWrite(s, *(CTransaction*)this, nType, nVersion, ser_action);
        nVersion = this->nVersion;
        READWRITE(hashBlock);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
    )

    int SetMerkleBranch(const CBlock* pblock=NULL);

    // Return depth of transaction in blockchain:
    // -1  : not in blockchain, and not in memory pool (conflicted transaction)
    //  0  : in memory pool, waiting to be included in a block
    // >=1 : this many blocks deep in the main chain
    int GetDepthInMainChain(CBlockIndex* &pindexRet, bool enableIX=true) const;
    int GetDepthInMainChain(bool enableIX=true) const;
    bool IsInMainChain() const;
    int GetBlocksToMaturity() const;
    bool AcceptToMemoryPool(bool fLimitFree=true, bool fRejectInsaneFee=true, bool ignoreFees=false);
    int GetTransactionLockSignatures() const;
    bool IsTransactionLockTimedOut() const;
};

#endif // CMERKLETX_H