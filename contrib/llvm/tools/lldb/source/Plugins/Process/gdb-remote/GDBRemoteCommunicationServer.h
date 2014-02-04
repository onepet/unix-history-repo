//===-- GDBRemoteCommunicationServer.h --------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_GDBRemoteCommunicationServer_h_
#define liblldb_GDBRemoteCommunicationServer_h_

// C Includes
// C++ Includes
#include <vector>
#include <set>
// Other libraries and framework includes
// Project includes
#include "lldb/Host/Mutex.h"
#include "lldb/Target/Process.h"
#include "GDBRemoteCommunication.h"

class ProcessGDBRemote;
class StringExtractorGDBRemote;

class GDBRemoteCommunicationServer : public GDBRemoteCommunication
{
public:
    typedef std::map<uint16_t, lldb::pid_t> PortMap;

    enum
    {
        eBroadcastBitRunPacketSent = kLoUserBroadcastBit
    };
    //------------------------------------------------------------------
    // Constructors and Destructors
    //------------------------------------------------------------------
    GDBRemoteCommunicationServer(bool is_platform);

    virtual
    ~GDBRemoteCommunicationServer();

    bool
    GetPacketAndSendResponse (uint32_t timeout_usec,
                              lldb_private::Error &error,
                              bool &interrupt, 
                              bool &quit);

    virtual bool
    GetThreadSuffixSupported ()
    {
        return true;
    }

    // After connecting, do a little handshake with the client to make sure
    // we are at least communicating
    bool
    HandshakeWithClient (lldb_private::Error *error_ptr);

    // Set both ports to zero to let the platform automatically bind to 
    // a port chosen by the OS.
    void
    SetPortMap (PortMap &&port_map)
    {
        m_port_map = port_map;
    }

    //----------------------------------------------------------------------
    // If we are using a port map where we can only use certain ports,
    // get the next available port.
    //
    // If we are using a port map and we are out of ports, return UINT16_MAX
    //
    // If we aren't using a port map, return 0 to indicate we should bind to
    // port 0 and then figure out which port we used.
    //----------------------------------------------------------------------
    uint16_t
    GetNextAvailablePort ()
    {
        if (m_port_map.empty())
            return 0; // Bind to port zero and get a port, we didn't have any limitations
        
        for (auto &pair : m_port_map)
        {
            if (pair.second == LLDB_INVALID_PROCESS_ID)
            {
                pair.second = ~(lldb::pid_t)LLDB_INVALID_PROCESS_ID;
                return pair.first;
            }
        }
        return UINT16_MAX;
    }

    bool
    AssociatePortWithProcess (uint16_t port, lldb::pid_t pid)
    {
        PortMap::iterator pos = m_port_map.find(port);
        if (pos != m_port_map.end())
        {
            pos->second = pid;
            return true;
        }
        return false;
    }

    bool
    FreePort (uint16_t port)
    {
        PortMap::iterator pos = m_port_map.find(port);
        if (pos != m_port_map.end())
        {
            pos->second = LLDB_INVALID_PROCESS_ID;
            return true;
        }
        return false;
    }

    bool
    FreePortForProcess (lldb::pid_t pid)
    {
        if (!m_port_map.empty())
        {
            for (auto &pair : m_port_map)
            {
                if (pair.second == pid)
                {
                    pair.second = LLDB_INVALID_PROCESS_ID;
                    return true;
                }
            }
        }
        return false;
    }

    void
    SetPortOffset (uint16_t port_offset)
    {
        m_port_offset = port_offset;
    }

protected:
    lldb::thread_t m_async_thread;
    lldb_private::ProcessLaunchInfo m_process_launch_info;
    lldb_private::Error m_process_launch_error;
    std::set<lldb::pid_t> m_spawned_pids;
    lldb_private::Mutex m_spawned_pids_mutex;
    lldb_private::ProcessInstanceInfoList m_proc_infos;
    uint32_t m_proc_infos_index;
    PortMap m_port_map;
    uint16_t m_port_offset;
    

    size_t
    SendUnimplementedResponse (const char *packet);

    size_t
    SendErrorResponse (uint8_t error);

    size_t
    SendOKResponse ();

    bool
    Handle_A (StringExtractorGDBRemote &packet);
    
    bool
    Handle_qLaunchSuccess (StringExtractorGDBRemote &packet);

    bool
    Handle_qHostInfo (StringExtractorGDBRemote &packet);
    
    bool
    Handle_qLaunchGDBServer (StringExtractorGDBRemote &packet);
    
    bool
    Handle_qKillSpawnedProcess (StringExtractorGDBRemote &packet);

    bool
    Handle_qPlatform_mkdir (StringExtractorGDBRemote &packet);
    
    bool
    Handle_qPlatform_chmod (StringExtractorGDBRemote &packet);
    
    bool
    Handle_qProcessInfoPID (StringExtractorGDBRemote &packet);
    
    bool
    Handle_qfProcessInfo (StringExtractorGDBRemote &packet);
    
    bool 
    Handle_qsProcessInfo (StringExtractorGDBRemote &packet);

    bool
    Handle_qC (StringExtractorGDBRemote &packet);

    bool 
    Handle_qUserName (StringExtractorGDBRemote &packet);

    bool 
    Handle_qGroupName (StringExtractorGDBRemote &packet);

    bool
    Handle_qSpeedTest (StringExtractorGDBRemote &packet);

    bool
    Handle_QEnvironment  (StringExtractorGDBRemote &packet);
    
    bool
    Handle_QLaunchArch (StringExtractorGDBRemote &packet);
    
    bool
    Handle_QSetDisableASLR (StringExtractorGDBRemote &packet);

    bool
    Handle_QSetWorkingDir (StringExtractorGDBRemote &packet);
    
    bool
    Handle_qGetWorkingDir (StringExtractorGDBRemote &packet);

    bool
    Handle_QStartNoAckMode (StringExtractorGDBRemote &packet);

    bool
    Handle_QSetSTDIN (StringExtractorGDBRemote &packet);

    bool
    Handle_QSetSTDOUT (StringExtractorGDBRemote &packet);

    bool
    Handle_QSetSTDERR (StringExtractorGDBRemote &packet);
    
    bool
    Handle_vFile_Open (StringExtractorGDBRemote &packet);

    bool
    Handle_vFile_Close (StringExtractorGDBRemote &packet);

    bool
    Handle_vFile_pRead (StringExtractorGDBRemote &packet);

    bool
    Handle_vFile_pWrite (StringExtractorGDBRemote &packet);

    bool
    Handle_vFile_Size (StringExtractorGDBRemote &packet);
    
    bool
    Handle_vFile_Mode (StringExtractorGDBRemote &packet);

    bool
    Handle_vFile_Exists (StringExtractorGDBRemote &packet);
    
    bool
    Handle_vFile_symlink (StringExtractorGDBRemote &packet);
    
    bool
    Handle_vFile_unlink (StringExtractorGDBRemote &packet);

    bool
    Handle_vFile_Stat (StringExtractorGDBRemote &packet);
    
    bool
    Handle_vFile_MD5 (StringExtractorGDBRemote &packet);
    
    bool
    Handle_qPlatform_shell (StringExtractorGDBRemote &packet);

private:
    bool
    DebugserverProcessReaped (lldb::pid_t pid);
    
    static bool
    ReapDebugserverProcess (void *callback_baton,
                            lldb::pid_t pid,
                            bool exited,
                            int signal,
                            int status);

    //------------------------------------------------------------------
    // For GDBRemoteCommunicationServer only
    //------------------------------------------------------------------
    DISALLOW_COPY_AND_ASSIGN (GDBRemoteCommunicationServer);
};

#endif  // liblldb_GDBRemoteCommunicationServer_h_
