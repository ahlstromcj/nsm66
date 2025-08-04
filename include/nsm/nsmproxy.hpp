#if ! defined NSM66_NSM_NSMPROXY_HPP
#define NSM66_NSM_NSMPROXY_HPP

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
 * \file          nsmproxy.hpp
 *
 *    This module refactors the nsmproxy class to replace C code with
 *    C++ code.
 *
 * \library       nsm-proxy66 application
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-03-06
 * \updates       2025-03-20
 * \version       $Revision$
 * \license       GNU GPL v3 or above
 *
 *   To do.
 */

#include <string>                       /* std::string class                */
#include <lo/lo.h>                      /* lo_server, lo_address            */

#include "c_macros.h"
#include "cpp_types.hpp"                /* lib66::tokenization alias        */

#if ! defined ENV_NSM_CLIENT_ID
#define ENV_NSM_CLIENT_ID       "NSM_CLIENT_ID"
#define ENV_NSM_SESSION_NSM     "NSM_SESSION_NAME"
#define ENV_NSM_CONFIG_FILE     "NSM_CONFIG_FILE"
#define ENV_NSM_URL             "NSM_URL"
#endif

#define NSM_CONFIG_FILE_NAME    "nsm-proxy.config"

namespace nsm
{

class nsmproxy
{

private:

    lo_server m_lo_server;
    lo_address m_nsm_address;
//  lo_address m_gui_address;
    std::string m_label;
    std::string m_executable;
    std::string m_arguments;
    std::string m_config_file;
    std::string m_client_error;
    int m_save_signal;
    int m_stop_signal;
    int m_pid;
    std::string m_nsm_client_id;
    std::string m_nsm_display_name;

public:

    nsmproxy ();
    nsmproxy
    (
        const std::string & client_id,
        const std::string & display_name
    );
    ~nsmproxy () = default;

    void handle_client_death (int status);
    bool start ();
    bool start
    (
        const std::string & executable,
        const std::string & arguments,
        const std::string & config_file
    );
    void kill ();

    int stop_signal ()
    {
        return m_stop_signal;
    }

    void save_signal (int s)
    {
        m_save_signal = s;
    }

    void stop_signal (int s)
    {
        m_stop_signal = s;
    }

    void label (const std::string & s);
    void save ();
    bool dump (const std::string & path);
    bool restore (const std::string & path);
    void update (lo_address to);

private:

    /*
     * Similar to the send() functions in the endpoint class.
     */

    int send
    (
        lo_address to,
        const std::string & oscpath,
        int signalvalue
    );
    int send
    (
        lo_address to,
        const std::string & oscpath,
        const std::string & stringvalue
    );

};          // class nsmproxy

}           // namespace nsm

#endif      // define NSM66_NSM_NSMPROXY_HPP

/*
 * nsmproxy.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

