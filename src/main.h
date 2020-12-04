// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_MAIN_H
#define BITCOIN_MAIN_H

#include "chain.h"
#include "bignum.h"
#include "sync.h"
#include "txmempool.h"
#include "net.h"
#include "script.h"
#include "scrypt.h"
#include "crypto/bmw/bmw512.h"
#include "crypto/echo/echo512.h"
#include "fork.h"
#include "genesis.h"
#include "mining.h"

#include <list>

struct CNodeStateStats;
class CValidationState;
class CBlock;
class CBlockIndex;
class CInv;
class CKeyItem;
class CNode;
class CReserveKey;
class CWallet;
class CReserveKey;
class CTxDB;
class CTxIndex;
class CWalletInterface;
class CBlockLocator;

/** The maximum allowed multiple for the computed block size */
static const unsigned int MAX_BLOCK_SIZE_INCREASE_MULTIPLE = 2;
/** The number of blocks to consider in the computation of median block size */
static const unsigned int NUM_BLOCKS_FOR_MEDIAN_BLOCK = 25;
/** The maximum allowed size for a serialized block, in bytes (network rule) */
static unsigned int MAX_BLOCK_SIZE = 15256128;
/** The minimum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MIN_BLOCK_SIZE = 1525612;
/** The maximum size for mined blocks */
static const unsigned int MAX_BLOCK_SIZE_GEN = MAX_BLOCK_SIZE/2;
/** Default for -blockprioritysize, maximum space for zero/low-fee transactions **/
static const unsigned int DEFAULT_BLOCK_PRIORITY_SIZE = 50000;
/** The maximum size for transactions we're willing to relay/mine **/
static const unsigned int MAX_STANDARD_TX_SIZE = MAX_BLOCK_SIZE_GEN/5;
/** The maximum allowed number of signature check operations in a block (network rule) */
static unsigned int MAX_BLOCK_SIGOPS = MAX_BLOCK_SIZE/50;
/** Maxiumum number of signature check operations in an IsStandard() P2SH script */
static const unsigned int MAX_P2SH_SIGOPS = 15;
/** The maximum number of sigops we're willing to relay/mine in a single tx */
static unsigned int MAX_TX_SIGOPS = MAX_BLOCK_SIGOPS/5;
/** The maximum number of orphan transactions kept in memory */
static const unsigned int MAX_ORPHAN_TRANSACTIONS = MAX_BLOCK_SIZE/100;
/** Default for -maxorphanblocks, maximum number of orphan blocks kept in memory */
static const unsigned int DEFAULT_MAX_ORPHAN_BLOCKS = 10000;
/** Fees smaller than this (in satoshi) are considered zero fee (for transaction creation) */
static const int64_t MIN_TX_FEE = 0.0001*COIN;
/** Fees smaller than this (in satoshi) are considered zero fee (for relaying) */
static const int64_t MIN_RELAY_TX_FEE = MIN_TX_FEE;
/** Minimum TX count (for relaying) */
static const int64_t MIN_TX_COUNT = 0;
/** Minimum TX value (for relaying) */
static const int64_t MIN_TX_VALUE = 0.01 * COIN;
/** No amount larger than this (in satoshi) is valid */
static const int64_t MAX_SINGLE_TX = 10000000000 * COIN; // 10 Billion DigitalNote coins
/** Moneyrange params */
inline bool MoneyRange(int64_t nValue) { return (nValue >= 0 && nValue <= MAX_SINGLE_TX); }
/** Threshold for nLockTime: below this value it is interpreted as block number, otherwise as UNIX timestamp. */
static const unsigned int LOCKTIME_THRESHOLD = 500000000; // Tue Nov  5 00:53:20 1985 UTC
/** Number of blocks that can be requested at any given time from a single peer. */
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 128;
/** Timeout in seconds before considering a block download peer unresponsive. */
static const unsigned int BLOCK_DOWNLOAD_TIMEOUT = 60;
/** Defaults to yes, adaptively increase/decrease max/min/priority along with the re-calculated block size **/
static const unsigned int DEFAULT_SCALE_BLOCK_SIZE_OPTIONS = 1;
/** Future drift value */
static const int64_t nDrift = 5 * 60;
/** Future drift params */
inline int64_t FutureDrift(int64_t nTime) { return nTime + nDrift; }
/** "reject" message codes **/
static const unsigned char REJECT_INVALID = 0x10;

struct CMainSignals {
    // Notifies listeners of updated transaction data (passing hash, transaction, and optionally the block it is found in.
    boost::signals2::signal<void (const CTransaction &, const CBlock *, bool, bool)> SyncTransaction;
    // Notifies listeners of an erased transaction (currently disabled, requires transaction replacement).
    boost::signals2::signal<void (const uint256 &)> EraseTransaction;
    // Notifies listeners of an updated transaction without new data (for now: a coinbase potentially becoming visible).
    boost::signals2::signal<void (const uint256 &)> UpdatedTransaction;
    // Notifies listeners of a new active block chain.
    boost::signals2::signal<void (const CBlockLocator &)> SetBestChain;
    // Notifies listeners about an inventory item being seen on the network.
    boost::signals2::signal<void (const uint256 &)> Inventory;
    // Tells listeners to broadcast their data.
    boost::signals2::signal<void (bool)> Broadcast;
};

extern CScript COINBASE_FLAGS;
extern CCriticalSection cs_main;
extern CTxMemPool mempool;
extern std::map<uint256, CBlockIndex*> mapBlockIndex;
extern std::set<std::pair<COutPoint, unsigned int> > setStakeSeen;
extern CBlockIndex* pindexGenesisBlock;
extern unsigned int nNodeLifespan;
extern int nBestHeight;
extern uint256 nBestChainTrust;
extern uint256 nBestInvalidTrust;
extern uint256 hashBestChain;
extern CBlockIndex* pindexBest;
extern uint64_t nLastBlockTx;
extern uint64_t nLastBlockSize;
extern int64_t nLastCoinStakeSearchInterval;
extern const std::string strMessageMagic;
extern int64_t nTimeBestReceived;
extern bool fImporting;
extern bool fReindex;
struct COrphanBlock;
extern std::map<uint256, COrphanBlock*> mapOrphanBlocks;
extern bool fHaveGUI;
// Settings
extern bool fUseFastIndex;
extern unsigned int nDerivationMethodIndex;

extern bool fLargeWorkForkFound;
extern bool fLargeWorkInvalidChainFound;

extern CMainSignals g_signals;
extern CBlockIndex* pblockindexFBBHLast;

// Minimum disk space required - used in CheckDiskSpace()
static const uint64_t nMinDiskSpace = 52428800;

/** Register a wallet to receive updates from core */
void RegisterWallet(CWalletInterface* pwalletIn);
/** Unregister a wallet from core */
void UnregisterWallet(CWalletInterface* pwalletIn);
/** Unregister all wallets from core */
void UnregisterAllWallets();
/** Push an updated transaction to all registered wallets */
void SyncWithWallets(const CTransaction& tx, const CBlock* pblock = NULL, bool fConnect = true, bool fFixSpentCoins = false);
/** Ask wallets to resend their transactions */
void ResendWalletTransactions(bool fForce = false);
/** Register with a network node to receive its signals */
void RegisterNodeSignals(CNodeSignals& nodeSignals);
/** Unregister a network node */
void UnregisterNodeSignals(CNodeSignals& nodeSignals);

void PushGetBlocks(CNode* pnode, CBlockIndex* pindexBegin, uint256 hashEnd);
bool ProcessBlock(CNode* pfrom, CBlock* pblock);
bool CheckDiskSpace(uint64_t nAdditionalBytes=0);
FILE* OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, const char* pszMode="rb");
FILE* AppendBlockFile(unsigned int& nFileRet);
bool LoadBlockIndex(bool fAllowNew=true);
void PrintBlockTree();
CBlockIndex* FindBlockByHeight(int nHeight);
bool ProcessMessages(CNode* pfrom);
bool SendMessages(CNode* pto, bool fSendTrickle);
void ThreadImport(std::vector<boost::filesystem::path> vImportFiles);
bool CheckProofOfWork(uint256 hash, unsigned int nBits);
bool IsInitialBlockDownload();
bool IsConfirmedInNPrevBlocks(const CTxIndex& txindex, const CBlockIndex* pindexFrom, int nMaxDepth, int& nActualDepth);
std::string GetWarnings(const std::string &strFor);
bool GetTransaction(const uint256 &hash, CTransaction &tx, uint256 &hashBlock);
uint256 WantedByOrphan(const COrphanBlock* pblockOrphan);
const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake);
void ThreadStakeMiner(CWallet *pwallet);


/** (try to) add transaction to memory pool **/
bool AcceptToMemoryPool(CTxMemPool& pool, CTransaction &tx, bool fLimitFree,
                        bool* pfMissingInputs, bool fRejectinsaneFee=false, bool ignoreFees=false, bool fFixSpentCoins=false);

bool AcceptableInputs(CTxMemPool& pool, const CTransaction &txo, bool fLimitFree,
                        bool* pfMissingInputs, bool fRejectinsaneFee=false, bool isDSTX=false);


bool FindTransactionsByDestination(const CTxDestination &dest, std::vector<uint256> &vtxhash);

int GetInputAge(CTxIn& vin);
int GetInputAgeIX(uint256 nTXHash, CTxIn& vin);
int GetIXConfirmations(uint256 nTXHash);
/** Abort with a message */
bool AbortNode(const std::string &msg, const std::string &userMessage="");
/** Get statistics from node state */
bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats);
/** Increase a node's misbehavior score. */
void Misbehaving(NodeId nodeid, int howmuch);

#include "cdisktxpos.h"

enum GetMinFee_mode
{
    GMF_BLOCK,
    GMF_RELAY,
    GMF_SEND,
};

int64_t GetMinFee(const CTransaction& tx, unsigned int nBytes, bool fAllowFree, enum GetMinFee_mode mode);

#include "ctransaction.h"
#include "ctxoutcompressor.h"

/** Check for standard transaction types
    @param[in] mapInputs    Map of previous transactions that have outputs we're spending
    @return True if all inputs (scriptSigs) use only standard transaction forms
    @see CTransaction::FetchInputs
*/
bool AreInputsStandard(const CTransaction& tx, const MapPrevTx& mapInputs);

/** Count ECDSA signature operations the old-fashioned (pre-0.6) way
    @return number of sigops this transaction's outputs will produce when spent
    @see CTransaction::FetchInputs
*/
unsigned int GetLegacySigOpCount(const CTransaction& tx);

/** Count ECDSA signature operations in pay-to-script-hash inputs.

    @param[in] mapInputs    Map of previous transactions that have outputs we're spending
    @return maximum number of sigops required to validate this transaction's inputs
    @see CTransaction::FetchInputs
 */
unsigned int GetP2SHSigOpCount(const CTransaction& tx, const MapPrevTx& mapInputs);

inline bool AllowFree(double dPriority)
{
    // Large (in bytes) low-priority (new, small-coin) transactions
    // need a fee.
    return dPriority > COIN * 576 / 250;
}

/** Check for standard transaction types
    @return True if all outputs (scriptPubKeys) use only standard transaction forms
*/
bool IsStandardTx(const CTransaction& tx, std::string& reason);

bool IsFinalTx(const CTransaction &tx, int nBlockHeight = 0, int64_t nBlockTime = 0);

bool BuildAddrIndex(const CScript &script, std::vector<uint160>& addrIds);
bool Reorganize(CTxDB& txdb, CBlockIndex* pindexNew);
void InvalidChainFound(CBlockIndex* pindexNew);

#include "cmerkletx.h"
#include "ctxindex.h"
#include "cblock.h"
#include "cdiskblockpos.h"
#include "cblockindex.h"

/** Used to marshal pointers into hashes for db storage. */
class CDiskBlockIndex : public CBlockIndex
{
private:
    uint256 blockHash;

public:
    uint256 hashPrev;
    uint256 hashNext;

    CDiskBlockIndex()
    {
        hashPrev = 0;
        hashNext = 0;
        blockHash = 0;
    }

    explicit CDiskBlockIndex(CBlockIndex* pindex) : CBlockIndex(*pindex)
    {
        hashPrev = (pprev ? pprev->GetBlockHash() : 0);
        hashNext = (pnext ? pnext->GetBlockHash() : 0);
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);

        READWRITE(hashNext);
        READWRITE(nFile);
        READWRITE(nBlockPos);
        READWRITE(nHeight);
        READWRITE(nMint);
        READWRITE(nMoneySupply);
        READWRITE(nFlags);
        READWRITE(nStakeModifier);
        READWRITE(bnStakeModifierV2);
        if (IsProofOfStake())
        {
            READWRITE(prevoutStake);
            READWRITE(nStakeTime);
        }
        else if (fRead)
        {
            const_cast<CDiskBlockIndex*>(this)->prevoutStake.SetNull();
            const_cast<CDiskBlockIndex*>(this)->nStakeTime = 0;
        }
        READWRITE(hashProof);

        // block header
        READWRITE(this->nVersion);
        READWRITE(hashPrev);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
        READWRITE(blockHash);
    )

    uint256 GetBlockHash() const
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

    std::string ToString() const
    {
        std::string str = "CDiskBlockIndex(";
        str += CBlockIndex::ToString();
        str += strprintf("\n                hashBlock=%s, hashPrev=%s, hashNext=%s)",
            GetBlockHash().ToString(),
            hashPrev.ToString(),
            hashNext.ToString());
        return str;
    }
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
class CBlockLocator
{
protected:
    std::vector<uint256> vHave;
public:

    CBlockLocator()
    {
    }

    explicit CBlockLocator(const CBlockIndex* pindex)
    {
        Set(pindex);
    }

    explicit CBlockLocator(uint256 hashBlock)
    {
        std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end())
            Set((*mi).second);
    }

    CBlockLocator(const std::vector<uint256>& vHaveIn)
    {
        vHave = vHaveIn;
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    )

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull()
    {
        return vHave.empty();
    }

    void Set(const CBlockIndex* pindex)
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

    int GetDistanceBack()
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

    CBlockIndex* GetBlockIndex()
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

    uint256 GetBlockHash()
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

    int GetHeight()
    {
        CBlockIndex* pindex = GetBlockIndex();
        if (!pindex)
            return 0;
        return pindex->nHeight;
    }
};

/** Capture information about block/transaction validation */
class CValidationState {
private:
    enum mode_state {
        MODE_VALID,   //! everything ok
        MODE_INVALID, //! network rule violation (DoS value may be set)
        MODE_ERROR,   //! run-time error
    } mode;
    int nDoS;
    std::string strRejectReason;
    unsigned char chRejectCode;
    bool corruptionPossible;
public:
    CValidationState() : mode(MODE_VALID), nDoS(0), chRejectCode(0), corruptionPossible(false) {}
    bool DoS(int level, bool ret = false,
             unsigned char chRejectCodeIn=0, std::string strRejectReasonIn="",
             bool corruptionIn=false) {
        chRejectCode = chRejectCodeIn;
        strRejectReason = strRejectReasonIn;
        corruptionPossible = corruptionIn;
        if (mode == MODE_ERROR)
            return ret;
        nDoS += level;
        mode = MODE_INVALID;
        return ret;
    }
    bool Invalid(bool ret = false,
                 unsigned char _chRejectCode=0, std::string _strRejectReason="") {
        return DoS(0, ret, _chRejectCode, _strRejectReason);
    }
    bool Error(std::string strRejectReasonIn="") {
        if (mode == MODE_VALID)
            strRejectReason = strRejectReasonIn;
        mode = MODE_ERROR;
        return false;
    }
    bool Abort(const std::string &msg) {
        AbortNode(msg);
        return Error(msg);
    }
    bool IsValid() const {
        return mode == MODE_VALID;
    }
    bool IsInvalid() const {
        return mode == MODE_INVALID;
    }
    bool IsError() const {
        return mode == MODE_ERROR;
    }
    bool IsInvalid(int &nDoSOut) const {
        if (IsInvalid()) {
            nDoSOut = nDoS;
            return true;
        }
        return false;
    }
    bool CorruptionPossible() const {
        return corruptionPossible;
    }
    unsigned char GetRejectCode() const { return chRejectCode; }
    std::string GetRejectReason() const { return strRejectReason; }
};

class CWalletInterface {
protected:
    virtual void SyncTransaction(const CTransaction &tx, const CBlock *pblock, bool fConnect, bool fFixSpentCoins) =0;
    virtual void EraseFromWallet(const uint256 &hash) =0;
    virtual void SetBestChain(const CBlockLocator &locator) =0;
    virtual bool UpdatedTransaction(const uint256 &hash) =0;
    virtual void Inventory(const uint256 &hash) =0;
    virtual void ResendWalletTransactions(bool fForce) =0;
    friend void ::RegisterWallet(CWalletInterface*);
    friend void ::UnregisterWallet(CWalletInterface*);
    friend void ::UnregisterAllWallets();
};

#endif
/** Open a block file (blk?????.dat) */
FILE* OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly);
/** Open an undo file (rev?????.dat) */
FILE* OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly);
/** Translation to a filesystem path */
boost::filesystem::path GetBlockPosFilename(const CDiskBlockPos &pos, const char *prefix);
