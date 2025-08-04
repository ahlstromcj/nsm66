#if ! defined NSM66_OSC_SIGNAL_HPP
#define NSM66_OSC_SIGNAL_HPP

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

#include <list>
#include <string>
#include <lo/lo.h>

#include "method.hpp"

namespace osc
{

class signal;

using signal_handler = int (*) (float value, void * user_data);

struct parameter_limits
{
    float pl_min;
    float pl_max;
    float pl_default_value;
};

using signal_list = std::list<signal *>;

struct peer
{
    bool p_scanning;
    std::string p_name;             // char * name;
    lo_address p_addr;
    signal_list p_signals;
};

using peer_list = std::list<peer *>;

class signal
{
    friend class endpoint;

public:

    enum state
    {
        created = 0,
        removed = 1
    };

    enum direction
    {
        input,
        output,
        bidirectional
    };

    using callback = void (*) (osc::signal *, void *);

private:

    endpoint * m_endpoint;          // shared or unique?

    peer * m_peer;                  // ditto?

    std::string m_path;             // char * _path;

    std::string m_documentation;    // char * _documentation;

    float m_value;

    direction m_direction;

    signal_handler m_handler;

    void * m_user_data;

    parameter_limits m_parameter_limits;

    void (* m_connection_state_callback) (osc::signal *, void *);

    void * m_connection_state_userdata;

public:

    signal () = default;
    signal (const std::string & path, direction dir);
    ~signal ();

    direction get_direction () const
    {
        return m_direction;
    }

    void connection_state_callback (callback cb, void * userdata);
    void set_parameter_limits (float min, float max, float default_value);

    const parameter_limits & get_parameter_limits () const
    {
        return m_parameter_limits;
    }

    const std::string & peer_name () const
    {
        return m_peer->p_name;
    }

    const char * peer_name_pointer () const
    {
        return m_peer->p_name.c_str();
    }

    const std::string & path () const
    {
        return m_path;
    }

    const char * path_pointer () const
    {
        return m_path.c_str();
    }

    void rename (const std::string & name);

    /*
     * Publishes a value to targets or gets the current value.
     */

    void value (float v);
    float value () const
    {
        return m_value;
    }

    // NOT DEFINED!
    // bool is_connected_to ( const signal *s ) const;

};

}           // namespace osc

#endif      // NSM66_OSC_SIGNAL_HPP

/*
 * signal.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

