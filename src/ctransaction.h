#ifndef CTRANSACTION_H
#define CTRANSACTION_H

#include <vector>
#include <map>

#include "serialize.h"
#include "scrypt.h"

struct CDiskTxPos;
class uint256;
class CTxIn;
class CTxOut;
class CTxDB;
class CTxIndex;
class COutPoint;
class CBlockIndex;
class CTransaction;

typedef std::map<uint256, std::pair<CTxIndex, CTransaction>> MapPrevTx;

/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class CTransaction
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    unsigned int nTime;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    unsigned int nLockTime;

    // Denial-of-service detection:
    mutable int nDoS;
    bool DoS(int nDoSIn, bool fIn) const;

    CTransaction();
    CTransaction(int nVersion, unsigned int nTime, const std::vector<CTxIn>& vin, const std::vector<CTxOut>& vout, unsigned int nLockTime)
        : nVersion(nVersion), nTime(nTime), vin(vin), vout(vout), nLockTime(nLockTime), nDoS(0)
    {};

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nTime);
        READWRITE(vin);
        READWRITE(vout);
        READWRITE(nLockTime);
    )

    void SetNull();
    bool IsNull() const;
    uint256 GetHash() const;
    bool IsCoinBase() const;
    bool IsCoinStake() const;
	
    // Compute priority, given priority of inputs and (optionally) tx size
    double ComputePriority(double dPriorityInputs, unsigned int nTxSize=0) const;

    /** Amount of bitcoins spent by this transaction.
        @return sum of all outputs (note: does not include fees)
     */
    int64_t GetValueOut() const;

    /** Amount of bitcoins coming in to this transaction
        Note that lightweight clients may not know anything besides the hash of previous transactions,
        so may not be able to calculate this.

        @param[in] mapInputs    Map of previous transactions that have outputs we're spending
        @return Sum of value of all inputs (scriptSigs)
        @see CTransaction::FetchInputs
     */
    int64_t GetValueIn(const MapPrevTx& mapInputs) const;

    friend bool operator==(const CTransaction& a, const CTransaction& b);
    friend bool operator!=(const CTransaction& a, const CTransaction& b);
    std::string ToString() const;
	
    bool ReadFromDisk(CDiskTxPos pos, FILE** pfileRet=NULL);
    bool ReadFromDisk(CTxDB& txdb, const uint256& hash, CTxIndex& txindexRet);
    bool ReadFromDisk(CTxDB& txdb, COutPoint prevout, CTxIndex& txindexRet);
    bool ReadFromDisk(CTxDB& txdb, COutPoint prevout);
    bool ReadFromDisk(COutPoint prevout);
    bool DisconnectInputs(CTxDB& txdb);

    /** Fetch from memory and/or disk. inputsRet keys are transaction hashes.

     @param[in] txdb    Transaction database
     @param[in] mapTestPool List of pending changes to the transaction index database
     @param[in] fBlock  True if being called to add a new best-block to the chain
     @param[in] fMiner  True if being called by CreateNewBlock
     @param[out] inputsRet  Pointers to this transaction's inputs
     @param[out] fInvalid   returns true if transaction is invalid
     @return    Returns true if all inputs are in txdb or mapTestPool
     */
    bool FetchInputs(CTxDB& txdb, const std::map<uint256, CTxIndex>& mapTestPool,
                     bool fBlock, bool fMiner, MapPrevTx& inputsRet, bool& fInvalid);

    /** Sanity check previous transactions, then, if all checks succeed,
        mark them as spent by this transaction.

        @param[in] inputs   Previous transactions (from FetchInputs)
        @param[out] mapTestPool Keeps track of inputs that need to be updated on disk
        @param[in] posThisTx    Position of this transaction on disk
        @param[in] pindexBlock
        @param[in] fBlock   true if called from ConnectBlock
        @param[in] fMiner   true if called from CreateNewBlock
        @return Returns true if all checks succeed
     */
    bool ConnectInputs(CTxDB& txdb, MapPrevTx inputs,
                       std::map<uint256, CTxIndex>& mapTestPool, const CDiskTxPos& posThisTx,
                       const CBlockIndex* pindexBlock, bool fBlock, bool fMiner, unsigned int flags = STANDARD_SCRIPT_VERIFY_FLAGS, bool fValidateSig = true);
    bool CheckTransaction() const;
    bool GetCoinAge(CTxDB& txdb, const CBlockIndex* pindexPrev, uint64_t& nCoinAge) const;

    const CTxOut& GetOutputFor(const CTxIn& input, const MapPrevTx& inputs) const;
};

#endif // CTRANSACTION_H