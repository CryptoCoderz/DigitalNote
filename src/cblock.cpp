#include <boost/algorithm/string/replace.hpp>

#include "util.h"
#include "crypto/bmw/bmw512.h"
#include "ctransaction.h"
#include "main.h"
#include "txdb-leveldb.h"
#include "blocksizecalculator.h"
#include "blockparams.h"
#include "ui_interface.h"
#include "kernel.h"
#include "spork.h"
#include "instantx.h"
#include "velocity.h"
#include "checkpoints.h"

#include "cblock.h"

// Every received block is assigned a unique and increasing identifier, so we
// know which one to give priority in case of a fork.
CCriticalSection cs_nBlockSequenceId;

// Blocks loaded from disk are assigned id 0, so start the counter at 1.
uint32_t nBlockSequenceId = 1;

bool CBlock::DoS(int nDoSIn, bool fIn) const
{
	nDoS += nDoSIn;
	
	return fIn;
}

CBlock::CBlock()
{
	SetNull();
}

void CBlock::SetNull()
{
	nVersion = CBlock::CURRENT_VERSION;
	hashPrevBlock = 0;
	hashMerkleRoot = 0;
	nTime = 0;
	nBits = 0;
	nNonce = 0;
	vtx.clear();
	vchBlockSig.clear();
	vMerkleTree.clear();
	nDoS = 0;
}

bool CBlock::IsNull() const
{
	return (nBits == 0);
}

uint256 CBlock::GetHash() const
{
	if (nVersion > 6)
		return Hash_bmw512(BEGIN(nVersion), END(nNonce));
	else
		return GetPoWHash();
}

uint256 CBlock::GetPoWHash() const
{
 return Hash_bmw512(BEGIN(nVersion), END(nNonce));
}

int64_t CBlock::GetBlockTime() const
{
	return (int64_t)nTime;
}

void CBlock::UpdateTime(const CBlockIndex* pindexPrev)
{
    nTime = std::max(GetBlockTime(), GetAdjustedTime());
}

// entropy bit for stake modifier if chosen by modifier
unsigned int CBlock::GetStakeEntropyBit() const
{
	// Take last bit of block hash as entropy bit
	unsigned int nEntropyBit = ((GetHash().Get64()) & 1llu);
	LogPrint("stakemodifier", "GetStakeEntropyBit: hashBlock=%s nEntropyBit=%u\n", GetHash().ToString(), nEntropyBit);
	return nEntropyBit;
}

// ppcoin: two types of block: proof-of-work or proof-of-stake
bool CBlock::IsProofOfStake() const
{
	return (vtx.size() > 1 && vtx[1].IsCoinStake());
}

bool CBlock::IsProofOfWork() const
{
	return !IsProofOfStake();
}

std::pair<COutPoint, unsigned int> CBlock::GetProofOfStake() const
{
	return IsProofOfStake()? std::make_pair(vtx[1].vin[0].prevout, vtx[1].nTime) : std::make_pair(COutPoint(), (unsigned int)0);
}

// ppcoin: get max transaction timestamp
int64_t CBlock::GetMaxTransactionTime() const
{
	int64_t maxTransactionTime = 0;
	BOOST_FOREACH(const CTransaction& tx, vtx)
		maxTransactionTime = std::max(maxTransactionTime, (int64_t)tx.nTime);
	return maxTransactionTime;
}

uint256 CBlock::BuildMerkleTree() const
{
	vMerkleTree.clear();
	BOOST_FOREACH(const CTransaction& tx, vtx)
		vMerkleTree.push_back(tx.GetHash());
	int j = 0;
	for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
	{
		for (int i = 0; i < nSize; i += 2)
		{
			int i2 = std::min(i+1, nSize-1);
			vMerkleTree.push_back(Hash(BEGIN(vMerkleTree[j+i]),  END(vMerkleTree[j+i]),
									   BEGIN(vMerkleTree[j+i2]), END(vMerkleTree[j+i2])));
		}
		j += nSize;
	}
	return (vMerkleTree.empty() ? 0 : vMerkleTree.back());
}

std::vector<uint256> CBlock::GetMerkleBranch(int nIndex) const
{
	if (vMerkleTree.empty())
		BuildMerkleTree();
	std::vector<uint256> vMerkleBranch;
	int j = 0;
	for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
	{
		int i = std::min(nIndex^1, nSize-1);
		vMerkleBranch.push_back(vMerkleTree[j+i]);
		nIndex >>= 1;
		j += nSize;
	}
	return vMerkleBranch;
}

static uint256 CBlock::CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex)
{
	if (nIndex == -1)
		return 0;
	BOOST_FOREACH(const uint256& otherside, vMerkleBranch)
	{
		if (nIndex & 1)
			hash = Hash(BEGIN(otherside), END(otherside), BEGIN(hash), END(hash));
		else
			hash = Hash(BEGIN(hash), END(hash), BEGIN(otherside), END(otherside));
		nIndex >>= 1;
	}
	return hash;
}


bool CBlock::WriteToDisk(unsigned int& nFileRet, unsigned int& nBlockPosRet)
{
	// Open history file to append
	CAutoFile fileout = CAutoFile(AppendBlockFile(nFileRet), SER_DISK, CLIENT_VERSION);
	if (!fileout)
		return error("CBlock::WriteToDisk() : AppendBlockFile failed");

	// Write index header
	unsigned int nSize = fileout.GetSerializeSize(*this);
	fileout << FLATDATA(Params().MessageStart()) << nSize;

	// Write block
	long fileOutPos = ftell(fileout);
	if (fileOutPos < 0)
		return error("CBlock::WriteToDisk() : ftell failed");
	nBlockPosRet = fileOutPos;
	fileout << *this;

	// Flush stdio buffers and commit to disk before returning
	fflush(fileout);
	if (!IsInitialBlockDownload() || (nBestHeight+1) % 500 == 0)
		FileCommit(fileout);

	return true;
}

bool CBlock::ReadFromDisk(unsigned int nFile, unsigned int nBlockPos, bool fReadTransactions)
{
	SetNull();

	// Open history file to read
	CAutoFile filein = CAutoFile(OpenBlockFile(nFile, nBlockPos, "rb"), SER_DISK, CLIENT_VERSION);
	if (!filein)
		return error("CBlock::ReadFromDisk() : OpenBlockFile failed");
	if (!fReadTransactions)
		filein.nType |= SER_BLOCKHEADERONLY;

	// Read block
	try {
		filein >> *this;
	}
	catch (std::exception &e) {
		return error("%s() : deserialize or I/O error", __PRETTY_FUNCTION__);
	}

	// Check the header
	if (fReadTransactions && IsProofOfWork() && !CheckProofOfWork(GetPoWHash(), nBits))
		return error("CBlock::ReadFromDisk() : errors in block header");

	return true;
}

std::string CBlock::ToString() const
{
	std::stringstream s;
	s << strprintf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u, vchBlockSig=%s)\n",
		GetHash().ToString(),
		nVersion,
		hashPrevBlock.ToString(),
		hashMerkleRoot.ToString(),
		nTime, nBits, nNonce,
		vtx.size(),
		HexStr(vchBlockSig.begin(), vchBlockSig.end()));
	for (unsigned int i = 0; i < vtx.size(); i++)
	{
		s << "  " << vtx[i].ToString() << "\n";
	}
	s << "  vMerkleTree: ";
	for (unsigned int i = 0; i < vMerkleTree.size(); i++)
		s << " " << vMerkleTree[i].ToString();
	s << "\n";
	return s.str();
}

bool CBlock::DisconnectBlock(CTxDB& txdb, CBlockIndex* pindex)
{
    // Disconnect in reverse order
    for (int i = vtx.size()-1; i >= 0; i--)
        if (!vtx[i].DisconnectInputs(txdb))
            return false;

    // Update block index on disk without changing it in memory.
    // The memory index structure will be changed after the db commits.
    if (pindex->pprev)
    {
        CDiskBlockIndex blockindexPrev(pindex->pprev);
        blockindexPrev.hashNext = 0;
        if (!txdb.WriteBlockIndex(blockindexPrev))
            return error("DisconnectBlock() : WriteBlockIndex failed");
    }

    // ppcoin: clean up wallet after disconnecting coinstake
    BOOST_FOREACH(CTransaction& tx, vtx)
        SyncWithWallets(tx, this, false);

    return true;
}

bool CBlock::ConnectBlock(CTxDB& txdb, CBlockIndex* pindex, bool fJustCheck)
{
    // Check it again in case a previous version let a bad block in, but skip BlockSig checking
    if (!CheckBlock(!fJustCheck, !fJustCheck, false))
        return false;

    unsigned int flags = SCRIPT_VERIFY_NOCACHE;

    //// issue here: it doesn't know the version
    unsigned int nTxPos;
    if (fJustCheck)
        // FetchInputs treats CDiskTxPos(1,1,1) as a special "refer to memorypool" indicator
        // Since we're just checking the block and not actually connecting it, it might not (and probably shouldn't) be on the disk to get the transaction from
        nTxPos = 1;
    else
        nTxPos = pindex->nBlockPos + ::GetSerializeSize(CBlock(), SER_DISK, CLIENT_VERSION) - (2 * GetSizeOfCompactSize(0)) + GetSizeOfCompactSize(vtx.size());

    map<uint256, CTxIndex> mapQueuedChanges;
    int64_t nFees = 0;
    int64_t nValueIn = 0;
    int64_t nValueOut = 0;
    int64_t nStakeReward = 0;
    unsigned int nSigOps = 0;
    int nInputs = 0;

    MAX_BLOCK_SIZE = BlockSizeCalculator::ComputeBlockSize(pindex);
    MAX_BLOCK_SIGOPS = MAX_BLOCK_SIZE/50;
    MAX_TX_SIGOPS = MAX_BLOCK_SIGOPS/5;

    BOOST_FOREACH(CTransaction& tx, vtx)
    {
        uint256 hashTx = tx.GetHash();
        nInputs += tx.vin.size();
        nSigOps += GetLegacySigOpCount(tx);

        if (nSigOps > MAX_BLOCK_SIGOPS)
            return DoS(100, error("ConnectBlock() : too many sigops"));

        CDiskTxPos posThisTx(pindex->nFile, pindex->nBlockPos, nTxPos);
        if (!fJustCheck)
            nTxPos += ::GetSerializeSize(tx, SER_DISK, CLIENT_VERSION);

        MapPrevTx mapInputs;
        if (tx.IsCoinBase())
            nValueOut += tx.GetValueOut();
        else
        {
            bool fInvalid;
            if (!tx.FetchInputs(txdb, mapQueuedChanges, true, false, mapInputs, fInvalid))
                return false;

            // Add in sigops done by pay-to-script-hash inputs;
            // this is to prevent a "rogue miner" from creating
            // an incredibly-expensive-to-validate block.
            nSigOps += GetP2SHSigOpCount(tx, mapInputs);
            if (nSigOps > MAX_BLOCK_SIGOPS)
                return DoS(100, error("ConnectBlock() : too many sigops"));

            int64_t nTxValueIn = tx.GetValueIn(mapInputs);
            int64_t nTxValueOut = tx.GetValueOut();
            nValueIn += nTxValueIn;
            nValueOut += nTxValueOut;
            if (!tx.IsCoinStake())
                nFees += nTxValueIn - nTxValueOut;
            if (tx.IsCoinStake())
                nStakeReward = nTxValueOut - nTxValueIn;


            if (!tx.ConnectInputs(txdb, mapInputs, mapQueuedChanges, posThisTx, pindex, true, false, flags))
                return false;
        }

        mapQueuedChanges[hashTx] = CTxIndex(posThisTx, tx.vout.size());
    }

    if (IsProofOfWork())
    {
        int64_t nReward = GetProofOfWorkReward(pindex->nHeight, nFees);
        // Check coinbase reward
        if (vtx[0].GetValueOut() > nReward)
            return DoS(50, error("ConnectBlock() : coinbase reward exceeded (actual=%d vs calculated=%d)",
                   vtx[0].GetValueOut(),
                   nReward));
    }
    if (IsProofOfStake())
    {
        // ppcoin: coin stake tx earns reward instead of paying fee
        uint64_t nCoinAge;
        if (!vtx[1].GetCoinAge(txdb, pindex->pprev, nCoinAge))
            return error("ConnectBlock() : %s unable to get coin age for coinstake", vtx[1].GetHash().ToString());

        int64_t nCalculatedStakeReward = GetProofOfStakeReward(pindex->pprev, nCoinAge, nFees);

        if (nStakeReward > nCalculatedStakeReward)
            return DoS(100, error("ConnectBlock() : coinstake pays too much(actual=%d vs calculated=%d)", nStakeReward, nCalculatedStakeReward));
    }

    // ppcoin: track money supply and mint amount info
    pindex->nMint = nValueOut - nValueIn + nFees;
    pindex->nMoneySupply = (pindex->pprev? pindex->pprev->nMoneySupply : 0) + nValueOut - nValueIn;
    if (!txdb.WriteBlockIndex(CDiskBlockIndex(pindex)))
        return error("Connect() : WriteBlockIndex for pindex failed");

    if (fJustCheck)
        return true;

    // Write queued txindex changes
    for (map<uint256, CTxIndex>::iterator mi = mapQueuedChanges.begin(); mi != mapQueuedChanges.end(); ++mi)
    {
        if (!txdb.UpdateTxIndex((*mi).first, (*mi).second))
            return error("ConnectBlock() : UpdateTxIndex failed");
    }

    if(GetBoolArg("-addrindex", false))
    {
        // Write Address Index
        BOOST_FOREACH(CTransaction& tx, vtx)
        {
            uint256 hashTx = tx.GetHash();
            // inputs
            if(!tx.IsCoinBase())
            {
                MapPrevTx mapInputs;
                map<uint256, CTxIndex> mapQueuedChangesT;
                bool fInvalid;
                if (!tx.FetchInputs(txdb, mapQueuedChangesT, true, false, mapInputs, fInvalid))
                    return false;

                MapPrevTx::const_iterator mi;
                for(MapPrevTx::const_iterator mi = mapInputs.begin(); mi != mapInputs.end(); ++mi)
                {
                    BOOST_FOREACH(const CTxOut &atxout, (*mi).second.second.vout)
                    {
                        std::vector<uint160> addrIds;
                        if(BuildAddrIndex(atxout.scriptPubKey, addrIds))
                        {
                            BOOST_FOREACH(uint160 addrId, addrIds)
                            {
                                if(!txdb.WriteAddrIndex(addrId, hashTx))
                                    LogPrintf("ConnectBlock(): txins WriteAddrIndex failed addrId: %s txhash: %s\n", addrId.ToString().c_str(), hashTx.ToString().c_str());
                            }
                        }
                    }
                }
            }

            // outputs
            BOOST_FOREACH(const CTxOut &atxout, tx.vout)
            {
                std::vector<uint160> addrIds;
                if(BuildAddrIndex(atxout.scriptPubKey, addrIds))
                {
                    BOOST_FOREACH(uint160 addrId, addrIds)
                    {
                        if(!txdb.WriteAddrIndex(addrId, hashTx))
                            LogPrintf("ConnectBlock(): txouts WriteAddrIndex failed addrId: %s txhash: %s\n", addrId.ToString().c_str(), hashTx.ToString().c_str());
                    }
                }
            }
        }
    }

    // Update block index on disk without changing it in memory.
    // The memory index structure will be changed after the db commits.
    if (pindex->pprev)
    {
        CDiskBlockIndex blockindexPrev(pindex->pprev);
        blockindexPrev.hashNext = pindex->GetBlockHash();
        if (!txdb.WriteBlockIndex(blockindexPrev))
            return error("ConnectBlock() : WriteBlockIndex failed");
    }

    // Watch for transactions paying to me
    BOOST_FOREACH(CTransaction& tx, vtx)
        SyncWithWallets(tx, this);

    return true;
}

bool CBlock::ReadFromDisk(const CBlockIndex* pindex, bool fReadTransactions)
{
    if (!fReadTransactions)
    {
        *this = pindex->GetBlockHeader();
        return true;
    }
    if (!ReadFromDisk(pindex->nFile, pindex->nBlockPos, fReadTransactions))
        return false;
    if (GetHash() != pindex->GetBlockHash())
        return error("CBlock::ReadFromDisk() : GetHash() doesn't match index");
    return true;
}

bool CBlock::SetBestChain(CTxDB& txdb, CBlockIndex* pindexNew)
{
    uint256 hash = GetHash();

    if (!txdb.TxnBegin())
        return error("SetBestChain() : TxnBegin failed");

    if (pindexGenesisBlock == NULL && hash == Params().HashGenesisBlock())
    {
        txdb.WriteHashBestChain(hash);
        if (!txdb.TxnCommit())
            return error("SetBestChain() : TxnCommit failed");
        pindexGenesisBlock = pindexNew;
    }
    else if (hashPrevBlock == hashBestChain)
    {
        if (!SetBestChainInner(txdb, pindexNew))
            return error("SetBestChain() : SetBestChainInner failed");
    }
    else
    {
        // the first block in the new chain that will cause it to become the new best chain
        CBlockIndex *pindexIntermediate = pindexNew;

        // list of blocks that need to be connected afterwards
        std::vector<CBlockIndex*> vpindexSecondary;

        // Reorganize is costly in terms of db load, as it works in a single db transaction.
        // Try to limit how much needs to be done inside
        while (pindexIntermediate->pprev && pindexIntermediate->pprev->nChainTrust > pindexBest->nChainTrust)
        {
            vpindexSecondary.push_back(pindexIntermediate);
            pindexIntermediate = pindexIntermediate->pprev;
        }

        if (!vpindexSecondary.empty())
            LogPrintf("Postponing %u reconnects\n", vpindexSecondary.size());

        // Switch to new best branch
        if (!Reorganize(txdb, pindexIntermediate))
        {
            txdb.TxnAbort();
            InvalidChainFound(pindexNew);
            return error("SetBestChain() : Reorganize failed");
        }

        // Connect further blocks
        BOOST_REVERSE_FOREACH(CBlockIndex *pindex, vpindexSecondary)
        {
            CBlock block;
            if (!block.ReadFromDisk(pindex))
            {
                LogPrintf("SetBestChain() : ReadFromDisk failed\n");
                break;
            }
            if (!txdb.TxnBegin()) {
                LogPrintf("SetBestChain() : TxnBegin 2 failed\n");
                break;
            }
            // errors now are not fatal, we still did a reorganisation to a new chain in a valid way
            if (!block.SetBestChainInner(txdb, pindex))
                break;
        }
    }

    // Update best block in wallet (so we can detect restored wallets)
    bool fIsInitialDownload = IsInitialBlockDownload();
    if ((pindexNew->nHeight % 20160) == 0 || (!fIsInitialDownload && (pindexNew->nHeight % 144) == 0))
    {
        const CBlockLocator locator(pindexNew);
        g_signals.SetBestChain(locator);
    }

    // New best block
    hashBestChain = hash;
    pindexBest = pindexNew;
    pblockindexFBBHLast = NULL;
    nBestHeight = pindexBest->nHeight;
    nBestChainTrust = pindexNew->nChainTrust;
    nTimeBestReceived = GetTime();
    mempool.AddTransactionsUpdated(1);

    uint256 nBestBlockTrust = pindexBest->nHeight != 0 ? (pindexBest->nChainTrust - pindexBest->pprev->nChainTrust) : pindexBest->nChainTrust;

    LogPrintf("SetBestChain: new best=%s  height=%d  trust=%s  blocktrust=%d  date=%s\n",
      hashBestChain.ToString(), nBestHeight,
      CBigNum(nBestChainTrust).ToString(),
      nBestBlockTrust.Get64(),
      DateTimeStrFormat("%x %H:%M:%S", pindexBest->GetBlockTime()));

    // Check the version of the last 100 blocks to see if we need to upgrade:
    if (!fIsInitialDownload)
    {
        int nUpgraded = 0;
        const CBlockIndex* pindex = pindexBest;
        for (int i = 0; i < 100 && pindex != NULL; i++)
        {
            if (pindex->nVersion > CBlock::CURRENT_VERSION)
                ++nUpgraded;
            pindex = pindex->pprev;
        }
        if (nUpgraded > 0)
            LogPrintf("SetBestChain: %d of last 100 blocks above version %d\n", nUpgraded, (int)CBlock::CURRENT_VERSION);
        if (nUpgraded > 100/2)
            // strMiscWarning is read by GetWarnings(), called by Qt and the JSON-RPC code to warn the user:
            strMiscWarning = _("Warning: This version is obsolete, upgrade required!");
    }

    std::string strCmd = GetArg("-blocknotify", "");

    if (!fIsInitialDownload && !strCmd.empty())
    {
        boost::replace_all(strCmd, "%s", hashBestChain.GetHex());
        boost::thread t(runCommand, strCmd); // thread runs free
    }

    return true;
}

bool CBlock::AddToBlockIndex(unsigned int nFile, unsigned int nBlockPos, const uint256& hashProof)
{
    AssertLockHeld(cs_main);

    // Check for duplicate
    uint256 hash = GetHash();
    if (mapBlockIndex.count(hash))
        return error("AddToBlockIndex() : %s already exists", hash.ToString());

    // Construct new block index object
    CBlockIndex* pindexNew = new CBlockIndex(nFile, nBlockPos, *this);
     {
          LOCK(cs_nBlockSequenceId);
          pindexNew->nSequenceId = nBlockSequenceId++;
     }
    if (!pindexNew)
        return error("AddToBlockIndex() : new CBlockIndex failed");
    pindexNew->phashBlock = &hash;
    map<uint256, CBlockIndex*>::iterator miPrev = mapBlockIndex.find(hashPrevBlock);
    if (miPrev != mapBlockIndex.end())
    {
        pindexNew->pprev = (*miPrev).second;
        pindexNew->nHeight = pindexNew->pprev->nHeight + 1;
    }

    // ppcoin: compute chain trust score
    pindexNew->nChainTrust = (pindexNew->pprev ? pindexNew->pprev->nChainTrust : 0) + pindexNew->GetBlockTrust();

    // ppcoin: compute stake entropy bit for stake modifier
    if (!pindexNew->SetStakeEntropyBit(GetStakeEntropyBit()))
        return error("AddToBlockIndex() : SetStakeEntropyBit() failed");

    // Record proof hash value
    pindexNew->hashProof = hashProof;

    // ppcoin: compute stake modifier
    uint64_t nStakeModifier = 0;
    bool fGeneratedStakeModifier = false;
    if (!ComputeNextStakeModifier(pindexNew->pprev, nStakeModifier, fGeneratedStakeModifier))
        return error("AddToBlockIndex() : ComputeNextStakeModifier() failed");
    pindexNew->SetStakeModifier(nStakeModifier, fGeneratedStakeModifier);
    pindexNew->bnStakeModifierV2 = ComputeStakeModifierV2(pindexNew->pprev, IsProofOfWork() ? hash : vtx[1].vin[0].prevout.hash);

    // Add to mapBlockIndex
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
    if (pindexNew->IsProofOfStake())
        setStakeSeen.insert(make_pair(pindexNew->prevoutStake, pindexNew->nStakeTime));
    pindexNew->phashBlock = &((*mi).first);

    // Write to disk block index
    CTxDB txdb;
    if (!txdb.TxnBegin())
        return false;
    txdb.WriteBlockIndex(CDiskBlockIndex(pindexNew));
    if (!txdb.TxnCommit())
        return false;

    // New best
    if (pindexNew->nChainTrust > nBestChainTrust)
        if (!SetBestChain(txdb, pindexNew))
            return false;

    if (pindexNew == pindexBest)
    {
        // Notify UI to display prev block's coinbase if it was ours
        static uint256 hashPrevBestCoinBase;
        g_signals.UpdatedTransaction(hashPrevBestCoinBase);
        hashPrevBestCoinBase = vtx[0].GetHash();
    }

    return true;
}

bool CBlock::CheckBlock(bool fCheckPOW, bool fCheckMerkleRoot, bool fCheckSig) const
{
    // These are checks that are independent of context
    // that can be verified before saving an orphan block.

    // Size limits
    if (vtx.empty() || vtx.size() > MAX_BLOCK_SIZE || ::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return DoS(100, error("CheckBlock() : size limits failed"));

    // Check proof of work matches claimed amount
    if (fCheckPOW && IsProofOfWork() && !CheckProofOfWork(GetPoWHash(), nBits))
        return DoS(50, error("CheckBlock() : proof of work failed"));

    // Check timestamp
    if (GetBlockTime() > FutureDrift(GetAdjustedTime()))
        return error("CheckBlock() : block timestamp too far in the future");

    // First transaction must be coinbase, the rest must not be
    if (vtx.empty() || !vtx[0].IsCoinBase())
        return DoS(100, error("CheckBlock() : first tx is not coinbase"));
    for (unsigned int i = 1; i < vtx.size(); i++)
        if (vtx[i].IsCoinBase())
            return DoS(100, error("CheckBlock() : more than one coinbase"));

    if (IsProofOfStake())
    {
        // Coinbase output should be empty if proof-of-stake block
        if (vtx[0].vout.size() != 1 || !vtx[0].vout[0].IsEmpty())
            return DoS(100, error("CheckBlock() : coinbase output not empty for proof-of-stake block"));

        // Second transaction must be coinstake, the rest must not be
        if (vtx.empty() || !vtx[1].IsCoinStake())
            return DoS(100, error("CheckBlock() : second tx is not coinstake"));
        for (unsigned int i = 2; i < vtx.size(); i++)
            if (vtx[i].IsCoinStake())
                return DoS(100, error("CheckBlock() : more than one coinstake"));
    }

    // Check proof-of-stake block signature
    if (fCheckSig && !CheckBlockSignature())
        return DoS(100, error("CheckBlock() : bad proof-of-stake block signature"));

    // ----------- instantX transaction scanning -----------

    if(IsSporkActive(SPORK_3_INSTANTX_BLOCK_FILTERING)){
        BOOST_FOREACH(const CTransaction& tx, vtx){
            if (!tx.IsCoinBase()){
                //only reject blocks when it's based on complete consensus
                BOOST_FOREACH(const CTxIn& in, tx.vin){
                    if(mapLockedInputs.count(in.prevout)){
                        if(mapLockedInputs[in.prevout] != tx.GetHash()){
                            if(fDebug) { LogPrintf("CheckBlock() : found conflicting transaction with transaction lock %s %s\n", mapLockedInputs[in.prevout].ToString().c_str(), tx.GetHash().ToString().c_str()); }
                            return DoS(0, error("CheckBlock() : found conflicting transaction with transaction lock"));
                        }
                    }
                }
            }
        }
    } else{
        if(fDebug) { LogPrintf("CheckBlock() : skipping transaction locking checks\n"); }
    }

    // ----------- masternode / devops - payments -----------

    bool MasternodePayments = false;
    bool fIsInitialDownload = IsInitialBlockDownload();

    int64_t cTime = nTime;
    int64_t mTime = START_MASTERNODE_PAYMENTS;
    if(cTime > mTime) MasternodePayments = true;
    if (!fIsInitialDownload)
    {
        if(MasternodePayments)
        {
            LOCK2(cs_main, mempool.cs);

            CBlockIndex *pindex = pindexBest;
            if(IsProofOfStake() && pindex != NULL){
                if(pindex->GetBlockHash() == hashPrevBlock){
                    // If we don't already have its previous block, skip masternode payment step
                    CAmount masternodePaymentAmount;
                    for (int i = vtx[1].vout.size(); i--> 0; ) {
                        masternodePaymentAmount = vtx[1].vout[i].nValue;
                        break;
                    }
                    bool foundPaymentAmount = false;
                    bool foundPayee = false;
                    bool foundPaymentAndPayee = false;

                    CScript payee;
                    CTxIn vin;
                    if(!masternodePayments.GetWinningMasternode(pindexBest->nHeight+1, payee, vin) || payee == CScript()){
                        foundPayee = true; //doesn't require a specific payee
                        foundPaymentAmount = true;
                        foundPaymentAndPayee = true;
                        if(fDebug) { LogPrintf("CheckBlock() : Using non-specific masternode payments %d\n", pindexBest->nHeight+1); }
                    }

                    for (unsigned int i = 0; i < vtx[1].vout.size(); i++) {
                        if(vtx[1].vout[i].nValue == masternodePaymentAmount )
                            foundPaymentAmount = true;
                        if(vtx[1].vout[i].scriptPubKey == payee )
                            foundPayee = true;
                        if(vtx[1].vout[i].nValue == masternodePaymentAmount && vtx[1].vout[i].scriptPubKey == payee)
                            foundPaymentAndPayee = true;
                    }

                    CTxDestination address1;
                    ExtractDestination(payee, address1);
                    CDigitalNoteAddress address2(address1);

                    if(!foundPaymentAndPayee) {
                        if(fDebug) { LogPrintf("CheckBlock() : Couldn't find masternode payment(%d|%d) or payee(%d|%s) nHeight %d. \n", foundPaymentAmount, masternodePaymentAmount, foundPayee, address2.ToString().c_str(), pindexBest->nHeight+1); }
                        return DoS(100, error("CheckBlock() : Couldn't find masternode payment or payee"));
                    } else {
                        LogPrintf("CheckBlock() : Found payment(%d|%d) or payee(%d|%s) nHeight %d. \n", foundPaymentAmount, masternodePaymentAmount, foundPayee, address2.ToString().c_str(), pindexBest->nHeight+1);
                    }
                } else {
                    if(fDebug) { LogPrintf("CheckBlock() : Skipping masternode payment check - nHeight %d Hash %s\n", pindexBest->nHeight+1, GetHash().ToString().c_str()); }
                }
            } else {
                if(fDebug) { LogPrintf("CheckBlock() : pindex is null, skipping masternode payment check\n"); }
            }
        } else {
            if(fDebug) { LogPrintf("CheckBlock() : skipping masternode payment checks\n"); }
        }
    } else {
        if(fDebug) { LogPrintf("CheckBlock() : Is initial download, skipping masternode payment check %d\n", pindexBest->nHeight+1); }
    }

    // Verify coinbase/coinstake tx includes devops payment -
    // first check for start of devops payments
    bool bDevOpsPayment = false;

    if ( Params().NetworkID() == CChainParams::TESTNET ){
        if (GetTime() > START_DEVOPS_PAYMENTS_TESTNET ){
            bDevOpsPayment = true;
        }
    }else{
        if (GetTime() > START_DEVOPS_PAYMENTS){
            bDevOpsPayment = true;
        }
    }
    // stop devops payments (for testing)
    if ( Params().NetworkID() == CChainParams::TESTNET ){
        if (GetTime() > STOP_DEVOPS_PAYMENTS_TESTNET ){
            bDevOpsPayment = false;
        }
    }else{
        if (GetTime() > STOP_DEVOPS_PAYMENTS){
            bDevOpsPayment = false;
        }
    }
    // Fork toggle for payment upgrade
    if(pindexBest->GetBlockTime() > 0)
    {
        if(pindexBest->GetBlockTime() > nPaymentUpdate_1) // Monday, May 20, 2019 12:00:00 AM
        {
            bDevOpsPayment = true;
        }
        else
        {
            bDevOpsPayment = false;
        }
    }
    else
    {
        bDevOpsPayment = false;
    }
    // Run checks if at fork height
    if(bDevOpsPayment)
    {
        int64_t nStandardPayment = 0;
        int64_t nMasternodePayment = 0;
        int64_t nDevopsPayment = 0;
        int64_t nProofOfIndexMasternode = 0;
        int64_t nProofOfIndexDevops = 0;
        int64_t nMasterNodeChecksDelay = 45 * 60;
        int64_t nMasterNodeChecksEngageTime = 0;
        const CBlockIndex* pindexPrev = pindexBest->pprev;
        bool isProofOfStake = !IsProofOfWork();
        bool fBlockHasPayments = true;
        std::string strVfyDevopsAddress;
        // Define primitives depending if PoW/PoS
        if (isProofOfStake) {
            nProofOfIndexMasternode = 2;
            nProofOfIndexDevops = 3;
            if (vtx[isProofOfStake].vout.size() != 4) {
                if (vtx[isProofOfStake].vout.size() != 5) {
                    LogPrintf("CheckBlock() : PoS submission doesn't include devops and/or masternode payment\n");
                    fBlockHasPayments = false;
                } else {
                    nProofOfIndexMasternode = 3;
                    nProofOfIndexDevops = 4;
                }
            }
            nStandardPayment = GetProofOfStakeReward(pindexPrev, 0, 0);
        } else {
            nProofOfIndexMasternode = 1;
            nProofOfIndexDevops = 2;
            if (vtx[isProofOfStake].vout.size() != 3) {
                    LogPrintf("CheckBlock() : PoW submission doesn't include devops and/or masternode payment\n");
                    fBlockHasPayments = false;
            }
            nStandardPayment = GetProofOfWorkReward(nBestHeight, 0);
        }
        // Set payout values depending if PoW/PoS
        nMasternodePayment = GetMasternodePayment(pindexBest->nHeight, nStandardPayment) / COIN;
        nDevopsPayment = GetDevOpsPayment(pindexBest->nHeight, nStandardPayment) / COIN;
        LogPrintf("Hardset MasternodePayment: %lu | Hardset DevOpsPayment: %lu \n", nMasternodePayment, nDevopsPayment);
        // Increase time for Masternode checks delay during sync per-block
        if (fIsInitialDownload) {
            nMasterNodeChecksDelayBaseTime = GetTime();
        } else {
            nMasterNodeChecksEngageTime = nMasterNodeChecksDelayBaseTime + nMasterNodeChecksDelay;
        }
        // Devops Address Set and Updates
        strVfyDevopsAddress = "dHy3LZvqX5B2rAAoLiA7Y7rpvkLXKTkD18";
        if(pindexBest->GetBlockTime() < nPaymentUpdate_2) { strVfyDevopsAddress = Params().DevOpsAddress(); }
        // Check PoW or PoS payments for current block
        for (unsigned int i=0; i < vtx[isProofOfStake].vout.size(); i++) {
            // Define values
            CScript rawPayee = vtx[isProofOfStake].vout[i].scriptPubKey;
            CTxDestination address;
            ExtractDestination(vtx[isProofOfStake].vout[i].scriptPubKey, address);
            CBitcoinAddress addressOut(address);
            int64_t nAmount = vtx[isProofOfStake].vout[i].nValue / COIN;
            int64_t nIndexedMasternodePayment = vtx[isProofOfStake].vout[nProofOfIndexMasternode].nValue / COIN;
            int64_t nIndexedDevopsPayment = vtx[isProofOfStake].vout[nProofOfIndexDevops].nValue / COIN;
            LogPrintf(" - vtx[%d].vout[%d] Address: %s Amount: %lu \n", isProofOfStake, i, addressOut.ToString(), nAmount);
            // PoS Checks
            if (isProofOfStake) {
                // Check for PoS masternode payment
                if (i == nProofOfIndexMasternode) {
                   if (mnodeman.IsPayeeAValidMasternode(rawPayee) && fMNselect(pindexBest->nHeight)) {
                       LogPrintf("CheckBlock() : PoS Recipient masternode address validity succesfully verified\n");
                   } else if (addressOut.ToString() == strVfyDevopsAddress) {
                       LogPrintf("CheckBlock() : PoS Recipient masternode address validity succesfully verified\n");
                   } else {
                       if (nMasterNodeChecksEngageTime != 0) {
                           if (fMnAdvRelay) {
                               LogPrintf("CheckBlock() : PoS Recipient masternode address validity could not be verified\n");
                               fBlockHasPayments = false;
                           } else {
                               LogPrintf("CheckBlock() : PoS Recipient masternode address validity skipping, Checks delay still active!\n");
                           }
                       }
                   }
                   if (nIndexedMasternodePayment == nMasternodePayment) {
                       LogPrintf("CheckBlock() : PoS Recipient masternode amount validity succesfully verified\n");
                   } else {
                       LogPrintf("CheckBlock() : PoS Recipient masternode amount validity could not be verified\n");
                       fBlockHasPayments = false;
                   }
                }
                // Check for PoS devops payment
                if (i == nProofOfIndexDevops) {
                   if (addressOut.ToString() == strVfyDevopsAddress) {
                       LogPrintf("CheckBlock() : PoS Recipient devops address validity succesfully verified\n");
                   } else {
                       LogPrintf("CheckBlock() : PoS Recipient devops address validity could not be verified\n");
                       // Skip check during transition to new DevOps
                       if (pindexBest->GetBlockTime() < nPaymentUpdate_3) {
                           // Check legacy blocks for valid payment, only skip for Update_2
                           if (pindexBest->GetBlockTime() < nPaymentUpdate_2) {
                               fBlockHasPayments = false;
                           }
                       } else {
                           // Re-enable enforcement post transition (Update_3)
                           fBlockHasPayments = false;
                       }
                   }
                   if (nIndexedDevopsPayment == nDevopsPayment) {
                       LogPrintf("CheckBlock() : PoS Recipient devops amount validity succesfully verified\n");
                   } else {
                       if (pindexBest->GetBlockTime() < nPaymentUpdate_2) {
                           LogPrintf("CheckBlock() : PoS Recipient devops amount validity could not be verified\n");
                           fBlockHasPayments = false;
                       } else {
                           if (nIndexedDevopsPayment >= nDevopsPayment) {
                               LogPrintf("CheckBlock() : PoS Reciepient devops amount is abnormal due to large fee paid");
                           } else {
                               LogPrintf("CheckBlock() : PoS Reciepient devops amount validity could not be verified");
                               fBlockHasPayments = false;
                           }
                       }
                   }
                }
            }
            // PoW Checks
            else if (!isProofOfStake) {
                // Check for PoW masternode payment
                if (i == nProofOfIndexMasternode) {
                   if (mnodeman.IsPayeeAValidMasternode(rawPayee) && fMNselect(pindexBest->nHeight)) {
                      LogPrintf("CheckBlock() : PoW Recipient masternode address validity succesfully verified\n");
                   } else if (addressOut.ToString() == strVfyDevopsAddress) {
                      LogPrintf("CheckBlock() : PoW Recipient masternode address validity succesfully verified\n");
                   } else {
                      if (nMasterNodeChecksEngageTime != 0) {
                          if (fMnAdvRelay) {
                              LogPrintf("CheckBlock() : PoW Recipient masternode address validity could not be verified\n");
                              fBlockHasPayments = false;
                          } else {
                              LogPrintf("CheckBlock() : PoW Recipient masternode address validity skipping, Checks delay still active!\n");
                          }
                      }
                   }
                   if (nAmount == nMasternodePayment) {
                      LogPrintf("CheckBlock() : PoW Recipient masternode amount validity succesfully verified\n");
                   } else {
                      LogPrintf("CheckBlock() : PoW Recipient masternode amount validity could not be verified\n");
                      fBlockHasPayments = false;
                   }
                }
                // Check for PoW devops payment
                if (i == nProofOfIndexDevops) {
                   if (addressOut.ToString() == strVfyDevopsAddress) {
                      LogPrintf("CheckBlock() : PoW Recipient devops address validity succesfully verified\n");
                   } else {
                       LogPrintf("CheckBlock() : PoW Recipient devops address validity could not be verified\n");
                       // Skip check during transition to new DevOps
                       if (pindexBest->GetBlockTime() < nPaymentUpdate_3) {
                           // Check legacy blocks for valid payment, only skip for Update_2
                           if (pindexBest->GetBlockTime() < nPaymentUpdate_2) {
                               fBlockHasPayments = false;
                           }
                       } else {
                           // Re-enable enforcement post transition (Update_3)
                           fBlockHasPayments = false;
                       }
                   }
                   if (nAmount == nDevopsPayment) {
                      LogPrintf("CheckBlock() : PoW Recipient devops amount validity succesfully verified\n");
                   } else {
                       if (pindexBest->GetBlockTime() < nPaymentUpdate_2) {
                           LogPrintf("CheckBlock() : PoW Recipient devops amount validity could not be verified\n");
                           fBlockHasPayments = false;
                       } else {
                           if (nIndexedDevopsPayment >= nDevopsPayment) {
                               LogPrintf("CheckBlock() : PoW Reciepient devops amount is abnormal due to large fee paid");
                           } else {
                               LogPrintf("CheckBlock() : PoW Reciepient devops amount validity could not be verified");
                               fBlockHasPayments = false;
                           }
                       }
                   }
                }
            }
        }
        // Final checks (DevOps/Masternode payments)
        if (fBlockHasPayments) {
            LogPrintf("CheckBlock() : PoW/PoS non-miner reward payments succesfully verified\n");
        } else {
            LogPrintf("CheckBlock() : PoW/PoS non-miner reward payments could not be verified\n");
            return DoS(100, error("CheckBlock() : PoW/PoS invalid payments in current block\n"));
        }
    }

    // Check transactions
    BOOST_FOREACH(const CTransaction& tx, vtx)
    {
        if (!tx.CheckTransaction())
            return DoS(tx.nDoS, error("CheckBlock() : CheckTransaction failed"));

        // ppcoin: check transaction timestamp
        if (GetBlockTime() < (int64_t)tx.nTime)
            return DoS(50, error("CheckBlock() : block timestamp earlier than transaction timestamp"));
    }

    // Check for duplicate txids. This is caught by ConnectInputs(),
    // but catching it earlier avoids a potential DoS attack:
    set<uint256> uniqueTx;
    BOOST_FOREACH(const CTransaction& tx, vtx)
    {
        uniqueTx.insert(tx.GetHash());
    }
    if (uniqueTx.size() != vtx.size())
        return DoS(100, error("CheckBlock() : duplicate transaction"));

    unsigned int nSigOps = 0;
    BOOST_FOREACH(const CTransaction& tx, vtx)
    {
        nSigOps += GetLegacySigOpCount(tx);
    }
    if (nSigOps > MAX_BLOCK_SIGOPS)
        return DoS(100, error("CheckBlock() : out-of-bounds SigOpCount"));

    // Check merkle root
    if (fCheckMerkleRoot && hashMerkleRoot != BuildMerkleTree())
        return DoS(100, error("CheckBlock() : hashMerkleRoot mismatch"));


    return true;
}

bool CBlock::AcceptBlock()
{
    AssertLockHeld(cs_main);

    // Check for duplicate
    uint256 hash = GetHash();
    if (mapBlockIndex.count(hash))
        return error("AcceptBlock() : block already in mapBlockIndex");

    // Get prev block index
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashPrevBlock);
    if (mi == mapBlockIndex.end())
        return DoS(10, error("AcceptBlock() : prev block not found"));
    CBlockIndex* pindexPrev = (*mi).second;
    int nHeight = pindexPrev->nHeight+1;

    // Check created block for version control
    if (nVersion < 7)
        return DoS(100, error("AcceptBlock() : reject too old nVersion = %d", nVersion));
    else if (nVersion > 7)
        return DoS(100, error("AcceptBlock() : reject too new nVersion = %d", nVersion));

    // Check block against Velocity parameters
    if(Velocity_check(nHeight))
    {
        // Announce Velocity constraint failure
        if(!Velocity(pindexPrev, this))
        {
            return DoS(100, error("AcceptBlock() : Velocity rejected block %d, required parameters not met", nHeight));
        }
    }

    uint256 hashProof;
    if (IsProofOfWork() && nHeight > Params().EndPoWBlock()){
        return DoS(100, error("AcceptBlock() : reject proof-of-work at height %d", nHeight));
    } else {
        // PoW is checked in CheckBlock()
        if (IsProofOfWork())
        {
            hashProof = GetPoWHash();
        }
    }

    if (IsProofOfStake() && nHeight < Params().StartPoSBlock())
        return DoS(100, error("AcceptBlock() : reject proof-of-stake at height <= %d", nHeight));

    // Check coinbase timestamp
    if (GetBlockTime() > FutureDrift((int64_t)vtx[0].nTime) && IsProofOfStake())
        return DoS(50, error("AcceptBlock() : coinbase timestamp is too early"));

    // Check coinstake timestamp
    if (IsProofOfStake() && !CheckCoinStakeTimestamp(nHeight, GetBlockTime(), (int64_t)vtx[1].nTime))
        return DoS(50, error("AcceptBlock() : coinstake timestamp violation nTimeBlock=%d nTimeTx=%u", GetBlockTime(), vtx[1].nTime));

    // Check proof-of-work or proof-of-stake
    if (nBits != GetNextTargetRequired(pindexPrev, IsProofOfStake()))
        return DoS(100, error("AcceptBlock() : incorrect %s", IsProofOfWork() ? "proof-of-work" : "proof-of-stake"));

    // Check timestamp against prev
    if (GetBlockTime() <= pindexPrev->GetPastTimeLimit() || FutureDrift(GetBlockTime()) < pindexPrev->GetBlockTime())
        return error("AcceptBlock() : block's timestamp is too early");

    // Check that all transactions are finalized
    BOOST_FOREACH(const CTransaction& tx, vtx)
        if (!IsFinalTx(tx, nHeight, GetBlockTime()))
            return DoS(10, error("AcceptBlock() : contains a non-final transaction"));

    // Check that the block chain matches the known block chain up to a checkpoint
    if (!Checkpoints::CheckHardened(nHeight, hash))
        return DoS(100, error("AcceptBlock() : rejected by hardened checkpoint lock-in at %d", nHeight));

    // Verify hash target and signature of coinstake tx
    if (IsProofOfStake())
    {
        uint256 targetProofOfStake;
        if (!CheckProofOfStake(pindexPrev, vtx[1], nBits, hashProof, targetProofOfStake))
        {
            return error("AcceptBlock() : check proof-of-stake failed for block %s", hash.ToString());
        }
    }

    // Check that the block satisfies synchronized checkpoint
    if (!Checkpoints::CheckSync(nHeight))
        return error("AcceptBlock() : rejected by synchronized checkpoint");

    // Enforce rule that the coinbase starts with serialized block height
    CScript expect = CScript() << nHeight;
    if (vtx[0].vin[0].scriptSig.size() < expect.size() ||
        !std::equal(expect.begin(), expect.end(), vtx[0].vin[0].scriptSig.begin()))
        return DoS(100, error("AcceptBlock() : block height mismatch in coinbase"));

    // Write block to history file
    if (!CheckDiskSpace(::GetSerializeSize(*this, SER_DISK, CLIENT_VERSION)))
        return error("AcceptBlock() : out of disk space");
    unsigned int nFile = -1;
    unsigned int nBlockPos = 0;
    if (!WriteToDisk(nFile, nBlockPos))
        return error("AcceptBlock() : WriteToDisk failed");
    if (!AddToBlockIndex(nFile, nBlockPos, hashProof))
        return error("AcceptBlock() : AddToBlockIndex failed");

    // Relay inventory, but don't relay old inventory during initial block download
    int nBlockEstimate = Checkpoints::GetTotalBlocksEstimate();
    if (hashBestChain == hash)
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            if (nBestHeight > (pnode->nStartingHeight != -1 ? pnode->nStartingHeight - 2000 : nBlockEstimate))
                pnode->PushInventory(CInv(MSG_BLOCK, hash));
    }

    return true;
}

#ifdef ENABLE_WALLET
// novacoin: attempt to generate suitable proof-of-stake
bool CBlock::SignBlock(CWallet& wallet, int64_t nFees)
{
    // if we are trying to sign
    //    something except proof-of-stake block template
    if (!vtx[0].vout[0].IsEmpty())
        return false;

    // if we are trying to sign
    //    a complete proof-of-stake block
    if (IsProofOfStake())
        return true;

    static int64_t nLastCoinStakeSearchTime = GetAdjustedTime(); // startup timestamp

    CKey key;
    CTransaction txCoinStake;
    txCoinStake.nTime &= ~STAKE_TIMESTAMP_MASK;

    int64_t nSearchTime = txCoinStake.nTime; // search to current time

    if (nSearchTime > nLastCoinStakeSearchTime)
    {
        int64_t nSearchInterval = 1;
        if (wallet.CreateCoinStake(wallet, nBits, nSearchInterval, nFees, txCoinStake, key))
        {
            if (txCoinStake.nTime >= pindexBest->GetPastTimeLimit()+1)
            {
                // make sure coinstake would meet timestamp protocol
                //    as it would be the same as the block timestamp
                vtx[0].nTime = nTime = txCoinStake.nTime;

                // we have to make sure that we have no future timestamps in
                //    our transactions set
                for (vector<CTransaction>::iterator it = vtx.begin(); it != vtx.end();)
                    if (it->nTime > nTime) { it = vtx.erase(it); } else { ++it; }

                vtx.insert(vtx.begin() + 1, txCoinStake);
                hashMerkleRoot = BuildMerkleTree();

                // append a signature to our block
                return key.Sign(GetHash(), vchBlockSig);
            }
        }
        nLastCoinStakeSearchInterval = nSearchTime - nLastCoinStakeSearchTime;
        nLastCoinStakeSearchTime = nSearchTime;
    }

    return false;
}
#endif

bool CBlock::CheckBlockSignature() const
{
    if (IsProofOfWork())
        return vchBlockSig.empty();

    if (vchBlockSig.empty())
        return false;

    vector<valtype> vSolutions;
    txnouttype whichType;

    const CTxOut& txout = vtx[1].vout[1];

    if (!Solver(txout.scriptPubKey, whichType, vSolutions))
        return false;

    if (whichType == TX_PUBKEY)
    {
        valtype& vchPubKey = vSolutions[0];
        return CPubKey(vchPubKey).Verify(GetHash(), vchBlockSig);
    }

    return false;
}

void CBlock::RebuildAddressIndex(CTxDB& txdb)
{
    BOOST_FOREACH(CTransaction& tx, vtx)
    {
        uint256 hashTx = tx.GetHash();
        // inputs
        if(!tx.IsCoinBase())
        {
            MapPrevTx mapInputs;
            map<uint256, CTxIndex> mapQueuedChangesT;
            bool fInvalid;
            if (!tx.FetchInputs(txdb, mapQueuedChangesT, true, false, mapInputs, fInvalid))
                return;

            MapPrevTx::const_iterator mi;
            for(MapPrevTx::const_iterator mi = mapInputs.begin(); mi != mapInputs.end(); ++mi)
            {
                BOOST_FOREACH(const CTxOut &atxout, (*mi).second.second.vout)
                {
                    std::vector<uint160> addrIds;
                    if(BuildAddrIndex(atxout.scriptPubKey, addrIds))
                    {
                        BOOST_FOREACH(uint160 addrId, addrIds)
                        {
                            if(!txdb.WriteAddrIndex(addrId, hashTx))
                                LogPrintf("RebuildAddressIndex(): txins WriteAddrIndex failed addrId: %s txhash: %s\n", addrId.ToString().c_str(), hashTx.ToString().c_str());
                        }
                    }
                }
            }
        }
        // outputs
        BOOST_FOREACH(const CTxOut &atxout, tx.vout) {
            std::vector<uint160> addrIds;
            if(BuildAddrIndex(atxout.scriptPubKey, addrIds))
            {
                BOOST_FOREACH(uint160 addrId, addrIds)
                {
                    if(!txdb.WriteAddrIndex(addrId, hashTx))
                        LogPrintf("RebuildAddressIndex(): txouts WriteAddrIndex failed addrId: %s txhash: %s\n", addrId.ToString().c_str(), hashTx.ToString().c_str());
                }
            }
        }
    }
}

// Called from inside SetBestChain: attaches a block to the new best chain being built
bool CBlock::SetBestChainInner(CTxDB& txdb, CBlockIndex *pindexNew)
{
    uint256 hash = GetHash();

    // Adding to current best branch
    if (!ConnectBlock(txdb, pindexNew) || !txdb.WriteHashBestChain(hash))
    {
        txdb.TxnAbort();
        InvalidChainFound(pindexNew);
        return false;
    }
    if (!txdb.TxnCommit())
        return error("SetBestChain() : TxnCommit failed");

    // Add to current best branch
    pindexNew->pprev->pnext = pindexNew;

    // Delete redundant memory transactions
    BOOST_FOREACH(CTransaction& tx, vtx)
        mempool.remove(tx);

    return true;
}

