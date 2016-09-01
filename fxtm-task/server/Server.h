#pragma once

// tweaks
#define _USE_REUSEADDR



#include "StatManager.h"
#include "WorkerThread.h"


__interface IServer
{
    long Connect(PADDRINFOT);
    long Disconnect(void);
    long IoControl(long, bool);
    long Run(void);
    void Destroy(void);
};


template <class T, class Log>
class ATL_NO_VTABLE CServerBase
    : public CStatManager<STAT_INTERVAL>
    , public IServer
{
public:
    CServerBase()
    {
        ATLTRACE(_T("CServer.ctor()\n"));
    }
    ~CServerBase() throw()
    {
        ATLTRACE(_T("CServer.dtor()\n"));
    }

    // IServer
    long Connect(PADDRINFOT /*pai*/) throw()
    {
        ATLTRACENOTIMPL(_T("CServer.Connect()"));
    }
    long Disconnect(void) throw()
    {
        ERR(_T("\nTerminated: %s\n"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%X")));

        return ERROR_SUCCESS;
    }
    long IoControl(long, bool) throw()
    {
        ATLTRACENOTIMPL(_T("CServer.IoControl()"));
    }
    long Run(void) throw()
    {
        ATLTRACE(_T("CServer.Run() must be implemented.\n"));
        ATLASSERT(false);
        return ERROR_SUCCESS;
    }
    void Destroy(void) throw()
    {
        delete static_cast<T*>(this);
    }

protected:
    // helper methods
    int DisplayError(LPCTSTR msg, int err = ::WSAGetLastError()) throw()
    {
        return Log::Error(msg, err);
    }

    LPCTSTR FStr(int family) const throw()
    {
        switch (family)
        {
        case AF_INET:
            return _T("AF_INET");
        case AF_INET6:
            return _T("AF_INET6");
        default:
            return _T("AF_UNSPEC");
        }
    }

private:

};


//////////////////////////////////////////////////////////////////////////
// TCP server implementation
//

class CServerTcp : public CServerBase<CServerTcp, CLog>
{
    static const size_t MAXIMUM_CONCURRENT_CONNECTIONS = 256;//4000;

public:
    CServerTcp() : m_MaxSockCnt(MAXIMUM_CONCURRENT_CONNECTIONS)
    {
        ATLTRACE(_T("CServerTcp.ctor()\n"));
    }
    ~CServerTcp() throw()
    {
        ATLTRACE(_T("CServerTcp.dtor()\n"));
    }

    // IServer

    long Connect(PADDRINFOT pai) throw()
    {
#ifdef _DEBUG
        MSG(0, _T("Greetings from TCP Server!\n\n"));
#endif
        ERR(_T("\nStarted: %s\n"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%X")));

        if (!CreateWorkers()) return errno; // cannot start threads

        int idx = 0;
        for (; pai != NULL; pai = pai->ai_next)
        {
            CSocketAsync socket;

            if ((pai->ai_family != AF_INET) && (pai->ai_family != AF_INET6)) continue;

            if (!socket.Create(pai))
            {
                DisplayError(_T("CSocketAsync.Create() failed."));
                continue;
            }

            if ((pai->ai_family == PF_INET6) && IN6_IS_ADDR_LINKLOCAL((PIN6_ADDR)INETADDR_ADDRESS(pai->ai_addr)) && (((PSOCKADDR_IN6)(pai->ai_addr))->sin6_scope_id == 0))
            {
                ERR(_T("IPv6 link local addresses should specify a scope ID.\n"));
            }

#ifdef _USE_REUSEADDR
            // @TRICKY: debugging trick that allows restart the server immediately after shut down;
            // otherwise it have to wait ~20 secs. avoids binding error "Address already in use".
            BOOL bEnable = TRUE;
            if (!socket.Option(SO_REUSEADDR, bEnable))
            {
                DisplayError(_T("CSocketAsync.Option() failed."));
            }
#endif

            if (!socket.Bind(pai))
            {
                DisplayError(_T("CSocketAsync.Bind() failed."));
                continue;
            }

            if (!socket.Listen())
            {
                DisplayError(_T("CSocketAsync.Listen() failed."));
                continue;
            }

            if (m_listener.Accept(socket))
            {
                socket.Detach(); // prevent closing..
                idx++;

                MSG(0, _T("Listening: port %u, (TCP, %s)\n"), ntohs(SS_PORT(pai->ai_addr)), FStr(pai->ai_family));
            }

        } // for()

        if (idx == 0)
        {
            ERR(_T("Fatal error: unable to serve on any address.\n"));
            return ERROR_FATAL_APP_EXIT;
        }

        return ERROR_SUCCESS; WSA_E_NO_MORE;
    }

    long Run(void) throw()
    {
        return ERROR_SUCCESS;
    }

protected:
    // helper methods
    bool CreateWorkers(void) throw()
    {
        size_t m = CWorkThreadTcp::MaxConnections();
        size_t n = (m > 0)? ((m_MaxSockCnt + m - 1) / m) : 1;

        for (size_t i = 0; i < n; ++i)
        {
            m_worker.push_back(move(new CWorkThreadTcp));

            if (!m_worker[i]->Create(false)) return false;
        }
        m_listener.SetWorkers(m_worker);

        return m_listener.Create(true);
    }


public:
    CListenThread m_listener;
    // @WARNING: worker objects are self-destructive
    std::vector<CWorkThreadTcp*> m_worker;

private:
    size_t m_MaxSockCnt; // maximum concurrent connections
};


//////////////////////////////////////////////////////////////////////////
// UDP server implementation

class CServerUdp : public CServerBase<CServerUdp, CLog>
{
public:
    CServerUdp()
        : m_bCircular(false)
    {
        ATLTRACE(_T("CServerUdp.ctor()\n"));
    }
    ~CServerUdp() throw()
    {
        ATLTRACE(_T("CServerUdp.dtor()\n"));
    }

    // IServer

    long Connect(PADDRINFOT pai) throw()
    {
#ifdef _DEBUG
        MSG(0, _T("Greetings from UDP Server!\n\n"));
#endif
        ERR(_T("\nStarted: %s\n"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%X")));

        if (!m_worker.Create(false)) return errno; // cannot start thread

        int idx = 0;
        for (; pai != NULL; pai = pai->ai_next)
        {
            CSocketAsync socket;

            if ((pai->ai_family != AF_INET) && (pai->ai_family != AF_INET6)) continue;

            if (!socket.Create(pai))
            {
                DisplayError(_T("CSocketAsync.Create() failed."));
                continue;
            }

            if ((pai->ai_family == PF_INET6) && IN6_IS_ADDR_LINKLOCAL((PIN6_ADDR)INETADDR_ADDRESS(pai->ai_addr)) && (((PSOCKADDR_IN6)(pai->ai_addr))->sin6_scope_id == 0))
            {
                ERR(_T("IPv6 link local addresses should specify a scope ID.\n"));
            }

#ifdef _USE_REUSEADDR
            // @TRICKY: debugging trick that allows restart the server immediately after shut down;
            // otherwise it have to wait ~20 secs. avoids binding error "Address already in use".
            {
                BOOL bEnable = TRUE;
                if (!socket.Option(SO_REUSEADDR, bEnable))
                {
                    DisplayError(_T("CSocketAsync.Option() failed."));
                }
            }
#endif

            if (!socket.Bind(pai))
            {
                DisplayError(_T("CSocketAsync.Bind() failed."));
                continue;
            }

            if (m_bCircular)
            {
                // @WARNING: this IOCTL is only valid for sockets associated with unreliable, message-oriented protocols.
                BOOL bEnable = TRUE;
                if (!socket.IOCtl(SIO_ENABLE_CIRCULAR_QUEUEING, bEnable))
                {
                    DisplayError(_T("CSocketAsync.IOCtl() failed."));
                }
            }

            if (m_worker.Accept(socket))
            {
                socket.Detach(); // prevent closing..
                idx++;

                MSG(0, _T("Listening: port %u, (UDP, %s)\n"), ntohs(SS_PORT(pai->ai_addr)), FStr(pai->ai_family));
            }

        } // for()

        if (idx == 0)
        {
            ERR(_T("Fatal error: unable to serve on any address.\n"));
            return -1; // WSANO_DATA ??
        }
        Reset((ULONG)-1); // statistics..

        return ERROR_SUCCESS; WSA_E_NO_MORE;
    }
    long IoControl(long code, bool bEnable) throw()
    {
        switch (code)
        {
        case SIO_ENABLE_CIRCULAR_QUEUEING:
            m_bCircular = bEnable;
            break;
        default:
            return ERROR_INVALID_PARAMETER;
        }
        return ERROR_SUCCESS;
    }
    long Run(void) throw()
    {
        return ERROR_SUCCESS;
    }


public:
    CWorkThreadUdp m_worker;

private:
    bool m_bCircular;
};

