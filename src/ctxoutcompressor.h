#ifndef CTXOUTCOMPRESSOR_H
#define CTXOUTCOMPRESSOR_H

/** wrapper for CTxOut that provides a more compact serialization */
class CTxOutCompressor
{
private:
    CTxOut &txout;

public:
    CTxOutCompressor(CTxOut &txoutIn)
		: txout(txoutIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(txout.nValue));
        CScriptCompressor cscript(REF(txout.scriptPubKey));
        READWRITE(cscript);
    )
};

#endif // CTXOUTCOMPRESSOR_H