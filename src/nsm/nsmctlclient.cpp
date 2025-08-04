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
 * \file          nsmctlclient.cpp
 *
 *    This module refactors the nsmd application to replace C code with
 *    C++ code.
 *
 * \library       nsmctl application
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-03-21
 * \updates       2025-03-29
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 * Tags implemented herein:
 *
 *      -   guisave
 *      -   guishow
 *      -   guihide
 *      -   guiremove
 *      -   guiresume
 *      -   guistop
 *
 *      See nsmcontroller for server-side tags.
 */

#include <cerrno>                       /* #include <errno.h>               */
#include <getopt.h>
#include <time.h>
#include <unistd.h>

#include "nsm/nsmctlclient.hpp"
#include "util/msgfunctions.hpp"        /* cfg66 util::xxxxx_message()....  */
#include "util/strfunctions.hpp"        /* cfg66 util::string_asprintf()... */

namespace nsm
{

/*--------------------------------------------------------------------------
 * daemon class
 *--------------------------------------------------------------------------*/

daemon::daemon () :
    m_url       (),
    m_addr      (nullptr),
    m_is_child  (false)
{
    // no code
}

daemon::daemon
(
    const std::string & url,
    lo_address addr,
    bool ischild
) :
    m_url       (url),
    m_addr      (addr),
    m_is_child  (ischild)
{
    // no code
}

/*--------------------------------------------------------------------------
 * nsmctlclient class
 *--------------------------------------------------------------------------*/

nsmctlclient::nsmctlclient
(
    osc::endpoint * oscserver,
    daemon_list & daemonlist,
    const std::string & client_id,
    const std::string & client_label,
    const std::string & client_name
) :
    m_osc_server    (oscserver),
    m_daemon_list   (daemonlist),
    m_client_id     (client_id),
    m_client_label  (client_label),
    m_client_name   (client_name),
    m_progress      (0.0),
    m_dirty         (false),
    m_visible       (false)
{
    stopped(false);
}

void
nsmctlclient::stopped (bool b)
{
    if (b)
    {
        /*
         *  _remove_button->show(); _restart_button->show();
         *  _kill_button->hide(); _gui->deactivate(); _dirty->deactivate();
         */
    }
    else
    {
        /*
         *  _gui->activate(); _dirty->activate(); _kill_button->show();
         *  _restart_button->hide(); _remove_button->hide();
         */
    }
}

void
nsmctlclient::pending_command (const std::string & command)
{
    stopped(false);
    if (command == "ready")
    {
        // _progress->value( 0.0f );
    }
    else if (command == "quit" || command == "kill" || command == "error")
    {
        // Set a border color to indicate warning
    }
    else if (command == "stopped")
    {
        stopped(true);
    }
}

/**
 *  Add the ability to send to a specific client ID or name in
 *  nsmcontroller.
 *
 *  The code here is provides the same functionality as in nsm-legacy-gui.
 */

bool
nsmctlclient::send_client_message (osc::tag o)
{
    std::string msg;
    std::string pattern;
    bool result = tag_lookup(o, msg, pattern);
    if (result)
    {
        result = false;
        if (o == osc::tag::guidirty || o == osc::tag::guisave)
        {
            result = tag_lookup(osc::tag::guisave, msg, pattern);
            if (result)
            {
                util::info_message("Sending save");
                for (const auto & d : m_daemon_list)
                    m_osc_server->send(d.addr(), msg, m_client_id);
            }
        }
        else if (o == osc::tag::guishow)
        {
            result = true;
            util::info_message("Sending show GUIs");
            for (const auto & d : m_daemon_list)
                    m_osc_server->send(d.addr(), msg, m_client_id);
        }
        else if (o == osc::tag::guihide)
        {
            result = true;
            util::info_message("Sending hide GUIs");
            for (const auto & d : m_daemon_list)
                m_osc_server->send(d.addr(), msg, m_client_id);
        }
        else if (o == osc::tag::guiremove)
        {
            result = true;
            util::info_message("Sending remove");
            for (const auto & d : m_daemon_list)
                m_osc_server->send(d.addr(), msg, m_client_id);
        }
        else if (o == osc::tag::guiresume)
        {
            result = true;
            util::info_message("Sending resume");
            for (const auto & d : m_daemon_list)
                m_osc_server->send(d.addr(), msg, m_client_id);
        }
        else if (o == osc::tag::guistop)
        {
            result = true;
            util::info_message("Sending stop");
            for (const auto & d : m_daemon_list)
                m_osc_server->send(d.addr(), msg, m_client_id);
        }
    }
    return result;
}

std::string
nsmctlclient::info (const std::string & tag) const
{
    std::string label = client_label().empty() ? "---" : client_label() ;
    std::string tag2 = tag.empty() ? "Client" : tag ;
    std::string result = util::string_asprintf
    (
        "%s ID: %s; Name %s; Label %s",
        V(tag2), V(client_id()), V(client_name()), V(label)
    );
    return result;
}

}           // namespace nsm

/*
 * nsmctlclient.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


