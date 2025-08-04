#if ! defined NSM66_NSM_HELPERS_HPP
#define NSM66_NSM_HELPERS_HPP

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
 * \file          helpers.hpp
 *
 *    This module refactors the nsmd application to replace C code with
 *    C++ code.
 *
 * \library       helpers application
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-03-01
 * \updates       2025-04-20
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   Declares functions and data items for working with port names, random
 *   numbers, various NSM files and directories, and parsing.
 */

#include <vector>                       /* std::vector<>                    */

#include "c_macros.h"
#include "cpp_types.hpp"                /* lib66::tokenization alias        */

namespace nsm
{

/**
 *  Return value for the process_patch_*() functions.
 */

enum class patch_direction
{
    left,                               /* "<" */
    duplex,                             /* "|" */
    right,                              /* ">" */
    error
};

/**
 *  A structure to store a line item from the session.nsm file. For the
 *  GNU compiler, we had to first give the structure a name; the construct
 *  "using session_triplet" yields the following diagnostic, which
 *  explains why we couldn't link to the parse_session_triplets() function.
 *
 *  "does not refer to the unqualified type, so it is not used for linkage"
 *
 *  See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99931 for more.
 *
 *  No need for this: using session_triplet = struct triplet;
 */

struct session_triplet
{
    std::string st_client_name;         /* official app name (e.g. seq66)   */
    std::string st_client_exe;          /* base name of app (e.g. qseq66)   */
    std::string st_client_id;           /* random client ID (e.g. nPSLM)    */
};

/**
 *  A container to store all the line items from the session.nsm file.
 */

using session_triplets = std::vector<session_triplet>;

/*---------------------------------------------------------------------------
 * Free functions in the nsm namespace.
 *--------------------------------------------------------------------------*/

extern bool valid_jack_port_char (char c);
extern bool valid_jack_port_name (const std::string & portname);
extern std::string fix_jack_port_name (const std::string & portname);
extern int generate_rand (int range);
extern std::string generate_client_id (const std::string & format);
extern bool mkpath (const std::string & path, bool createfinaldir = true);
extern std::string get_lock_file_name
(
    const std::string & lockdirectory,
    const std::string & sessionname,
    const std::string & absolutesessionpath
);
extern bool write_lock_file
(
    const std::string & filename,
    const std::string & session_path,
    const std::string & serverurl
);
extern bool delete_lock_file (const std::string & filename);
extern bool parse_session_triplet
(
    const std::string & line,
    session_triplet & destination
);
extern std::string session_triplet_to_string (const session_triplet & t);
extern session_triplets parse_session_lines (const std::string & sessionfile);
extern bool make_xdg_runtime_lock_directory (std::string & lockfiledir);
extern std::string lookup_active_nsmd_url ();
extern std::string get_daemon_pid_file ();
extern bool make_daemon_directory
(
    const std::string & directory,
    std::string & outdirectory
);
extern bool make_session_root (std::string & outdirectory);

#if defined USE_PROCESS_PATCH_SSCANF

extern patch_direction process_patch_sscanf
(
    const std::string & patch,
    std::string & left_capture,
    std::string & left_playback,
    std::string & right_capture,
    std::string & right_playback
);
extern bool extract_client_port_sscanf
(
    const std::string & portname,
    std::string & clientpart,
    std::string & portpart
);

#endif

extern patch_direction extract_patch_line
(
    const std::string & patch,
    std::string & leftside,
    std::string & rightside
);
extern bool extract_client_port
(
    const std::string & fullname,
    std::string & clientname,
    std::string & portname
);
extern patch_direction process_patch
(
    const std::string & patch,
    std::string & left_client,
    std::string & left_port,
    std::string & right_client,
    std::string & right_port
);

}           // namespace nsm

#endif      // defined NSM66_NSM_HELPERS_HPP

/*
 * helpers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

