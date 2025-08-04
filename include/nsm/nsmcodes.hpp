#if ! defined NSM66_NSM_NSMCODES_HPP
#define NSM66_NSM_NSMCODES_HPP

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
 * \file          nsmcodes.hpp
 *
 *    This module provides enumeration values used by NSM code.
 *    C++ code.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-04-05
 * \updates       2025-04-05
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   The nsmcodes class isolates the enumerations into an easy-to-find
 *   module.
 */

#include <map>
#include <string>
#include <lo/lo.h>

#include "osc/lowrapper.hpp"            /* osc::lowrapper base class        */
#include "osc/method.hpp"               /* osc::method, method_list alias   */
#include "osc/osc_value.hpp"            /* osc::osc_value_list              */
#include "osc/signal.hpp"               /* osc::signal, peer & signal lists */
#include "osc/thread.hpp"               /* osc::thread                      */

#define OSC_DMSG() MESSAGE("Got OSC message: %s", path);

namespace nsm
{

/**
 *  Provides reply codes matching those of NSM's Client class.
 *  These codes were defined in the nsmd.cpp file, and are currently
 *  used exclusively by that application.
 */

enum error
{
    ok                  =    0,
    general             =   -1,         /* some clients couldn't save       */
    incompatible_api    =   -2,         /* version too old for client       */
    blacklisted         =   -3,         /* an unused vengeful error code    */
    launch_failed       =   -4,         /* bad path, launch failure         */
    no_such_file        =   -5,         /* non-existent session.nsm file    */
    no_session_open     =   -6,         /* no session active or to save     */
    unsaved_changes     =   -7,         /* unused error code                */
    not_now             =   -8,         /* unused error code                */
    bad_project         =   -9,         /* unused error code                */
    create_failed       =  -10,         /* couldn't open session because... */
    session_locked      =  -11,         /* session locked by lock-file      */
    operation_pending   =  -12,         /* op in progess, try next handler  */
    save_failed         =  -99          /* doesn't exist in the Non project */
};

/**
 *  Provides command codes matching those of NSM's Client class.
 *  Used only in the nsmd application.
 */

enum command
{
    none = 0,
    quit,
    kill,
    save,
    open,
    start,
    close,
    duplicate,
    new_session
};

}           // namespace nsm

#endif      // defined NSM66_NSM_NSMCODES_HPP

/*
 * nsmcodes.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

