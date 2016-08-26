///////////////////////////////////////////////////////////////////////////////
//  WSA-related wrapper classes.
//
#ifndef __WSAHELP_H__
#define __WSAHELP_H__

#pragma once


#ifndef _WINSOCK2API_
    #error 'ws2tcpip.h must be included first
#endif

#ifndef _EXCEPTION_
    #error '<exception> must be included first
#endif


namespace WSA
{

class CWsaInitialize
{
public:
    CWsaInitialize()
    {
        int err = Start(NULL);
        if (err != ERROR_SUCCESS)
            throw err;
    }
    CWsaInitialize(LPWSADATA pData)
    {
        int err = Start(pData);
        if (err != ERROR_SUCCESS)
            throw err;
    }
    ~CWsaInitialize() throw() { Clean(); }

    int Start(LPWSADATA pData)
    {
        if (pData == NULL) { pData = &m_data; }

        // ask for Winsock version 2.2.
        if (::WSAStartup(WINSOCK_VERSION, pData) != ERROR_SUCCESS)
            return ::WSAGetLastError();

        if (pData->wVersion != WINSOCK_VERSION)
        {
            Clean(WSAVERNOTSUPPORTED);
            throw(std::exception("Winsock version requested is not supported."));
        }
        return ERROR_SUCCESS;
    }
    int Clean(int err = ERROR_SUCCESS) throw()
    {
        ::WSACleanup();
        ::WSASetLastError(err);
        return err;
    }

public:
    WSADATA m_data;
};


class CSocketAsync
{
public:
    CSocketAsync() throw() : m_s(INVALID_SOCKET) {}
    CSocketAsync(CSocketAsync& s) throw() : m_s(INVALID_SOCKET) // copy-ctor
    {
        Attach(s.Detach());
    }
    explicit CSocketAsync(SOCKET s) throw() : m_s(s) {}

    ~CSocketAsync() throw() { Close(); }

    CSocketAsync& operator=(CSocketAsync& s) throw()
    {
        if (this != &s)
        {
            if (m_s != INVALID_SOCKET) { Close(); }
            Attach(s.Detach());
        }
        return  *this;
    }
    operator SOCKET() const throw() { return m_s; }

    void Attach(SOCKET s) throw()
    {
        ATLASSUME(m_s == INVALID_SOCKET);
        m_s = s; // take ownership

        m_addrlen = sizeof(m_addr);
        getpeername(m_s, (LPSOCKADDR)&m_addr, &m_addrlen);
    }
    SOCKET Detach(void) throw()
    {
        SOCKET s = m_s; // release ownership
        m_s = INVALID_SOCKET;

        return  s;
    }

    void Close(void) throw()
    {
        if (m_s != INVALID_SOCKET)
        {
            closesocket(m_s);
            m_s = INVALID_SOCKET;
        }
    }

    bool Create(PADDRINFOT pai) throw()
    {
        ATLASSUME(m_s == INVALID_SOCKET);
        m_s = socket(pai->ai_family, pai->ai_socktype, pai->ai_protocol);
        return  (m_s != INVALID_SOCKET);
    }
    bool Bind(PADDRINFOT pai) throw()
    {
        return (bind(m_s, pai->ai_addr, (int)pai->ai_addrlen) != SOCKET_ERROR);
    }
    bool IOCtl(DWORD code, PBOOL bOpt) throw() // get the ioctl
    {
        DWORD bytes = 0;
        return (WSAIoctl(m_s, code, NULL, 0, bOpt, sizeof(BOOL), &bytes, NULL, NULL) != SOCKET_ERROR);
    }
    bool IOCtl(DWORD code, BOOL bOpt) throw() // set the ioctl
    {
        DWORD bytes = 0;
        return (WSAIoctl(m_s, code, &bOpt, sizeof(bOpt), NULL, 0, &bytes, NULL, NULL) != SOCKET_ERROR);
    }
    bool Option(int opt, BOOL b) throw()
    {
        return (setsockopt(m_s, SOL_SOCKET, opt, (const char*)&b, sizeof(BOOL)) != SOCKET_ERROR);
    }
    bool Listen(int backlog = SOMAXCONN) throw()
    {
        return (listen(m_s, backlog) != SOCKET_ERROR);
    }
    bool Accept(LPSOCKADDR_STORAGE addr, LPINT addrlen, SOCKET& s) throw()
    {
        return ((s = accept(m_s, (LPSOCKADDR)addr, addrlen)) != SOCKET_ERROR);
    }
    long NameInfo(LPSOCKADDR *addr, LPTSTR hostname, DWORD size) throw()
    {
        *addr = (LPSOCKADDR)&m_addr;
        return ::GetNameInfo((LPSOCKADDR)&m_addr, m_addrlen, hostname, size, NULL, 0, NI_NUMERICHOST);
    }

    bool SelectEvents(WSAEVENT ev, LONG events) throw()
    {
        return (::WSAEventSelect(m_s, ev, events) != SOCKET_ERROR);
    }
    bool EnumEvents(WSAEVENT ev, LPWSANETWORKEVENTS ne) throw()
    {
        return (::WSAEnumNetworkEvents(m_s, ev, ne) != SOCKET_ERROR);
    }
    bool Shutdown(int how) throw()
    {
        return (shutdown(m_s, how) != SOCKET_ERROR);
    }

    // @TODO: km 20160724 - consider to add RecvFrom(), SendTo(), ..

public:
    // @TODO: km 20160825 - TBD..
/*
    CHeapPtr<char> m_data;  // pending data buffer
    int m_datasize;         // pending data size
*/

private:
    SOCKET m_s;

    SOCKADDR_STORAGE m_addr;
    int m_addrlen;
};


// ok, the standard ATL::CEvent class could be used surely.
// this custom wrapper is used instead to avoid one more dependency on ATL and make code more WSA-specific.
//
class CSocketEvent
{
public:
    CSocketEvent() throw() : m_h(NULL) {}
    CSocketEvent(CSocketEvent& h) throw() : m_h(NULL) // copy-ctor
    {
        Attach(h.Detach());
    }
    explicit CSocketEvent(WSAEVENT h) throw() : m_h(h) {}

    ~CSocketEvent() throw() { Close(); } // @WARNING: non-virtual dtor !!

    CSocketEvent& operator=(CSocketEvent& h) throw()
    {
        if (this != &h)
        {
            if (m_h != NULL)
            {
                Close();
            }
            Attach(h.Detach());
        }
        return  *this;
    }
    operator WSAEVENT() const throw() { return m_h; }

    void Attach(WSAEVENT h) throw()
    {
        ATLASSUME(m_h == NULL);
        m_h = h; // take ownership
    }
    WSAEVENT Detach(void) throw()
    {
        HANDLE h = m_h; // release ownership
        m_h = NULL;

        return  h;
    }

    void Close(void) throw()
    {
        if (m_h != NULL)
        {
            ::WSACloseEvent(m_h);
            m_h = NULL;
        }
    }

    bool Create(void) throw()
    {
        ATLASSUME(m_h == NULL);
        m_h = ::WSACreateEvent();
        return  (m_h != WSA_INVALID_EVENT);
    }
    // set the event to the non-signaled state
    bool Reset(void) throw()
    {
        ATLASSUME(m_h != NULL);
        return  !!(::WSAResetEvent(m_h));
    }
    // set the event to the signaled state
    bool Set(void) throw()
    {
        ATLASSUME(m_h != NULL);
        return  !!(::WSASetEvent(m_h));
    }

private:
    WSAEVENT m_h;
};


class CAddrInfo
{
    CAddrInfo() throw() {}
public:
    CAddrInfo(LPCTSTR host, LPCTSTR port, int socktype, int family = AF_UNSPEC, int flags = 0) : m_ai(NULL)
    {
        Create(host, port, socktype, family, flags);
    }
    CAddrInfo(LPCTSTR port, int socktype, int family = AF_UNSPEC) : m_ai(NULL)
    {
        Create(NULL, port, socktype, family, AI_PASSIVE);
    }
   
    ~CAddrInfo() throw() { Free(); }

    operator PADDRINFOT() const throw() { return m_ai; }

    void Create(LPCTSTR host, LPCTSTR port, int socktype, int family, int flags)
    {
        ADDRINFOT hints = {0};

        hints.ai_socktype = socktype;
        hints.ai_family   = family;
        hints.ai_flags    = flags;

        int err = NO_ERROR;
        // Resolve the server address and port
        if ((err = ::GetAddrInfo(host, port, &hints, &m_ai)) != 0)
        {
            throw err;
        }
    }
    void Free(void) throw()
    {
        if (m_ai)
        {
            ::FreeAddrInfo(m_ai);
            m_ai = NULL;
        }
    }

private:
    PADDRINFOT m_ai;
};


} // WSA

#pragma comment(lib, "Ws2_32.lib")

#endif // __WSAHELP_H__

