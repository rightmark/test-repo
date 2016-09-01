///////////////////////////////////////////////////////////////////////////////
//  Server statistics class.
//

#pragma once


template<UINT t_nInterval = 1000>
class ATL_NO_VTABLE CStatManager
{
public:
    CStatManager() {}
    ~CStatManager() throw() {}

    UINT AddConnStat(void) throw()
    {
        InterlockedIncrement(&ms_nDirty);
        return InterlockedIncrement(&ms_ConnCount);
    }
    UINT DelConnStat(void) throw()
    {
        InterlockedIncrement(&ms_nDirty);
        return InterlockedDecrement(&ms_ConnCount);
    }

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
    bool DisplayData(void) throw()
    {
        static UINT64 lasttick = 0;

        static UINT64 lastread = 0;
        static UINT64 lastsent = 0;

        UINT64 tick = ::GetTickCount64();
        LONG chgcnt = ms_nDirty;

        if (chgcnt && tick > lasttick)
        {
            if (ms_ConnCount < (UINT)-1) // @WARNING: no connections available for datagrams..
            {
                MSG(0, _T("+ connections: %u\n"), ms_ConnCount);
            }

            if (lastread > 0 && lastsent > 0)
            {
                UINT64 readspeed = 1000 * (ms_BytesRead - lastread) / (tick - lasttick);
                UINT64 sentspeed = 1000 * (ms_BytesSent - lastsent) / (tick - lasttick);

                MSG(0, _T("-> bytes read: %I64u (%I64u bps)\n"), ms_BytesRead, readspeed);
                MSG(0, _T("<- bytes sent: %I64u (%I64u bps)\n"), ms_BytesSent, sentspeed);
            } 
            else
            {
                lastread = ms_BytesRead;
                lastsent = ms_BytesSent;

                MSG(0, _T("-> bytes read: %I64u\n"), ms_BytesRead);
                MSG(0, _T("<- bytes sent: %I64u\n"), ms_BytesSent);
            }

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
        ms_ConnCount = c;
    }

private:
    static LONG volatile ms_nDirty;         // data changes counter

    // Statistics counters   
    static UINT64 volatile ms_BytesRead;
    static UINT64 volatile ms_BytesSent;
    static UINT32 volatile ms_ConnCount;    // number of concurrent connections

};

template <UINT t> LONG volatile CStatManager<t>::ms_nDirty(0);

template <UINT t> UINT64 volatile CStatManager<t>::ms_BytesRead(0);
template <UINT t> UINT64 volatile CStatManager<t>::ms_BytesSent(0);
template <UINT t> UINT32 volatile CStatManager<t>::ms_ConnCount(0);
