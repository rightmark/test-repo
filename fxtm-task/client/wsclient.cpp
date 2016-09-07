// wsclient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define _CLIENT_BUILD_

#include "apphelper.h"

using namespace std;
using namespace WSA;

#include "StatManager.h"
#include "Task.h"


// CExeModule

class CExeModule
    : public IExeModuleImpl<CExeModule, CLog>
    , public CStatManager<STAT_INTERVAL>
    , public CQuit
{

    static const DWORD
        CLIENT_WAIT   = 10, // ms
        CLOSE_TIMEOUT = 5000,
        RX_TIMEOUT    = 10000;

public:
    CExeModule()
        : m_bPause(false)
        , m_clientid(0)
        , m_requests(1)
        , m_delaytik(0)
        , m_worktime(0)
    {
    }


    // IExeModule
    int Connect(void)
    {

        if (!m_bConnected)
        {
#ifdef _DEBUG
            MSG(0, _T("Greetings from %s Client!\n\n"), PStr(m_socktype));
#endif
            ERR(_T("\nStarted: %s\n"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%X")));
            ERR(_T("parameters: ip=%s, port=%s (%s, %s)\n"), (LPCTSTR)m_addr, (LPCTSTR)m_port, PStr(m_socktype), FStr(m_family));
            ERR(_T("requests: %u, delay ticks: %u, working time: %u\n\n"), m_requests, m_delaytik, m_worktime);

            CAddrInfo ai(m_addr, m_port, m_socktype, m_family);

            TCHAR hostname[MAX_ADDRSTRLEN];
            PADDRINFOT pai = ai;

            for (; pai != NULL; pai = pai->ai_next)
            {
                CSocketAsync socket;

                if ((pai->ai_family != AF_INET) && (pai->ai_family != AF_INET6)) continue;

                if (!socket.Create(pai))
                {
                    DisplayError(_T("CSocketAsync.Create() failed."));
                    continue;
                }

#if 0
                if ((pai->ai_family == PF_INET6) && IN6_IS_ADDR_LINKLOCAL((PIN6_ADDR)INETADDR_ADDRESS(pai->ai_addr)) && (((PSOCKADDR_IN6)(pai->ai_addr))->sin6_scope_id == 0))
                {
                    ERR(_T("IPv6 link local addresses should specify a scope ID.\n"));
                }
#endif

                if (::GetNameInfo(pai->ai_addr, (int)pai->ai_addrlen, hostname, _countof(hostname), NULL, 0, NI_NUMERICHOST) != NO_ERROR)
                {
                    _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
                }

                MSG(0, _T("Attempting to connect to: %s\n"), hostname);

                if (socket.Connect(pai))
                {
                    m_ConnSocket = socket;
                    break;
                }

                DisplayError(_T("CSocketAsync.Connect() failed."));

            } // for()

            if (pai == NULL)
            {
                ERR(_T("Fatal error: unable to connect to the server.\n"));
                return ERROR_FATAL_APP_EXIT;
            }

            // socket connection details..

            LPSOCKADDR psa = NULL;

            // remote address and port
            if (m_ConnSocket.NameInfo(&psa, hostname, _countof(hostname)) != NO_ERROR)
            {
                _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
            }
            MSG(0, _T("Connected to %s, port %u, (%s, %s)\n"), hostname, ntohs(SS_PORT(psa)), PStr(pai->ai_socktype), FStr(pai->ai_family));

            // local address and port the system picked
            SOCKADDR_STORAGE addr;
            int addrlen = sizeof(addr);

            if (getsockname(m_ConnSocket, (LPSOCKADDR)&addr, &addrlen) != SOCKET_ERROR)
            {
                LPCTSTR p = InetNtop(pai->ai_family, INETADDR_ADDRESS((LPSOCKADDR)&addr), hostname, _countof(hostname));
                MSG(0, _T("Using local address %s, port %u\n"), p ? p : UNKNOWN_NAME, ntohs(SS_PORT(&addr)));
            }

            m_bConnected = true;
        }
        return ERROR_SUCCESS; WSA_E_NO_MORE;
    }

    int Disconnect(void) throw()
    {
        if (m_bConnected)
        {
            m_ConnSocket.Shutdown(SD_SEND);

            DWORD idx = ::WSAWaitForMultipleEvents(1, (LPWSAEVENT)&m_ev, FALSE, CLOSE_TIMEOUT, FALSE);

            if (idx == WSA_WAIT_FAILED)
            {
                return DisplayError(_T("WSAWaitForMultipleEvents() failed."));
            }
            else if (idx == WSA_WAIT_TIMEOUT)
            {
                // @TODO: km 20160830 - return on timeout..
            }
            idx -= WSA_WAIT_EVENT_0;

            WSANETWORKEVENTS ne = {0};
            if (!m_ConnSocket.EnumEvents(m_ev, &ne))
            {
                return DisplayError(_T("CSocketAsync.EnumEvents() failed."));
            }
            if (ne.lNetworkEvents & FD_CLOSE)
            {
                MSG(1, _T("FD_CLOSE event fired\n"));
                if (ne.iErrorCode[FD_CLOSE_BIT] != 0)
                {
                    return DisplayError(_T("FD_CLOSE failed."), ne.iErrorCode[FD_CLOSE_BIT]);
                }
            }
            m_ConnSocket.Close();
            m_ev.Close();

            m_bConnected = false;

            ERR(_T("\nTerminated: %s\n"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%X")));
        }

        return ERROR_SUCCESS;
    }

    int Run(void) throw()
    {
        int err = ERROR_SUCCESS;

        // Set Ctrl+C, Ctrl+Break handler
        CCtrlHandler ctrl(_CtrlHandler);

        if (!m_ev.Create())
        {
            return DisplayError(_T("CSocketEvent.Create() failed."));
        }

#if 0
        return RunAsync();
#endif

        m_ConnSocket.SelectEvents(m_ev, /*FD_READ | FD_WRITE |*/ FD_CLOSE);

        // @WARNING: Notice that nothing in this code is specific to whether we are using UDP or TCP.
        //
        // When connect() is called on a datagram socket, it does not actually establish the connection as a stream socket would.
        // Instead, TCP/IP establishes the remote half of the (local IP/port, remote IP/port) mapping.
        // So, it's possible to use send() and recv() on datagram sockets, instead of recvfrom() and sendto().
        //

        UINT64 tick = 0;

        if (m_worktime > 0)
        {
            m_requests = 0; // infinite
            tick = ::GetTickCount64() + m_worktime * 1000;
        }

        for (int i = 0; !m_requests || i < m_requests; ++i)
        {
            // compose a message to send.
            DATABOXT box;
            box.pkg.anchor = ANCHOR;
            box.pkg.keyval = ntohs((USHORT)m_rg.number());

            int len = DATABOX_SIZE_SEND;

            if ((err = SendData(box.buffer, len, RX_TIMEOUT)) == ERROR_TIMEOUT) continue;
            if (err != ERROR_SUCCESS) break; // reconnect ??

            char buf[BUFFER_SIZE] = {0};
            len = sizeof(buf);

            if ((err = ReadData(buf, len, RX_TIMEOUT)) != ERROR_SUCCESS) break; // reconnect ?

            LPDATABOXT pbox = (LPDATABOXT)buf;
            if (pbox->pkg.keyval != box.pkg.keyval)
            {
                ERR(_T("Key mismatch: sent %u, received %u\n"), box.pkg.keyval, pbox->pkg.keyval);
            }

            if (CQuit::yes()) break;

            DisplayData(); // display statistics..

            if (m_delaytik > 0) { ::Sleep(m_delaytik); }

            if (m_worktime > 0 && tick < ::GetTickCount64())
            {
                MSG(0, _T("Working time (%i s) is elapsed\n"), m_worktime);
                break;
            }

        } // for ()

        m_ConnSocket.Shutdown(SD_SEND);

        // Since TCP does not preserve message boundaries, there may still be more data arriving from the server.
        // So we continue to receive data until the server closes the connection.
        //
        if (m_socktype == SOCK_STREAM)
        {
            char buf[BUFFER_SIZE] = {0};
            int len = sizeof(buf);

            while (ReadData(buf, len, RX_TIMEOUT) == ERROR_SUCCESS);
        }
        MSG(0, _T("Done sending\n"));

        return err;
    }


public:
    // overridables

    int ReadConfig(void) throw()
    {
        CPath path;

        if (GetIniFile(path))
        {
            int f = ::GetPrivateProfileInt(_T("client"), _T("family"), 0, path);
            m_family = (f == 4) ? AF_INET : (f == 6) ? AF_INET6 : DEFAULT_FAMILY;

            m_bPause = !!::GetPrivateProfileInt(_T("client"), _T("pause"), m_bPause, path);
            m_delaytik = ::GetPrivateProfileInt(_T("client"), _T("delaytick"), m_delaytik, path);
            m_worktime = ::GetPrivateProfileInt(_T("client"), _T("worktime"), m_worktime, path);

            CString buf;

            if (GetConfigString(buf, path, _T("ipaddress"))) { m_addr = buf; }

            if (GetConfigString(buf, path, _T("port"))) { m_port = buf; }

            if (GetConfigString(buf, path, _T("protocol")))
            {
                if (buf.CompareNoCase(_T("TCP")) == 0)
                {
                    m_socktype = SOCK_STREAM;
                }
                else if (buf.CompareNoCase(_T("UDP")) == 0)
                {
                    m_socktype = SOCK_DGRAM;
                }
            }

        }

        return ERROR_SUCCESS;
    }

protected:
    // helper methods

    int RunAsync(void) throw()
    {
        if (!m_ConnSocket.SelectEvents(m_ev, FD_READ | FD_WRITE | FD_CLOSE))
        {
            return DisplayError(_T("CSocketAsync.SelectEvents() failed."));
        }

        for (;;)
        {
            DWORD idx = ::WSAWaitForMultipleEvents(1, (LPWSAEVENT)&m_ev, FALSE, RX_TIMEOUT, FALSE);

            if (idx == WSA_WAIT_FAILED)
            {
                return DisplayError(_T("WSAWaitForMultipleEvents() failed."));
            }
            else if (idx == WSA_WAIT_TIMEOUT)
            {
                return ERROR_TIMEOUT;
            }

            WSANETWORKEVENTS ne = { 0 };
            if (!m_ConnSocket.EnumEvents(m_ev, &ne))
            {
                return DisplayError(_T("CSocketAsync.EnumEvents() failed."));
            }

            if (ne.lNetworkEvents & FD_READ)
            {
                MSG(1, _T("FD_READ event fired\n"));

                if (ne.iErrorCode[FD_READ_BIT] != 0)
                {
                    return DisplayError(_T("FD_READ failed."), ne.iErrorCode[FD_READ_BIT]);
                }

                // @TODO: km 20160906 - write code..

            }
            else if (ne.lNetworkEvents & FD_WRITE)
            {
                MSG(1, _T("FD_WRITE event fired\n"));

                if (ne.iErrorCode[FD_WRITE_BIT] != 0)
                {
                    return DisplayError(_T("FD_WRITE failed."), ne.iErrorCode[FD_WRITE_BIT]);
                }

                // @TODO: km 20160906 - write code..
            }

        } // for ()

        return ERROR_SUCCESS;
    }
    // @TODO: km 20160831 - work-flow should be detailed
#if 0
    int SendDataAsync(const char* buf, int& len, DWORD timeout) throw()
    {

        for (;;)
        {
            int bytes = 0;

            if ((bytes = send(m_ConnSocket, buf, len, 0)) > 0 || ::WSAGetLastError() != WSAEWOULDBLOCK)
                break;

            DWORD idx = ::WSAWaitForMultipleEvents(1, (LPWSAEVENT)&m_ev, FALSE, timeout, FALSE);

            if (idx == WSA_WAIT_FAILED)
            {
                return DisplayError(_T("WSAWaitForMultipleEvents() failed."));
            }
            else if (idx == WSA_WAIT_TIMEOUT)
            {
                return ERROR_TIMEOUT;
            }


            WSANETWORKEVENTS ne = {0};
            if (!m_ConnSocket.EnumEvents(m_ev, &ne))
            {
                return DisplayError(_T("CSocketAsync.EnumEvents() failed."));
            }

            if (ne.lNetworkEvents & FD_WRITE)
            {
                MSG(1, _T("FD_WRITE event fired\n"));

                if (ne.iErrorCode[FD_WRITE_BIT] != 0)
                {
                    return DisplayError(_T("FD_WRITE failed."), ne.iErrorCode[FD_WRITE_BIT]);
                }

                len = send(m_ConnSocket, buf, len, 0);
                break;
            }

        } // for ()

        return ERROR_SUCCESS;
    }
#endif

    // @TODO: km 20160831 - work-flow should be detailed
#if 0
    int ReadDataAsync(char* buff, int& len, DWORD timeout) throw()
    {

        for (;;)
        {
            DWORD idx = ::WSAWaitForMultipleEvents(1, (LPWSAEVENT)&m_ev, FALSE, timeout, FALSE);

            if (idx == WSA_WAIT_FAILED)
            {
                return DisplayError(_T("WSAWaitForMultipleEvents() failed."));
            }
            else if (idx == WSA_WAIT_TIMEOUT)
            {
                return ERROR_TIMEOUT;
            }


            WSANETWORKEVENTS ne = {0};
            if (!m_ConnSocket.EnumEvents(m_ev, &ne))
            {
                return DisplayError(_T("CSocketAsync.EnumEvents() failed."));
            }

            if (ne.lNetworkEvents & FD_READ)
            {
                MSG(1, _T("FD_READ event fired\n"));

                if (ne.iErrorCode[FD_READ_BIT] != 0)
                {
                    return DisplayError(_T("FD_READ failed."), ne.iErrorCode[FD_READ_BIT]);
                }

                if ((len = recv(m_ConnSocket, buff, len, 0)) == WSAEWOULDBLOCK)
                {
                    len = 0;
                    return WSA_E_NO_MORE;
                }
            }

        } // for ()

        return ERROR_SUCCESS;
    }
#endif

    int SendData(const char* buf, int& len, DWORD timeout) throw()
    {
        UINT64 tick = ::GetTickCount64() + timeout;

        while (CQuit::run())
        {
            int bytes = send(m_ConnSocket, buf, len, 0);
            if (bytes != SOCKET_ERROR && bytes == len)
            {
                LPDATABOXT box = (LPDATABOXT)buf;
                MSG(0, _T("Sent %i bytes, key=%u\n"), bytes, htons(box->pkg.keyval));

                SentMoreData(bytes); // statistics..
                break; // success
            }

            if (::WSAGetLastError() != WSAEWOULDBLOCK)
            {
                len = 0;
                return DisplayError(_T("send() failed."));
            }

            if (tick < ::GetTickCount64())
                return ERROR_TIMEOUT;

            ::Sleep(CLIENT_WAIT);
        }

        return ERROR_SUCCESS;
    }

    int ReadData(char* buf, int& len, int timeout) throw()
    {
        UINT64 tick = ::GetTickCount64() + timeout;

        do 
        {
            int bytes = recv(m_ConnSocket, buf, len, 0);
            if (bytes != SOCKET_ERROR)
            {
                len = bytes;
                // We are not likely to see this with UDP, since there is no 'connection' established. 
                if (bytes == 0)
                {
                    MSG(0, _T("Server closed connection\n"));
                    return SOCKET_ERROR;
                }
                else
                {
                    while (DATABOX_SIZE_READ <= bytes && ((LPDATABOXT)buf)->pkg.anchor == ANCHOR)
                    {
                        DATABOXT& box = *(LPDATABOXT)buf;
                        MSG(0, _T("Received: key=%u, result=%u\n"), htons(box.pkg.keyval), htonl(box.pkg.result));

                        buf += bytes; bytes -= DATABOX_SIZE_READ;
                    }
                    if (bytes > 0)
                    {
                        CString str(buf, bytes);
                        MSG(0, _T("Received %i bytes from server: \"%s\"\n"), bytes, (LPCTSTR)str);
                    }
                    ReadMoreData(len); // statistics..
                }
                break; // success
            }

            if (::WSAGetLastError() != WSAEWOULDBLOCK)
            {
                len = 0;
                return DisplayError(_T("recv() failed."));
            }

            if (tick < ::GetTickCount64())
                return ERROR_TIMEOUT;

            ::Sleep(CLIENT_WAIT);

        } while (CQuit::run());

        return ERROR_SUCCESS;
    }

    bool GetConfigString(CString& buf, LPCTSTR path, LPCTSTR key, LPCTSTR dflt = 0, LPCTSTR app = _T("client")) throw()
    {
        DWORD cch, len = 16;

        do
        {
            len *= 2;
            cch = ::GetPrivateProfileString(app, key, dflt, buf.GetBuffer(len), len, path);
        } while (cch == (len - 2));

        if (cch > 0) { buf.ReleaseBuffer(); }

        return (cch > 0);
    }

    static BOOL WINAPI _CtrlHandler(DWORD /*type*/)
    {
        MSG(0, _T("\nStop signaled. Terminating...\n"));
        ERR(_T("User break.\n"));

        CQuit::set();

        return TRUE;
    }


public:
    bool m_bPause;      // "press any key" on exit..

    int m_clientid;
    int m_requests;
    int m_delaytik;     // delay between requests, ms (client)
    int m_worktime;     // total working time, s (client)

private:
    CSocketAsync m_ConnSocket;
    CSocketEvent m_ev;

    RandomGenerator m_rg;
};

CExeModule _Module;     // reserved name, DO NOT change


int _tmain(int argc, LPTSTR argv[])
{

    int nRet = 0;

    try
    {

        nRet = _Module.LoadPreferences(argc, argv);
        if (nRet == ERROR_SUCCESS)
        {
            CWsaInitialize _;

            do 
            {
                nRet = _Module.Connect();
                if (nRet == ERROR_SUCCESS)
                {
                    nRet = _Module.Run();
                }
                _Module.Disconnect();

            } while (nRet == ERROR_TIMEOUT);
        }
    }
    catch (int e)
    {
        nRet = CLog::Error(_T("Client failed."), e);
    }
    catch (std::exception& e)
    {
        ERR(_T("Exception caught: %s\n"), (LPCTSTR)CString(e.what()));
    }
    catch (...)
    {
        ERR(_T("Exception caught: unspecified.\n"));
    }

    if (_Module.m_bPause)
    {
        PAUSE(_T("\npress a key to exit...\n"));
    }

    return nRet;
}

