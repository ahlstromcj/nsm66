#if ! defined NSM_NSMCONTROLLER_HPP
#define NSM_NSMCONTROLLER_HPP

/*
 * Copyright (C) 2008-2020 Jonathan Moore Liles (as "Non-Session-Manager")
 * Copyright (C) 2020- Nils Hilbricht
 *
 * This file is part of New-Session-Manager
 *
 * New-Session-Manager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * New-Session-Manager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with New-Session-Manager. If not, see <https://www.gnu.org/licenses/>
 */

/**
 * \file          nsmcontroller.hpp
 *
 *    This module refactors the nsmd application to replace C code with
 *    C++ code.
 *
 * \library       nsmctl application
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-03-21
 * \updates       2025-04-16
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   To do.
 */

#include <map>                          /* std::map<>                       */
#include <memory>                       /* std::unique_ptr<>                */

#include "nsm/nsmctlclient.hpp"         /* nsm::nsmctlclient & nsm::daemon  */
#include "osc/messages.hpp"             /* osc::tag                         */

namespace osc
{
    class endpoint;
}

namespace nsm
{

class nsmcontroller
{

public:

    /*
     *  The key is the client ID, which is the random tag used by nsmd,
     *  of the form "nXYZT". Note that the nsmctlclient holds information
     *  about its ID, label, and name.
     *
     * TODO: use unique_ptr
     */

    using clientmap = std::map<std::string, nsmctlclient *>;

private:

    std::unique_ptr<osc::endpoint> m_osc_server;
    daemon_list & m_daemon_list;
    lib66::tokenization m_session_list;
    clientmap m_clients_pack;           /* name reminiscent of the FLTK GUI */
    time_t m_last_ping_response;
    time_t m_ping_timeout;
    int m_ping_count;
    std::string m_app_name;
    std::string m_exe_name;
    std::string m_capabilities;
    std::string m_api_version;
    std::string m_session_name;

public:

    nsmcontroller () = delete;
    nsmcontroller
    (
        daemon_list &,
        const std::string & appname      = "",
        const std::string & exename      = "",
        const std::string & capabilities = "",
        const std::string & apiversion   = "",
        int pingct                       = 4
    );
    nsmcontroller (const nsmcontroller &) = delete;
    nsmcontroller (nsmcontroller &&) = delete;
    nsmcontroller & operator = (const nsmcontroller &) = delete;
    nsmcontroller & operator = (nsmcontroller &&) = delete;
    ~nsmcontroller () = default;

    static void send_server_message (void * v, osc::tag msg);
    bool send_server_message
    (
        osc::tag msg,
        const std::string & subjectname = ""
    );
    bool send_client_message
    (
        osc::tag msg,
        const std::string & clientname
    );

    nsmctlclient * client_by_id (const std::string & id) const;
    nsmctlclient * client_by_name (const std::string & name) const;
    void log_status (const std::string & s, bool iserror = false);

    std::string url () const
    {
        return not_nullptr(m_osc_server) ?
            m_osc_server->url() : std::string("") ;
    }

    const std::string & session_name () const
    {
        return m_session_name;
    }

    void session_name (const std::string & name)
    {
        m_session_name = name;
    }

    void client_stopped (const std::string & client_id);
    void client_quit (const std::string & client_id);
    bool client_new
    (
        const std::string & client_id,
        const std::string & client_name
    );
    void client_pending_command
    (
        nsmctlclient * c,
        const std::string & command
    );

    void add_session_to_list (const std::string & name)
    {
        if (! name.empty())
            m_session_list.push_back(name);
    }

    std::string get_session_list () const;
    void osc_wait (int timeout);
    bool osc_active () const;
    bool deactivate ();
    bool ping ();
    bool init_osc (const std::string & portname = "");
    void announce ();
    void quit ();

private:

    void announce (const std::string & nsmurl, bool legacy = true);
    bool child_check () const;
    void add_method
    (
        osc::tag t,
        osc::method_handler f,
        const std::string & argument_description
    );

private:

    static int osc_broadcast_handler
    (
        const char * path, const char * types, lo_arg **,
        int argc, lo_message msg, void * user_data
    );
    static int osc_handler
    (
        const char * path, const char * types, lo_arg **argv,
        int argc, lo_message msg, void * user_data
    );
};

}           // namespace nsm

#endif      // NSM_NSMCONTROLLER_HPP

/*
 * nsmcontroller.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
