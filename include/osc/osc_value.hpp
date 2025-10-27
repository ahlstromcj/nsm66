#if ! defined NSM66_OSC_OSC_VALUE_HPP
#define NSM66_OSC_OSC_VALUE_HPP

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
 * \file          osc_value.hpp
 *
 *    This module refactors the osc_value classes to replace C code with
 *    C++ code.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-02-06
 * \updates       2025-10-27
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   To do.
 */

#include <list>
#include <string>

#include "cpp_types.hpp"                /* CSTR() inline functions          */

namespace osc
{

class osc_value
{

protected:

    char m_type;
    float m_f;
    double m_d;
    int m_i;
    std::string m_s;

public:

    osc_value () = default;                         /* values = 0 or empty  */
    osc_value (const osc_value & rhs) = default;
    virtual ~osc_value () = default;

    virtual char type () const
    {
        return m_type;
    }
};          // class osc_value

using osc_value_list = std::list<osc_value>;

class osc_float : public osc_value
{

public:

    osc_float () = default;                         /* values = 0 or empty  */

    osc_float (float v) : osc_value ()
    {
        m_type = 'f';
        m_f = v;
    }

    float value () const
    {
        return m_f;
    }

};          // class osc_float

class osc_int : public osc_value
{

public:

    osc_int () = default;                           /* values = 0 or empty  */

    osc_int (int v) : osc_value ()
    {
        m_type = 'i';
        m_i = v;
    }

    int value () const
    {
        return m_i;
    }

};          // class osc_int

class osc_string : public osc_value
{

public:

    osc_string (const std::string & v) : osc_value ()
    {
        m_type = 's';
        m_s = v;
    }

    osc_string (const char * v) : osc_value ()
    {
        m_type = 's';
        m_s = std::string(v);
    }

    const std::string & value () const
    {
        return m_s;
    }

    const char * value_ptr () const
    {
        return CSTR(m_s);
    }

};          // class osc_string

}           // namespace osc

#endif      // NSM66_OSC_OSC_VALUE_HPP

/*
 * osc_value.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

