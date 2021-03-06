#pragma once

// tweaks
#define _LOG_CONNECTIONS
#define _USE_REUSEADDR
#define _USE_UDP_STATS


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


template<class T, class Log>
class ATL_NO_VTABLE CServerBase
    : public CStatManager<STAT_INTERVAL>
    , public IServer
{
public:
    CServerBase()
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CServer.ctor()\n"));
    }
    ~CServerBase() throw()
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CServer.dtor()\n"));
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
    static const size_t MAXIMUM_CONCURRENT_CONNECTIONS = 4096;

public:
    CServerTcp() : m_MaxSockCnt(MAXIMUM_CONCURRENT_CONNECTIONS)
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CServerTcp.ctor()\n"));
    }
    ~CServerTcp() throw()
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CServerTcp.dtor()\n"));
    }

    // IServer

    long Connect(PADDRINFOT pai) throw()
    {
#ifdef _DEBUG
        MSG(0, _T("Greetings from TCP Server!\n\n"));
#endif
        ERR(_T("Started: %s\n\n"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%X")));
        ERR(_T("connections maximum: %Iu, connections per thread: %Iu, connection wait, ms: %u\n\n"), m_MaxSockCnt, CWorkThreadTcp::MaxConnections(), CWorkThreadTcp::WSA_WAIT);

        if (!CreateWorkers()) return errno; // cannot start threads
        // errno is set to EAGAIN if there are too many threads,
        // to EINVAL if the argument is invalid or the stack size is incorrect,
        // or to EACCES if there are insufficient resources(such as memory).

        Reset(); // statistics..

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

            if ((pai->ai_family == PF_INET6) && IN6_IS_ADDR_LINKLOCAL((PIN6_ADDR)INETADDR_ADDRESS(pai->ai_addr)) && (INETADDR_SCOPE_ID(pai->ai_addr).Value == 0))
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

    // methods

    void SetMaxConnections(size_t maxconn) throw()
    {
        if (maxconn > 0) { m_MaxSockCnt = maxconn; }
    }

protected:
    // helper methods
    bool CreateWorkers(void) throw()
    {
        size_t m = CWorkThreadTcp::MaxConnections();
        size_t n = (m > 0)? ((m_MaxSockCnt + m - 1) / m) : 1;

        if (m_listener.SetWorkers(n))
        {
            return m_listener.Create(true);
        }
        return false;
    }


public:
    CListenThread m_listener;

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
        ATLTRACE(atlTraceRefcount, 0, _T("CServerUdp.ctor()\n"));
    }
    ~CServerUdp() throw()
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CServerUdp.dtor()\n"));
    }

    // IServer

    long Connect(PADDRINFOT pai) throw()
    {
#ifdef _DEBUG
        MSG(0, _T("Greetings from UDP Server!\n\n"));
#endif
        ERR(_T("Started: %s\n\n"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%X")));

        if (!m_worker.Create(false)) return errno; // cannot start thread
        // errno is set to EAGAIN if there are too many threads,
        // to EINVAL if the argument is invalid or the stack size is incorrect,
        // or to EACCES if there are insufficient resources(such as memory).

#ifndef _USE_UDP_STATS
        Reset((ULONG)-1); // statistics..
#endif

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

            if ((pai->ai_family == PF_INET6) && IN6_IS_ADDR_LINKLOCAL((PIN6_ADDR)INETADDR_ADDRESS(pai->ai_addr)) && (INETADDR_SCOPE_ID(pai->ai_addr).Value == 0))
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

        return ERROR_SUCCESS;
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

