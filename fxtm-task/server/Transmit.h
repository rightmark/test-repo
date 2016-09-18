
#pragma once

// default response
#define HELLO_STR   "Hello. Server version 1.0"


template<class T, class Log = CLog>
class ATL_NO_VTABLE CTransmitBase : public CStatManager<STAT_INTERVAL>
{
public:
    int ReadData(CSocketAsync& s) throw()
    {
        T* pT = static_cast<T*>(this);
        return pT->ReadData(s);
    }
    int SendData(CSocketAsync& s) throw()
    {
        T* pT = static_cast<T*>(this);
        return pT->SendData(s);
    }

    int DisplayError(LPCTSTR msg, int err = ::WSAGetLastError()) throw()
    {
        return Log::Error(msg, err);
    }

protected:
    // helpers
    void ReadDataStat(int cb) throw()
    {
        ReadMoreData(cb);
    }

    void SentDataStat(int cb) throw()
    {
        SentMoreData(cb);
    }

    int Hello(CSocketAsync& s) throw()
    {
        char buf[] = HELLO_STR;
        int len = (int)strlen(buf) + 1;

        int bytes = send(s, buf, len, 0);
        if (bytes == SOCKET_ERROR)
        {
            DisplayError(_T("send() failed."));
        }
        else
        {
            MSG(1, _T("hello message sent\n"));
        }
        return bytes;
    }

private:
};


//////////////////////////////////////////////////////////////////////////
// Simple implementation echoing the same data back to client.
//

class CTransmitTcpEcho
    : public CTransmitBase<CTransmitTcpEcho>
    , public CTaskProxy
{
public:
    int ReadData(CSocketAsync& s) throw()
    {
        unique_ptr<char[]> sp(new char[BUFFER_SIZE]);
        char* buf = sp.get();

        int bytes = recv(s, buf, BUFFER_SIZE, 0);
        if (bytes == SOCKET_ERROR)
        {
            if (::WSAGetLastError() != WSAEWOULDBLOCK)
            {
                DisplayError(_T("recv() failed."));
                return SOCKET_ERROR;
            }
            return 0; // next time
        }
        else if (bytes == 0)
        {
            MSG(0, _T("Client closed connection\n"));
            return SOCKET_ERROR;
        }

        ReadDataStat(bytes);

        TCHAR hostname[MAX_ADDRSTRLEN];
        LPSOCKADDR from = NULL;

        int err = s.NameInfo(&from, hostname, _countof(hostname));
        if (err != NO_ERROR)
        {
            DisplayError(_T("CSocketAsync.NameInfo() failed."), err);
            _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
        }

        CString str(buf, bytes);
        MSG(1, _T("recv bytes=%i, port=%u: %s, from %s\n"), bytes, ntohs(SS_PORT(from)), (LPCTSTR)str, hostname);

        int cbSent = send(s, buf, bytes, 0);
        if (cbSent == SOCKET_ERROR)
        {
            if (::WSAGetLastError() != WSAEWOULDBLOCK)
            {
                DisplayError(_T("send() failed."));
                return SOCKET_ERROR;
            }
            else
            {
                m_data.push_back(std::move(sp));
                m_datasize.push_back(bytes);

                return 0; // next time
            }
        }
        else
        {
            SentDataStat(bytes);
        }

        return bytes;
    }
    int SendData(CSocketAsync& s) throw()
    {
        if (m_data.empty()) { return Hello(s); }

        while (!m_data.empty())
        {
            char* buf = m_data.front().get();

            int len = m_datasize.front();

            int bytes = send(s, buf, len, 0);
            if (bytes == SOCKET_ERROR)
            {
                if (::WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    DisplayError(_T("send() failed."));

                    m_data.pop_front(); // remove on socket error
                    m_datasize.pop_front();

                    return SOCKET_ERROR;
                }
                break; // next time on WSAEWOULDBLOCK
            }
            else
            {
                SentDataStat(bytes);

                m_data.pop_front(); // remove from list on success
                m_datasize.pop_front();
            }
        }

        return 0;
    }

private:
    std::list<unique_ptr<char[]>> m_data; // pending data
    std::list<int> m_datasize; // pending data size
};

class CTransmitUdpEcho
    : public CTransmitBase<CTransmitUdpEcho>
    , public CTaskProxy
{
public:
    int ReadData(CSocketAsync& s) throw()
    {
        char buf[BUFFER_SIZE] = {0};
        TCHAR hostname[MAX_ADDRSTRLEN];

        SOCKADDR_STORAGE from = {0};
        int fromlen = sizeof(from);

        // Since UDP maintains message boundaries, the amount of data we get from a recvfrom()
        // should match exactly the amount of data the client sent in the corresponding sendto().
        //
        int bytes = recvfrom(s, buf, sizeof(buf), 0, (LPSOCKADDR)&from, &fromlen);
        if (bytes == SOCKET_ERROR)
        {
            DisplayError(_T("recvfrom() failed."));
            return bytes;
        }
        if (bytes == 0)
        {
            // Actually, this should never happen on an unconnected socket..
            MSG(1, _T("recvfrom() returned zero, aborting\n"));
            return bytes;
        }

        ReadDataStat(bytes); // statistics..

        int err = ::GetNameInfo((LPSOCKADDR)&from, fromlen, hostname, sizeof(hostname), NULL, 0, NI_NUMERICHOST);
        if (err != NO_ERROR)
        {
            DisplayError(_T("GetNameInfo() failed."), err);
            _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
        }

        CString str(buf, bytes);
        MSG(1, _T("recv bytes=%i, port=%u: %s, from %s\n"), bytes, ntohs(SS_PORT(&from)), (LPCTSTR)str, hostname);

        bytes = sendto(s, buf, bytes, 0, (LPSOCKADDR)&from, fromlen);
        if (bytes == SOCKET_ERROR)
        {
            DisplayError(_T("sendto() failed."));
        }
        else
        {
            SentDataStat(bytes);
        }

        return bytes;
    }
    int SendData(CSocketAsync& /*s*/) throw()
    {
        return 0;
    }
};



//////////////////////////////////////////////////////////////////////////
//
//

class CTransmitTcpBuffer
    : public CTransmitBase<CTransmitTcpBuffer>
    , public CTaskProxy
{
public:
    int ReadData(CSocketAsync& s) throw()
    {
        DATABOXT box = {0};

        int bytes = recv(s, box.buffer, DATABOX_SIZE_READ, 0);
        if (bytes == SOCKET_ERROR)
        {
            if (::WSAGetLastError() != WSAEWOULDBLOCK)
            {
                DisplayError(_T("recv() failed."));
                return SOCKET_ERROR;
            }
            return 0; // next time
        }
        else if (bytes == 0)
        {
            MSG(0, _T("Client closed connection\n"));
            return SOCKET_ERROR;
        }

        ReadDataStat(bytes);

        TCHAR hostname[MAX_ADDRSTRLEN];
        LPSOCKADDR from = NULL;

        int err = s.NameInfo(&from, hostname, _countof(hostname));
        if (err != NO_ERROR)
        {
            DisplayError(_T("CSocketAsync.NameInfo() failed."), err);
            _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
        }

        if (DATABOX_SIZE_READ == bytes && box.pkg.anchor == ANCHOR)
        {
            ULONG result = 0;
            USHORT key = htons(box.pkg.keyval);

            if (key <= KEY_MAXIMUM)
            {
                MSG(1, _T("recv bytes=%i, key=%u, port=%u, from %s\n"), bytes, key, ntohs(SS_PORT(from)), hostname);

                result = GetResult(key);
            }
            box.pkg.result = ntohl(result);

            bytes = send(s, box.buffer, DATABOX_SIZE_SEND, 0);
            if (bytes == SOCKET_ERROR)
            {
                if (::WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    DisplayError(_T("send() failed."));
                    return SOCKET_ERROR;
                }
                else
                {
                    m_data.push_back(box); // send later on FD_WRITE
                    return 0; // next time
                }
            }
            else
            {
                SentDataStat(bytes);
            }
        }

        return bytes;
    }

    int SendData(CSocketAsync& s) throw()
    {
//         if (m_data.empty()) { return Hello(s); }

        while (!m_data.empty())
        {
            DATABOXT& box = m_data.front();

            int bytes = send(s, box.buffer, DATABOX_SIZE_SEND, 0);
            if (bytes == SOCKET_ERROR)
            {
                if (::WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    m_data.pop_front(); // remove on socket error

                    DisplayError(_T("send() failed."));
                    return SOCKET_ERROR;
                }
                break; // next time on WSAEWOULDBLOCK
            }
            else
            {
                SentDataStat(bytes);

                m_data.pop_front(); // remove from list on success
            }
        }

        return 0;
    }

private:
    std::list<DATABOXT> m_data; // pending data
};

class CTransmitUdpBuffer
    : public CTransmitBase<CTransmitUdpBuffer>
    , public CTaskProxy
{
public:
    int ReadData(CSocketAsync& s) throw()
    {
        DATABOXT box = {0};

        TCHAR hostname[MAX_ADDRSTRLEN];
        SOCKADDR_STORAGE from = {0};
        int fromlen = sizeof(from);

        // Since UDP maintains message boundaries, the amount of data we get from a recvfrom()
        // should match exactly the amount of data the client sent in the corresponding sendto().
        //
        int bytes = recvfrom(s, box.buffer, DATABOX_SIZE_READ, 0, (LPSOCKADDR)&from, &fromlen);
        if (bytes == SOCKET_ERROR)
        {
            if (::WSAGetLastError() != WSAEWOULDBLOCK)
            {
                DisplayError(_T("recvfrom() failed."));
                return SOCKET_ERROR;
            }
            return 0; // next time
        }

        if (bytes == 0)
        {
            // Actually, this should never happen on an unconnected socket..
            MSG(1, _T("recvfrom() returned zero, aborting\n"));
            return bytes;
        }

        ReadDataStat(bytes);

        int err = ::GetNameInfo((LPSOCKADDR)&from, fromlen, hostname, _countof(hostname), NULL, 0, NI_NUMERICHOST);
        if (err != NO_ERROR)
        {
            DisplayError(_T("GetNameInfo() failed."), err);
            _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
        }

        if (DATABOX_SIZE_READ == bytes && box.pkg.anchor == ANCHOR)
        {
            ULONG result = 0;
            USHORT key = htons(box.pkg.keyval);

            if (key <= KEY_MAXIMUM)
            {
                MSG(1, _T("recv bytes=%i, key=%u, port=%u, from %s\n"), bytes, key, ntohs(SS_PORT(&from)), hostname);

                result = GetResult(key);
            }

            box.pkg.result = ntohl(result);

            bytes = sendto(s, box.buffer, DATABOX_SIZE_SEND, 0, (LPSOCKADDR)&from, fromlen);
            if (bytes == SOCKET_ERROR)
            {
                if (::WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    DisplayError(_T("sendto() failed."));
                    return SOCKET_ERROR;
                }
                else
                {
                    //m_data.push_back(box); // send later on FD_WRITE
                    return 0; // next time
                }
            }
            else
            {
                SentDataStat(bytes);
            }
        }

        return bytes;
    }
    int SendData(CSocketAsync& /*s*/) throw()
    {
        // @TODO: km 20160810 - write code..
        return 0;
    }
};



typedef CTransmitTcpBuffer CTransmitTcp;
typedef CTransmitUdpBuffer CTransmitUdp;
