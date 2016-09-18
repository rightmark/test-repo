///////////////////////////////////////////////////////////////////////////////
//  Client statistics class.
//

#pragma once


template<UINT t_nInterval = 1000>
class ATL_NO_VTABLE CStatManager
{
public:
    CStatManager() {}
    ~CStatManager() throw() {}

    void ReadMoreData(UINT64 add) throw()
    {
        InterlockedIncrement(&ms_nDirty);
        InterlockedExchangeAdd(&ms_BytesRead, add);
    }
    void SentMoreData(UINT64 add) throw()
    {
        InterlockedIncrement(&ms_nDirty);
        InterlockedExchangeAdd(&ms_BytesSent, add);
    }
    bool DisplayData(bool bForce) throw()
    {
        static UINT64 lasttick = 0;

        UINT64 tick = ::GetTickCount64();
        LONG chgcnt = ms_nDirty;

        if (bForce || (chgcnt > 0 && tick > lasttick))
        {
            MSG(0, _T("-> bytes read: %I64u\n"), ms_BytesRead);
            MSG(0, _T("<- bytes sent: %I64u\n"), ms_BytesSent);

            lasttick = tick + t_nInterval;

            InterlockedExchangeAdd(&ms_nDirty, -chgcnt);

            return true;
        }
        return false;
    }
    void Reset(UINT c = 0) throw()
    {
        ms_BytesRead = 0;
        ms_BytesSent = 0;
    }

private:
    static LONG volatile ms_nDirty;         // data changes counter

    // Statistics counters   
    static UINT64 volatile ms_BytesRead;
    static UINT64 volatile ms_BytesSent;

};

template<UINT t> LONG volatile CStatManager<t>::ms_nDirty(0);

template<UINT t> UINT64 volatile CStatManager<t>::ms_BytesRead(0);
template<UINT t> UINT64 volatile CStatManager<t>::ms_BytesSent(0);
