#if ! defined NSM66_OSC_METHOD_HPP
#define NSM66_OSC_METHOD_HPP

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
 * \file          method.hpp
 *
 *    This module refactors the method class to replace C code with
 *    C++ code.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-02-05
 * \updates       2025-02-05
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   To do.
 */

#include <list>
#include <string>

namespace osc
{

class endpoint;
class signal;

/*
 * Moved to signal.hpp
 *
 * using signal_hander = int (*) (float value, void * user_data);
 */

/**
 *  To do
 */

class method
{
    friend class endpoint;

private:

    std::string m_path;
    std::string m_typespec;
    std::string m_documentation;

public:

    const std::string & path () const
    {
        return m_path;
    }

    const char * path_pointer ()
    {
        return m_path.c_str();
    }

    const std::string & typespec () const
    {
        return m_typespec;
    }

    const char * typespec_pointer ()
    {
        return m_typespec.c_str();
    }

    const std::string & documentation () const
    {
        return m_documentation;
    }

    method () = default;
    method (const method &) = delete;
    method & operator = (const method &) = delete;
    ~method () = default;

};

/**
 *  A little easier to read and write. Should we use vector instead?
 */

using method_list = std::list<method *>;

}           // namespace osc

#endif      // NSM66_OSC_METHOD_HPP

/*
 * method.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

