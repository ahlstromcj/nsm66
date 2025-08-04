#if ! defined NSM_NSMCTLCLIENT_HPP
#define NSM_NSMCTLCLIENT_HPP

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
 * \file          nsmctlclient.hpp
 *
 *    This module refactors the nsmd application to replace C code with
 *    C++ code.
 *
 * \library       nsmctl application
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-03-21
 * \updates       2025-04-04
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   To do.
 */

#include <lo/lo.h>                      /* lo_address                       */
#include <vector>                       /* std::vector<> container          */

#include "osc/endpoint.hpp"

namespace nsm
{

/*--------------------------------------------------------------------------
 * daemon class
 *--------------------------------------------------------------------------*/

class daemon
{

private:

    std::string m_url;
    lo_address m_addr;
    bool m_is_child;

public:

    daemon ();
    daemon
    (
        const std::string & url,
        lo_address addr,
        bool ischild = false
    );

    const std::string & url () const
    {
        return m_url;
    }

    void url (const std::string & u)
    {
        m_url = u;
    }

    lo_address addr () const
    {
        return m_addr;
    }

    lo_address addr ()
    {
        return m_addr;
    }

    void addr (lo_address a)
    {
        m_addr = a;
    }

    bool is_child () const
    {
        return m_is_child;
    }

    void is_child (bool c)
    {
        m_is_child = c;
    }

};

/**
 *  Supports a list of nsm::daemon classes. This is plain data, so
 *  we don't need pointers; we can store copies.
 */

using daemon_list = std::vector<daemon>;    /* list of connected daemons    */

/*--------------------------------------------------------------------------
 * nsmctlclient class
 *--------------------------------------------------------------------------*/

class nsmctlclient
{
    osc::endpoint * m_osc_server;       /* pointer owned by nsmcontroller   */
    daemon_list & m_daemon_list;        /* list owned by the application    */
    std::string m_client_id;
    std::string m_client_label;
    std::string m_client_name;
    float m_progress;
    bool m_dirty;
    bool m_visible;

public:

    nsmctlclient () = delete;
    nsmctlclient
    (
        osc::endpoint * oscserver,
        daemon_list & daemonlist,
        const std::string & client_id,
        const std::string & client_label,
        const std::string & client_name
    );
    ~nsmctlclient () = default;     // TODO

    std::string info (const std::string & tag = "") const;

    const std::string & client_name () const
    {
        return m_client_name;
    }

    void name (const std::string & v)
    {
        m_client_name = v;
    }

    const std::string & client_label () const
    {
        return m_client_label;
    }

    void client_label (const std::string & s)
    {
        m_client_label = s;
    }

    const std::string & client_id () const
    {
        return m_client_id;
    }

    void client_id (const std::string & i)
    {
        m_client_id = i;                            /* take that, FORTRAN!  */
    }

    void progress (float f)
    {
        m_progress = f;
    }

    void dirty (bool b)
    {
        m_dirty = b;
    }

    void gui_visible (bool b)
    {
        m_visible = b;
    }

    void stopped (bool b);
    void pending_command (const std::string & command);
    bool send_client_message (osc::tag o);

};          // class nsmctlclient

}           // namespace nsm

#endif      // NSM_NSMCTLCLIENT_HPP

/*
 * nsmctlclient.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


