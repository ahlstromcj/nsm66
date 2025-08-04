/*
 *  This file is part of nsm66.
 *
 *  nsm66 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  nsm66 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with nsm66; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          nsm66.cpp
 *
 *  This module defines an informational function.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom
 * \date          2025-01-29
 * \updates       2025-01-29
 * \license       GNU GPLv2 or above
 *
 */

#include "nsm66.hpp"                   /* no-namespace function library    */

/*
 * Version information string.
 */

const std::string &
nsm66_version () noexcept
{
    static std::string s_info = NSM66_NAME "-" NSM66_VERSION " " __DATE__ ;
    return s_info;
}

/*
 * nsm66.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

