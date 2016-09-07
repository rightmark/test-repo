#pragma once


//////////////////////////////////////////////////////////////////////////
// Purpose:
// Create a client-server application for Windows.
//
// Algorithm:
// - Clients send to the server random integer numbers from 0 to 1023. 
// - The server receives and stores these numbers in a binary tree using them as unique keys. 
// - Upon a client's request, the average of squares of the numbers including the newly received number is calculated
//   and sent back to the client as a response.
// - A client receives this value and generates a new random number and then again sends it to the server and so on.
// - Every N seconds store all the items from the binary tree into a file in the binary format
//
// Tests:
// - Output all the processes into console and the application's log file.
// - Run about 20-30 clients and keep the program working about 10-20 minutes.
//



#ifdef _SERVER_BUILD_

// Singleton object

class CTask
{
    typedef ATL::CComAutoCriticalSection AutoCriticalSection;
    typedef ATL::CComCritSecLock<AutoCriticalSection> AutoLock;

    typedef std::map<SHORT, UINT> TStorage;

    static const size_t STORAGE_SIZE = (KEY_MAXIMUM + 1);

    static const UINT SAVER_WAIT = 100; // ms
    // save state interval in seconds
    static const UINT
        INTERVAL_MIN    = 5,
        INTERVAL_MAX    = 50,
        INTERVAL_INI    = 10;


public:
    CTask()
        : m_sqsum(0)
        , m_count(0)
        , m_bQuit(false)
        , m_interval(INTERVAL_INI)
        , m_bRequiresSave(false)
        , m_h(NULL)
    {}

    ~CTask() throw() { Terminate(); }
    //
    CTask(CTask const&) = delete;             // copy ctor
    CTask(CTask&&) = delete;                  // move ctor
    CTask& operator=(CTask const&) = delete;  // copy assign
    CTask& operator=(CTask &&) = delete;      // move assign

    bool LoadState(LPCTSTR fn) throw()
    {
        bool bRet = false;

        if (m_data.size() == 0)
        {
            m_file = fn;

            CAtlFile f;

            if (SUCCEEDED(f.Create(m_file, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING)))
            {
                UINT buf[STORAGE_SIZE] = {0};
                DWORD cch, len = sizeof(buf);

                if (SUCCEEDED(f.Read(buf, len, cch)) && cch == sizeof(buf))
                {
                    AutoLock _(m_cs);

                    m_sqsum = 0;
                    m_count = 0;

                    for (short i = 0; i < _countof(buf); ++i)
                    {
                        if (buf[i] > 0)
                        {
                            m_count += buf[i];
                            m_sqsum += (i * i) * buf[i];

                            m_data[i] = buf[i];
                        }
                    }
                    ATLTRACE(_T("CTask.LoadState(\"%s\")\n"), (LPCTSTR)m_file);
                }
            }

            m_h = (HANDLE)_beginthreadex(NULL, 0, _ThreadProc, this, 0, NULL);
            bRet = (NULL != m_h);
        }
        return bRet;
    }
    bool SaveState(void) throw()
    {
        bool bRet = false;

        if (m_data.size() > 0)
        {
            CAtlFile f;

            if (SUCCEEDED(f.Create(m_file, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS)))
            {
                Lock();

                auto temp = m_data;

                Unlock();

                UINT buf[STORAGE_SIZE] = {0};

                for (const auto& it : temp)
                {
                    buf[it.first] = it.second;
                }
                bRet = SUCCEEDED(f.Write(buf, sizeof(buf)));

                ATLTRACE(_T("CTask.SaveState(\"%s\")\n"), (LPCTSTR)m_file);
            }
        }
        return bRet;
    }

    UINT GetResult(SHORT key) throw()
    {
        UINT result = 0;

        AutoLock _(m_cs);

        ++m_data[key]; // ++(data[k])

        m_sqsum += (key * key);
        if (++m_count)
        {
            result = (UINT)(m_sqsum / m_count);

            m_bRequiresSave = true;
        }

        return result;
    }
    void SetInterval(UINT val) throw()
    {
        m_interval = min(max(val, INTERVAL_MIN), INTERVAL_MAX);
    }
    void Terminate(void) throw()
    {
        m_bQuit = true;

        if (NULL != m_h)
        {
            ::WaitForSingleObject(m_h, INFINITE); m_h = NULL;
        }
    }


protected:
    // helpers
    void Lock(void) throw() { m_cs.Lock(); }
    void Unlock(void) throw() { m_cs.Unlock(); }

private:
    UINT ThreadProc(void) throw()
    {
        ATLTRACE(_T("Task thread run (%p)\n"), this);

        static UINT64 lasttick = 0;

        while (!m_bQuit)
        {
            ::Sleep(SAVER_WAIT);

            UINT64 tick = ::GetTickCount64();
            if (tick > lasttick)
            {
                lasttick = tick + m_interval * 1000; // ms

                if (m_bRequiresSave)
                {
                    m_bRequiresSave = false;

                    if (!SaveState()) break;
                }
            }
        }
        if (m_bRequiresSave) { SaveState(); }

        ATLTRACE(_T("Task thread exited (%p)\n"), this);

        return ERROR_SUCCESS;
    }

    static UINT WINAPI _ThreadProc(void* pv)
    {
        CTask* p = static_cast<CTask*>(pv);
        return p->ThreadProc();
    }

protected:
    UINT64 m_sqsum;
    UINT32 m_count;

    TStorage m_data;
    CString m_file; // task state storage file name

private:
    bool volatile m_bQuit;
    bool volatile m_bRequiresSave;

    UINT volatile m_interval; // data saving interval in seconds

    HANDLE m_h; // thread handle

    AutoCriticalSection m_cs;
};



class CTaskProxy
{
public:
    CTaskProxy() {}
    ~CTaskProxy() throw() {}

    UINT GetResult(SHORT val) throw()
    {
        return Task.GetResult(val);
    }

    // properties
    __declspec(property(get=get_Task)) CTask& Task;
    CTask& get_Task(void) const { return CFactorySingleton<CTask>::Instance(); }

};

// server specific
static const int
    DATABOX_SIZE_READ = 4,
    DATABOX_SIZE_SEND = 8;

#endif // _SERVER_BUILD_

#ifdef _CLIENT_BUILD_

// client specific
static const int
    DATABOX_SIZE_READ = 8,
    DATABOX_SIZE_SEND = 4;

#endif // _CLIENT_BUILD_

// common
static const USHORT ANCHOR = 0xffff;

#include <pshpack1.h>
typedef union tag_buf {
    char buffer[8];     // raw data
    struct tag_pkg
    {
        UINT16 anchor;  // must be 0xffff
        UINT16 keyval;  // key value (0-1023)
        UINT32 result;
    } pkg;
} DATABOXT, *LPDATABOXT;
#include <poppack.h>

