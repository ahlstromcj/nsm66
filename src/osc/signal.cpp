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
 * along with New-Session-Manager. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file          signal.hpp
 *
 *    This module refactors the signal class to replace C code with
 *    C++ code.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-02-05
 * \updates       2025-04-05
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   To do.
 */

#include "osc/endpoint.hpp"             /* osc::endpoint class              */
#include "osc/signal.hpp"               /* osc::signal class                */
#include "util/msgfunctions.hpp"        /* util::info_printf() from cfg66   */
#include "util/strfunctions.hpp"        /* util::string_format() from cfg66 */

namespace osc
{

signal::signal (const std::string & path, direction dir) :
    m_endpoint      (),
    m_peer          (),
    m_path          (path),
    m_documentation (),
    m_value         (0.0f),
    m_direction     (dir),
    m_handler       (),
    m_user_data     (),
    m_parameter_limits          (),
    m_connection_state_callback (),
    m_connection_state_userdata ()
{
    // no code
}

signal::~signal ( )
{
    if (not_nullptr(m_endpoint))
    {
        m_endpoint->del_signal(this);

        /*
         * We're gone!
         *
         * m_endpoint = nullptr;
         */
    }
}

void
signal::set_parameter_limits (float min, float max, float default_value)
{
    m_parameter_limits.pl_min = min;
    m_parameter_limits.pl_max = max;
    m_parameter_limits.pl_default_value = default_value;
    m_value = default_value;
}

void
signal::connection_state_callback (callback cb, void * userdata)
{
    m_connection_state_callback = cb;
    m_connection_state_userdata = userdata;
}

/**
 * asprintf(char ** strp, const char * fmt, ...) allocates a string, returns
 * the pointer via strp, and prints the varargs to it.
 *
 * We can use util::string_format() from the cfg66 library instead.
 *
 * We need to validate m_endpoint.
 */

void
signal::rename (const std::string & path)
{
    std::string newpath = util::string_format
    (
        "%s%s", V(m_endpoint->name()), V(path)
    );
    util::info_printf("Renaming signal %s to %s", V(path), V(newpath));
    lo_server_del_method(m_endpoint->server(), m_path.c_str(), NULL);
    lo_server_add_method
    (
        m_endpoint->server(), newpath.c_str(), NULL,
        m_endpoint->osc_sig_handler, this
    );
    for (auto & p : m_endpoint->m_peers)
    {
        m_endpoint->send
        (
            p->p_addr, tag_message(tag::sigrenamed), m_path, newpath
        );
    }
    m_endpoint->rename_translation_destination(m_path, newpath);
    m_path = newpath;
}

void
signal::value (float f)
{
    if (f == m_value)
        return;

    m_value = f;
    if (get_direction() == output )
    {
        for (auto & p : m_endpoint->m_peers)
            m_endpoint->send(p->p_addr, path(), f);
    }
}

}           // namespace osc

/*
 * signal.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

