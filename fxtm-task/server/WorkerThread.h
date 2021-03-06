
#pragma once


#ifndef __WSAHELP_H__
    #error 'wsahelper.h must be included first
#endif

using namespace WSA;

#include "Transmit.h"



template<class T, class Transmit, DWORD t_N = WSA_MAXIMUM_WAIT_EVENTS>
class ATL_NO_VTABLE CWorkThreadBase
    : public Transmit
    , public CQuit
{
    typedef ATL::CComAutoCriticalSection AutoCriticalSection;
    typedef ATL::CComCritSecLock<AutoCriticalSection> AutoLock;

protected:
    static const size_t THREAD_CONNECTIONS = t_N;

public:
    // @WARNING: all parameters are subjects for tuning..
    static const DWORD WORKER_DELAY = 50; // ms
    static const DWORD WSA_WAIT = 5; // ms

public:
    CWorkThreadBase()
        : m_nConnSockets(0)
        , m_h(NULL)
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CWorkThreadBase.ctor()\n"));
    }
    ~CWorkThreadBase() throw()
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CWorkThreadBase.dtor()\n"));

        T* pT = static_cast<T*>(this);
        pT->Destroy();
    }

    // methods
    bool Accept(SOCKET s) throw()
    {
        if (m_h != NULL)
        {
            AutoLock _(m_cs);

            T* pT = static_cast<T*>(this);
            if (pT->HasPlace())
            {
                m_ConnPending.push_back(s);
                // Checks the suspend count of the thread. If the suspend count is zero, the thread is not currently suspended.
                // Otherwise, the subject thread's suspend count is decremented. If the resulting value is zero,
                // then the execution of the subject thread is resumed.
                ::ResumeThread(m_h);

                return true;
            }
        }
        return false;
    }
    bool Create(bool bRun) throw()
    {
        if (m_h == NULL)
        {
            m_h = (HANDLE)_beginthreadex(NULL, 0, _ThreadProc, this, bRun ? 0 : CREATE_SUSPENDED, NULL);
        }
        return (NULL != m_h);
    }

public:
    // overridables

    bool HasPlace(void) const throw()
    {
        // @WARNING: reading of aligned field is atomic. synchronization is not required.
        // see Intel 325462, 8.1.1 Guaranteed Atomic Operations
        return (THREAD_CONNECTIONS > (m_nConnSockets + m_ConnPending.size()));
    }

    void Destroy(void) throw() {}

    UINT ThreadProc(void) throw()
    {
        ATLTRACE(_T(">> Worker thread run (%p)\n"), this);

        while (CQuit::run())
        {
            LONG events = (FD_CLOSE | FD_READ | FD_WRITE);

            if (m_nConnSockets < 1 && !AcceptHelper(events))
            {
                ::Sleep(WORKER_DELAY);
                continue;
            }

            for (DWORD offs = 0; offs < m_nConnSockets; offs += WSA_MAXIMUM_WAIT_EVENTS)
            {
                if (CQuit::yes()) break;

                LPCWSAEVENT pev = m_sockEvents[offs];

                DWORD cnt = min((DWORD)(m_nConnSockets - offs), (DWORD)WSA_MAXIMUM_WAIT_EVENTS);
                DWORD idx = ::WSAWaitForMultipleEvents(cnt, pev, FALSE, WSA_WAIT, FALSE);

                if (idx == WSA_WAIT_FAILED)
                {
                    DisplayError(_T("WSAWaitForMultipleEvents() failed."));
                    continue;
                }
                else if (idx == WSA_WAIT_TIMEOUT)
                {
                    if (CQuit::yes()) break;

                    AcceptHelper(events);
                    continue;
                }

                // one socket fired event
                idx = idx + offs - WSA_WAIT_EVENT_0;

                WSANETWORKEVENTS ne = {0};

                if (!m_arrSockets[idx].EnumEvents(m_sockEvents[idx], &ne))
                {
                    DisplayError(_T("CSocketAsync.EnumEvents() failed."));
                    continue;
                }
                else if (ne.lNetworkEvents & FD_READ)
                {
                    MSG(2, _T("FD_READ event fired\n"));

                    if (ne.iErrorCode[FD_READ_BIT] != 0)
                    {
                        DisplayError(_T("FD_READ failed."), ne.iErrorCode[FD_READ_BIT]);
                        continue;
                    }

                    if (ReadData(m_arrSockets[idx]) == SOCKET_ERROR)
                    {
                        Remove(idx);
                    }
                }
                else if (ne.lNetworkEvents & FD_WRITE)
                {
                    MSG(2, _T("FD_WRITE event fired\n"));

                    if (ne.iErrorCode[FD_WRITE_BIT] != 0)
                    {
                        DisplayError(_T("FD_WRITE failed."), ne.iErrorCode[FD_WRITE_BIT]);
                        continue;
                    }

                    if (SendData(m_arrSockets[idx]) == SOCKET_ERROR)
                    {
                        Remove(idx);
                    }
                }
                else if (ne.lNetworkEvents & FD_CLOSE)
                {
                    MSG(2, _T("FD_CLOSE event fired\n"));

                    if (ne.iErrorCode[FD_CLOSE_BIT] != 0)
                    {
                        DisplayError(_T("FD_CLOSE failed."), ne.iErrorCode[FD_CLOSE_BIT]);

                        WSAECONNRESET; // connection was reset by the remote side
                        WSAECONNABORTED; // connection was terminated due to a timeout or other failure
                    }
                    m_arrSockets[idx].Shutdown(SD_SEND);

                    Remove(idx);
                }

            } // for (offs)

        } // while()

        ATLTRACE(_T("<< Worker thread exited (%p)\n"), this);

        return ERROR_SUCCESS;
    }

protected:
    // helpers
    void Lock(void) throw() { m_cs.Lock(); }
    void Unlock(void) throw() { m_cs.Unlock(); }

    bool AcceptHelper(LONG events) throw()
    {
        AutoLock _(m_cs);

        if (m_ConnPending.empty()) return false;

        UINT m = m_nConnSockets;

        while (!m_ConnPending.empty())
        {
            if (m_sockEvents[m].Create())
            {
                m_arrSockets[m].Attach(m_ConnPending.front());
                m_arrSockets[m].SelectEvents(m_sockEvents[m], events);
            }
            m_ConnPending.pop_front(); ++m;
        }
        m_nConnSockets = m;

        return true;
    }

    bool Remove(UINT n) throw()
    {
        UINT m = m_nConnSockets - 1;

        if (m_nConnSockets < 1 || n > m) return false;

        Lock();

        if (n != m)
        {
            m_arrSockets[n] = m_arrSockets[m];
            m_sockEvents[n] = m_sockEvents[m];
        }
        else
        {
            m_arrSockets[m].Close();
            m_sockEvents[m].Close();
        }

        m_nConnSockets = m;

        Unlock();

        CStatManager::DelConnStat(); // statistics..

        return true;
    }

    void ShutThread(void) throw()
    {
        while (m_h != NULL)
        {
            if (CQuit::run()) { CQuit::set(); }

            ::ResumeThread(m_h);
            ::Sleep(1); // give others a chance..
        }
    }

private:
    static UINT WINAPI _ThreadProc(void* pv)
    {
        T* pT = static_cast<T*>(pv);
        UINT uRet = pT->ThreadProc();

        pT->m_h = NULL; // Zed's dead..

        return uRet;
    }

public:
    static size_t MaxConnections(void) throw()
    {
        return THREAD_CONNECTIONS;
    }

protected:
    UINT m_nConnSockets; // number of connected sockets

    // good for memory usage
    std::list<SOCKET> m_ConnPending;

    CSocketAsync m_arrSockets[THREAD_CONNECTIONS];
    CSocketEvent m_sockEvents[THREAD_CONNECTIONS];

private:
    HANDLE m_h; // thread handle

    AutoCriticalSection m_cs;
};


class CWorkThreadTcp : public CWorkThreadBase<CWorkThreadTcp, CTransmitTcp, WSA_MAXIMUM_WAIT_EVENTS * 16/* 1024 */>
{
public:
    CWorkThreadTcp()
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CWorkThreadTcp.ctor()\n"));
    }
    ~CWorkThreadTcp() throw()
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CWorkThreadTcp.dtor()\n"));
    }

public:
    // overridables

    void Destroy(void) throw() { ShutThread(); }

private:
};


class CWorkThreadUdp : public CWorkThreadBase<CWorkThreadUdp, CTransmitUdp, 2/*FD_SETSIZE*/>
{
public:
    CWorkThreadUdp()
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CWorkThreadUdp.ctor()\n"));
    }
    ~CWorkThreadUdp() throw()
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CWorkThreadUdp.dtor()\n"));
    }

private:
};


class CListenThread : public CWorkThreadBase<CListenThread, CTransmitTcp, 2/*FD_SETSIZE*/>
{
    typedef std::vector<unique_ptr<CWorkThreadTcp>> TWorkerList;

public:
    CListenThread()
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CListenThread.ctor()\n"));
    }
    ~CListenThread() throw()
    {
        ATLTRACE(atlTraceRefcount, 0, _T("CListenThread.dtor()\n"));
    }

    // methods
    bool SetWorkers(size_t n) throw()
    {
        try
        {
            m_worker.reserve(n * 2);

            for (size_t i = 0; i < n; ++i)
            {
                m_worker.emplace_back(new CWorkThreadTcp);

                if (!m_worker.back()->Create(false)) return false;
            }
        }
        catch (std::bad_alloc& e)
        {
            ERR(_T("[Listener] bad_alloc caught: %s\n"), (LPCTSTR)CString(e.what()));
            return false;
        }
        return true;
    }

public:
    // overridables

    bool HasPlace(void) const throw()
    {
        // @WARNING: reading of aligned field is atomic. synchronization is not required.
        // see Intel 325462, 8.1.1 Guaranteed Atomic Operations
        return (WSA_MAXIMUM_WAIT_EVENTS > (m_nConnSockets + m_ConnPending.size()));
    }

    UINT ThreadProc(void) throw()
    {
        ATLTRACE(_T(">> Listener thread run (%p)\n"), this);

        while (CQuit::run())
        {
            LONG events = (FD_ACCEPT | FD_CLOSE /*| FD_CONNECT*/);

            if (m_nConnSockets < 1 && !AcceptHelper(events))
            {
                ::Sleep(WORKER_DELAY);
                continue;
            }

            DWORD idx = ::WSAWaitForMultipleEvents(m_nConnSockets, m_sockEvents[0], FALSE, WORKER_DELAY, FALSE);
  
            if (idx == WSA_WAIT_FAILED)
            {
                DisplayError(_T("WSAWaitForMultipleEvents() failed."));
                continue;
            }
            else if (idx == WSA_WAIT_TIMEOUT)
            {
                if (CQuit::yes()) break;

                AcceptHelper(events);
                continue;
            }

            idx -= WSA_WAIT_EVENT_0;

            WSANETWORKEVENTS ne = {0};
            if (!m_arrSockets[idx].EnumEvents(m_sockEvents[idx], &ne))
            {
                DisplayError(_T("CSocketAsync.EnumEvents() failed."));
                continue;
            }
            else if (ne.lNetworkEvents & FD_ACCEPT)
            {
                MSG(3, _T("FD_ACCEPT event fired\n"));
                if (ne.iErrorCode[FD_ACCEPT_BIT] != 0)
                {
                    DisplayError(_T("FD_ACCEPT failed."), ne.iErrorCode[FD_ACCEPT_BIT]);
                    continue;
                }

                TCHAR hostname[MAX_ADDRSTRLEN];

                SOCKADDR_STORAGE from;
                int fromlen = sizeof(from);

                // for TCP, accept the connection and hand off the client socket to a worker thread
                //

                DWORD err = ERROR_SUCCESS;
                SOCKET ConnSock = INVALID_SOCKET;
                if (!m_arrSockets[idx].Accept(&from, &fromlen, ConnSock))
                {
                    if ((err = ::WSAGetLastError()) != WSAEWOULDBLOCK)
                    {
                        DisplayError(_T("CSocketAsync.Accept() failed."), err);
                    }
                    continue;
                }

                err = ::GetNameInfo((PSOCKADDR)&from, fromlen, hostname, _countof(hostname), NULL, 0, NI_NUMERICHOST);
                if (err != NO_ERROR)
                {
                    DisplayError(_T("GetNameInfo() failed."), err);
                    _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
                }
                MSG(0, _T("Accepted connection from %s, port %u\n"), hostname, ntohs(SS_PORT(&from)));

                try
                {
                    TWorkerList& w = m_worker;

                    bool bFound = false;
                    const int tries = 4; // @TODO: km 20160901 - TBD..

                    for (int i = 0; i < tries && !bFound; ++i)
                    {
                        for (const auto& it : w)
                        {
                            if (it->Accept(ConnSock))
                            {
                                bFound = true; break; // free worker found..
                            }
                        }

                        if (!bFound) // free worker not found
                        {
                            w.emplace_back(new CWorkThreadTcp);

                            w.back()->Create(false);
                        }
                    } // for ()

                    if (bFound) { CStatManager::AddConnStat(); } // statistics.. 
                }
                catch (std::bad_alloc& e)
                {
                    ERR(_T("[Listener] bad_alloc caught: %s\n"), (LPCTSTR)CString(e.what()));
                }
                catch (...)
                {
                    ERR(_T("[Listener] unspecified exception.\n"));
                }

            }
            else if (ne.lNetworkEvents & FD_CLOSE)
            {
                MSG(3, _T("FD_CLOSE event fired (listener)\n"));
                if (ne.iErrorCode[FD_CLOSE_BIT] != 0)
                {
                    DisplayError(_T("FD_CLOSE failed."), ne.iErrorCode[FD_CLOSE_BIT]);
                    continue;
                }
                // @TODO: km 20160824 - TBD..
            } 
            else if (ne.lNetworkEvents & FD_CONNECT)
            {
                MSG(3, _T("FD_CONNECT event fired\n"));
            } 

        } // while(bQuit)

        ATLTRACE(_T("<< Listener thread exited (%p)\n"), this);

        return ERROR_SUCCESS;
    }

private:
    TWorkerList m_worker;
};


/*
Graceful Shutdown and Socket Closure

(1) client invokes shutdown(s, SD_SEND) to signal end of session and that client has no more data to send.	
(2) server receives FD_CLOSE, indicating graceful shutdown in progress and that all data has been received.
(3) server sends any remaining response data.
    (local timing significance only) client gets FD_READ and calls recv() to get any response data sent by server.
(4) server invokes shutdown(s, SD_SEND) to indicate server has no more data to send.
(5) client receives FD_CLOSE indication.	
    (local timing significance only) server invokes closesocket().
(6) client invokes closesocket().
*/