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
 * \file          helpers.cpp
 *
 *    This module helps to refactor the nsmd application to replace C code
 *    with C++ code.
 *
 * \library       helpers application
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-03-03
 * \updates       2025-04-20
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   This module contains functions extracted from the various NSM
 *   programs and placed here for re-use and to enhance the readability
 *   of the program code.
 */

#include <algorithm>                    /* std::find()                      */
#include <cctype>                       /* std::isalnum()                   */
#include <cerrno>                       /* #include <errno.h>               */
#include <cstring>                      /* std::strerror()                  */
#include <cstdlib>                      /* std::getenv(), std::rand()       */
#include <sys/time.h>                   /* time() and time_t                */
#include <unistd.h>                     /* getpid()                         */

#include "c_macros.h"
#include "cpp_types.hpp"                /* lib66::tokenization alias        */
#include "nsm/helpers.hpp"              /* functions in this module         */
#include "util/filefunctions.hpp"       /* cfg66: util::file_write_lines()  */
#include "util/ftswalker.hpp"           /* util::ftswalker class & funcs    */
#include "util/msgfunctions.hpp"        /* cfg66: util::string_asprintf()   */
#include "util/strfunctions.hpp"        /* cfg66: util::simple_hash()       */

namespace nsm
{

/**
 *  Code adapted from a2jmidid/port.c. This should be part of JACK API.
 *  Note that the space character is a valid character in the port
 *  name.
 */

bool
valid_jack_port_char (char c)
{
    return
    (
        std::isalnum(c) || c == ' ' ||
        c == '/' || c == '_' || c == ':' || c == '(' ||
        c == ')' || c == '-' || c == '[' || c == ']'
    );
}

bool
valid_jack_port_name (const std::string & portname)
{
    bool result = true;
    for (auto c : portname)
    {
        if (! valid_jack_port_char(c))
        {
            result = false;
            break;
        }
    }
    return result;
}

/**
 *  Replaces an invalid character with a space, in place.
 *
 *  Returns the fixed string or the original string.
 */

std::string
fix_jack_port_name (const std::string & portname)
{
    std::string result;
    for (auto c : portname)
        result += valid_jack_port_char(c) ? c : ' ' ;

    return result;
}

/**
 *  These can be combined, with a hidden list. Operations:
 *
 *      -   reseed
 *      -   generate random number & add it to the list
 *      -   clear the list
 *
 *  A small enumeration { calculate [default], reseed, clear }
 */

/*
 * Generate a random number for the client IDs generation.
 * It generates a seed the first time.
 *
 * \param range
 *      Provides a range limit. For example, 26 will generate numbers
 *      from 0 to 25. If set to 0, the values are in [0, RAND_MAX].
 *
 * \return
 *      Returns a random number, simplistically.
 */

int
generate_rand (int range)
{
    static bool s_uninitialized = true;
    if (s_uninitialized)
    {
        time_t seconds;
        time(&seconds);
        srand((unsigned int)(seconds));
        s_uninitialized = false;
    }
    return range > 0 ? (std::rand() % (range - 1)) : std::rand() ;
}

/*
 *  Before v1.4 this returned "n" + 4 random upper-case letters, which could
 *  lead to collisions.  We changed behaviour to still generate 4 letters,
 *  but check for collision with existing IDs.
 *
 *  Loaded client IDs are not checked, just copied from "session.nsm" because
 *  loading happens before any generation of new clients. Loaded clients are
 *  part of further checks of course.
 *
 *  There is a theoretical limit when all 26^4 IDs are in use which will lead
 *  to an infinite loop of generation. We risk leaving this unhandled.
 *
 * \param format
 *      Defines the length and layout of the ID. Consecutive '-' characters
 *      determine the length. Other characters can surround them.
 *      An example used by the nsm66d application is "n----".
 *
 * \return
 *      Returns the random string. If the format does not contain hyphens,
 *      an empty string is returned.
 */

std::string
generate_client_id (const std::string & format)
{
    static lib66::tokenization s_id_list;   /* vector of strings            */
    std::string id = format;
    for (;;)
    {
        int count = 0;
        for (std::string::iterator i = id.begin(); i != id.end(); ++i)
        {
            if (*i == '-')
            {
                *i = 'A' + generate_rand(26);   /* i.e. 0 to 25, 'A' to 'Z' */
                ++count;
            }
        }
        auto found = std::find(s_id_list.begin(), s_id_list.end(), id);
        if (found == s_id_list.end() && count > 0)
        {
            s_id_list.push_back(id);
            break;
        }
        if (count == 0)
        {
            id.clear();
            break;

        }
    }
    return id;
}

/**
 *  This function creates a path, optionally including the last
 *  sub-directory in the path.
 *
 *  Use util::make_directory_path(p, 0711). It does not need to find
 *  and make each subdirectory here.
 *
 * \param path
 *      The full path-name to be created.
 *
 * \param createfinaldir
 *      If false, the last sub-directory is not included in the path.
 *      The default is true.
 *
 * \return
 *      Returns true if the directory could be created.
 */

bool
mkpath (const std::string & path, bool createfinaldir)
{
    bool result = false;
    if (createfinaldir)                     /* create everything            */
    {
        result = util::make_directory_path(path, 0711);
    }
    else
    {
        auto endpos = path.find_last_of("/");
        if (endpos < path.length())
        {
            std::string shortpath = path.substr(0, endpos);
            result = util::make_directory_path(shortpath, 0711);
        }
    }
    return result;
}

/*
 *  To avoid collisions of two simple session names under either different
 *  sub-directories or even different session roots, we use a hash.
 *
 * \param lockdirectory
 *      Use s_lockfile_directory for this parameter in nsmd.
 *
 * \param sessionname
 *      Use s_session_name for this parameter in nsmd.
 *
 * \param absolutesessionpath
 *      Use s_session_path for this parameter in nsmd.
 *
 */

std::string
get_lock_file_name
(
    const std::string & lockdirectory,
    const std::string & sessionname,
    const std::string & absolutesessionpath
)
{
    std::string sessionhash = util::simple_hash(absolutesessionpath);
    std::string sessionlock = util::string_asprintf
    (
        "%s/%s%s", V(lockdirectory), V(sessionname), V(sessionhash)
    );
    return sessionlock;
}

/*
 *  Not a GNU lockfile, which features were never used by nsmd anyway,
 *  but simply a file with information about the NSM Server and the
 *  loaded session.
 *
 *  For this new version of the code, util::file_write_string()
 *  used, which prepends the file-name and date-time, then *appends*
 *  the string to the file.
 *
 * \param filename
 *      Provides the lock-file name, obtained from get_lock_file_name().
 *
 * \param sessionpath
 *      Provides the session path, e.g. s_session_path in nsmd.
 *
 * \param serverurl
 *      Provides the OSC server URL, e.g. s_osc_server->url().
 *
 * \return
 *      Returns true if the lock-file could be written.
 */

bool
write_lock_file
(
    const std::string & filename,
    const std::string & sessionpath,
    const std::string & serverurl
)
{
    std::string lockdata = util::string_asprintf
    (
        "%s\n%s\n%d\n", V(sessionpath), V(serverurl), int(getpid())
    );
    bool result = util::file_write_string(filename, lockdata);
    if (result)
    {
        util::file_message("Created lock file", filename);
    }
    else
    {
        int ec = errno;
        util::error_printf
        (
            "Failed to write lock file %s; error: %s",
            V(filename), std::strerror(ec)
        );
    }
    return result;
}

/**
 *  Deletes the given lock-file. This is used in close_session() and
 *  load_session_file.
 *
 *  Do we need boolean return value?
 *
 * \param filename
 *      The name of the lock-file as obtained by get_lock_file_name().
 */

bool
delete_lock_file (const std::string & filename)
{
    bool result = util::file_delete(filename);
    if (result)
    {
        util::file_message("Deleted lock file", filename);
    }
    else
    {
        int ec = errno;
        util::error_printf
        (
            "Failed to delete lock file %s; error: %s",
            V(filename), std::strerror(ec)
        );
    }
    return result;
}

/**
 *  A helper function for parse_session_triplet(). It tries to scan the
 *  file and create a list of client specifications.
 *  Copying not an issue here.
 *
 *  Here is a sample:
 *
 *      Data-Storage:nsm-data:nQPEJ
 *      JACKPatch:jackpatch:nLWNW
 *      qsynth:qsynth:nLMTD
 *      seq66:qseq66:nPSLM
 *      qjackctl:qjackctl:nDVTC
 *
 * \param line
 *      The input line, like one of the samples above.
 *
 * \param [out] destination
 *      The destination for the client name, executable, and ID.
 *
 * \return
 *      Returns true if the 3 tokens could be retrieved.
 */

bool
parse_session_triplet
(
    const std::string & line,
    session_triplet & destination
)
{
    lib66::tokenization tokens = util::tokenize(line, ":");
    bool result = tokens.size() == 3;
    if (result)
    {
        destination.st_client_name = tokens[0];
        destination.st_client_exe = tokens[1];
        destination.st_client_id = tokens[2];
    }
    return result;
}

/**
 *  Converts the triplet structure to the format shown above
 *  in parse_session_triplet().
 */

std::string
session_triplet_to_string (const session_triplet & t)
{
    std::string result;
    bool ok =
    (
        ! t.st_client_name.empty() &&
        ! t.st_client_exe.empty() &&
        ! t.st_client_id.empty()
    );
    if (ok)
    {
        result = util::string_asprintf
        (
            "%s:%s:%s",
            V(t.st_client_name), V(t.st_client_exe), V(t.st_client_id)
        );
    }
    return result;
}

/**
 *  A helper function for load_session_file(). It tries to scan the
 *  session_triplets line-by-line, storing them in a vector.
 */

session_triplets __attribute__((used))
parse_session_lines (const std::string & sessionfile)
{
    session_triplets result;
    lib66::tokenization lines;
    bool ok = util::file_read_lines(sessionfile, lines, true);  /* trim */
    if (ok)
    {
        for (const auto & line : lines)
        {
            session_triplet lineitem;
            if (parse_session_triplet(line, lineitem))
                result.push_back(lineitem);
            else
                break;
        }
    }
    return result;
}

/**
 *  Get the XDG runtime directory for lockfiles. See
 *
 * https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
 *
 *  Unlike $XDG_DATA_HOME, the runtime environment variable must be set,
 *  usually to "/run/user/<id>/".
 *
 *  If the system is not XDG compliant (var not set) we fall back to
 *  "/run/user/<id> directly. This is in accordance with
 *
 * https://en.wikipedia.org/wiki/Filesystem_Hierarchy_Standard 3.0
 *
 * getuid(2) man page:
 *
 *  "These functions are always successful and never modify errno."
 *
 *  We call it in case the user was changed for this process. Usually 1000,
 *  for the first user.
 *
 * \param [out] lockfiledir
 *      Provides the lock-file subdirectory, usually "nsm".
 *
 * \return
 *      Returns true if the directory could be created/accessed.
 *
 *  Note: compare this function to util::make_xdg_runtime_directory().
 */

bool
make_xdg_runtime_lock_directory (std::string & lockfiledir)
{
    std::string dirtemp = util::get_xdg_runtime_directory("nsm");
    bool result = ! dirtemp.empty();
    if (result)
    {
        result = util::make_directory_path(dirtemp, 0771);
        if (result)
        {
            /*
             * Might be redundant.
             *
             * util::info_message("Directory for lock-files", dirtemp);
             */

            lockfiledir = dirtemp;
        }
        else
        {
            int ec = errno;
            util::error_printf
            (
                "Failed to create lock file directory %s with error: %s",
                V(dirtemp), std::strerror(ec)
            );
        }
    }
    return result;
}

/**
 *  This function tries to look up /run/user/<id>/nsm directory.
 *
 *  The first file is like "/run/user/1000/nsm/seq0", where "seq" is
 *  the name of an NSM section.  It has contents like the following:
 *
 *      /home/user/.local/share/nsm/seq
 *      osc.udp://mlsleno:16133/
 *      37739
 *
 *  The second file is "/run/user/1000/nsm/d/37739. It's contents:
 *
 *      osc.udp://mlsleno:16133/
 *
 *  Thus, an easy way to hook to the first daemon we find is to
 *  look for that second file and read it.
 */

std::string
lookup_active_nsmd_url ()
{
    std::string result;
    std::string runtimedir = util::get_xdg_runtime_directory("nsm", "d");
    if (! runtimedir.empty())
    {
        util::ftswalker walker(runtimedir);
        lib66::tokenization files;
        if (walker.find_regular_files(files))
        {
            std::string f0 = files[0];
            lib66::tokenization lines;
            if (util::file_read_lines(f0, lines))
            {
                f0 = lines[0];
                if (util::strncompare("osc", f0))
                    result = f0;
            }
        }
    }
    return result;
}

/**
 *  Get the daemon directory plus the PID file. This is useful in reading
 *  the PID file.
 */

std::string
get_daemon_pid_file ()
{
    std::string result;
    std::string daemondir = util::get_xdg_runtime_directory("nsm", "d");
    if (! daemondir.empty())
    {
        result = util::string_asprintf
        (
            "%s/%d", V(daemondir), getpid()
        );
        util::info_message("Daemon file", daemondir);
    }
    else
        util::error_message("Could not get a daemon file-name");

    return result;
}

/**
 *  Creates another sub-directory for daemons ".../nsm/d/" where each
 *  daemon has a port number file.  The actual daemon file will be written
 *  by the application after announcing the session URL.
 *
 * \param directory
 *      This is the lock-file directory, usually s_lockfile_directory.
 *
 * \param [out] daemonfile
 *      Holds the name of the full path to the daemon file. The first
 *      part of the path, "..../d" is created. Then the PID is
 *      appended and returned in this parameter. In nsmd, the is the
 *      s_daemon_file value.
 *
 * \return
 *      Returns true if the creation of the daemon-file "..../d"
 *      was successful.
 */

bool
make_daemon_directory
(
    const std::string & directory,
    std::string & daemonfile
)
{
    std::string daemondirectory = util::string_asprintf("%s/d", V(directory));
    bool result = util::make_directory_path(daemondirectory, 0771);
    if (result)
    {
        daemonfile = util::string_asprintf
        (
            "%s/%d", V(daemondirectory), getpid()
        );
        util::info_message("Daemon file", daemonfile);
    }
    else
    {
        util::error_printf
        (
            "Failed to create daemon file directory %s with error: %s",
            V(daemondirectory), std::strerror(errno)
        );
    }
    return result;
}

/*
 *  The user gave no specific session directory. We use the default.
 *  The default directory follows the XDG Basedir Specifications.
 *
 * https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
 *
 *  It is used by looking up environment variables. $XDG_DATA_HOME defines
 *  the base directory relative to which user-specific data files
 *  should be stored. If $XDG_DATA_HOME is either not set or empty,
 *  a default equal to "$HOME/.local/share" should be used.
 *
 *  Up to version 1.5.3 the default directory was "~/NSM Sessions".
 *  If this old directory exists we will use it but write mild warning
 *  to the log.  Moving old sessions is left to the user, or an external GUI.
 *
 *  If $XDG_DATA_HOME is explicitly set by the user we assume it to exist.
 *  We don't want to recursively create system directories. If the
 *  XDG-directory does not exist yet, we FATAL out just below.
 *
 *  If $XDG_DATA_HOME is either not set or empty, a default equal to
 *  "$HOME/.local/share" should be used.
 *
 * \param [out] outdirectory
 *      The session-root directory, e.g. s_session_root in nsmd.
 *
 * \return
 *      Returns true if the directory could be created/accessed.
 */

bool
make_session_root (std::string & outdirectory)
{
    bool result = true;
    std::string dirtemp;
    char * home = std::getenv("HOME");
    if (not_nullptr(home))
        dirtemp = util::string_asprintf("%s/%s", home, "NSM Sessions");

    if (util::file_exists(dirtemp))
    {
        util::warn_message
        (
            "Old-style session directory found. Usable, but "
            "better to move sessions to $XDG_DATA_HOME/nsm/. "
            "To see the new directory, rename your current "
            "session-directory and restart nsmd. Current session:",
            dirtemp
        );
    }
    else
    {
        std::string xdg_data_home;
        char * env = std::getenv("XDG_DATA_HOME");
        if (not_nullptr(env))
            xdg_data_home = env;

        if (xdg_data_home.empty())
        {
            if (not_nullptr(home))
                dirtemp = util::string_asprintf("%s/.local/share/nsm", home);
        }
        else
        {
            dirtemp = util::string_asprintf("%s/%s", V(xdg_data_home), "nsm");
        }
    }
    result = util::make_directory_path(dirtemp, 0771);
    if (result)
    {
        util::info_message("Session root", dirtemp);
        outdirectory = dirtemp;
    }
    else
    {
        int ec = errno;
        util::error_printf
        (
            "Failed to create session directory %s with error: %s",
            result, std::strerror(ec)
        );
    }
    return result;
}

#if defined USE_PROCESS_PATCH_SSCANF

/**
 * Convert a symbolic string of a JACK connection into an actual data struct
 * patch_record.
 *
 *  The name of the patch file is like this: "JACKPatch.nLWNW.jackpatch".
 *
 *  Examples of patches in this file. All items are one-liners, even if long:
 *
 *      PulseAudio JACK Sink:front-left |> system:playback_1
 *
 *      a2j:Launchpad Mini (capture): Launchpad Mini MIDI 1 |>
 *              seq66.nPSLM:a2j:Launchpad Mini (capture): Launchpad Mini MIDI 1
 *
 *      seq66.nPSLM:a2j:Launchpad Mini (playback): Launchpad Mini MIDI 1 |>
 *              a2j:Launchpad Mini (playback): Launchpad Mini MIDI 1
 *
 *      seq66.nPSLM:fluidsynth-midi:midi_00 |> fluidsynth-midi:midi_00
 *
 *          ----------------------------------------- left client
 *         |       ---------------------------------- left port
 *         |      |
 *         |      |                ------------------ right client
 *         |      |               |       ----------- right port
 *         |      |               |      |
 *         v      v               v      v
 *      " %m[^:]:%m[^|] |%1[<>|] %m[^:]:%m[^\n]",
 *       ^ ^ ^  ^   ^   ^ ^ ^^    ^
 *       | | |  |   |   | | dir   |
 *       | | |  |   |   | |        -----> The rest are simple.
 *       | | |  |   |   |  ----> Get only one of the <, >, or | characters.
 *       | | |  |   |    ------> Absorb the pipe
 *       | | |  |    ----------> Fetch all characters up to the pipe
 *       | | |   --------------> Absorb the colon
 *       | |  -----------------> See "[^:]" notes below
 *       |  -------------------> A character pointer allocated by sscanf()
 *        ---------------------> Throw-away white space
 *
 *  "[^:]"
 *
 *      This token suppresses white-space scanning. The characters in the
 *      square brackets are the characters to be accepted as data; but here,
 *      the "^:" means "accept all characters that are not a colon".
 *
 *  Issue:
 *
 *      One issue is that, with a2jmidid running (on older JACK setups),
 *      the client-name itself will have a colon, screwing up the parsing
 *      above.
 *
 *  At some point we would like to use something a bit easier to read, such
 *  as the INI or JSON formats. XML, as used in ajsnapshot, is overkill.
 *
 *  The 5 elements processed are the source client and port, the destination
 *  client and port, and the directional indicator ("<", ">", and "|>".
 *
 * \return
 *      Returns 0 if the 5 elements were not found. Returns -1 if an EOF was
 *      encountered.
 *
 * WARNING: ISO C++11 doesn't support the 'm' scanf() flag.
 *
 * TODO: write a test for this function
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"

patch_direction
process_patch_sscanf
(
    const std::string & patch,
    std::string & left_client,
    std::string & left_port,
    std::string & right_client,
    std::string & right_port
)
{
    patch_direction result = patch_direction::error;    /* be pessimistic   */
    left_client.clear();
    left_port.clear();
    right_client.clear();
    right_port.clear();

    /*
     * Pointers filled in by %m by sscanf(), which is not portable.
     */

    char * leftc;               /* left client          */
    char * leftp;               /* left port            */
    char * rightc;              /* right client         */
    char * rightp;              /* right port           */
    char dir[4];                /* < | > direction      */
    int count = sscanf
    (
        CSTR(patch), " %m[^:]:%m[^|] |%1[<>|] %m[^:]:%m[^\n]",
        &leftc, &leftp, dir, &rightc, &rightp
    );

    if (count == 5)
    {
        /*
         * Trim space
         */

        for (int j = strlen(leftp) - 1; j > 0; --j)
        {
            if (leftp[j] == ' ' || leftp[j] == '\t')
                leftp[j] = 0;
            else
                break;
        }
        switch (dir[0])
        {
            case '<':               /* this character is not used, AFAICT   */

                result = patch_direction::left;
                break;

            case '>':

                result = patch_direction::right;
                break;

            case '|':

                result = patch_direction::duplex;
                break;

            default:

                break;
        }
        if (result != patch_direction::error)
        {
            left_client = leftc;
            left_port = leftp;
            right_client = rightc;
            right_port = rightp;
        }
    }
    if (not_nullptr(leftc))
        free(leftc);

    if (not_nullptr(leftp))
        free(leftp);

    if (not_nullptr(rightc))
        free(rightc);

    if (not_nullptr(rightp))
        free(rightp);

    return result;
}

#pragma GCC diagnostic pop

bool
extract_client_port_sscanf
(
    const std::string & portname,
    std::string & clientpart,
    std::string & portpart
)
{
    char client[512];                           /* Linux JACK limit is 64   */
    char port[512];                             /* Linux JACK limit is 256  */
    int count = sscanf(CSTR(portname), "%[^:]:%[^\n]", client, port);
    bool result = count == 2;
    clientpart.clear();
    portpart.clear();
    if (result)
    {
        clientpart = client;
        portpart = port;
    }
    return result;
}

#endif  // defined USE_PROCESS_PATCH_SSCANF

/**
 *  Defined because a2j client:port names have an extra colon, so the
 *  sscanf() method does not work, as far as we can tell.
 *
 *  The format of a patch line is, as far as we can tell, something like
 *  this:
 *
 *          left             direction            right
 *      "client:port         separator         client:port"
 *
 *  where the direction separator is one of the following:
 *
 *      "<|" -  Right is the source port, left is the destination port.
 *      "||" -  The connection is duplex, so that two patches are specified.
 *      "|>" -  Left is the source port, right is the destination port.
 *
 *  This function grabs the left and right sides of the direction separator.
 */

patch_direction
extract_patch_line
(
    const std::string & patch,
    std::string & leftside,
    std::string & rightside
)
{
    patch_direction result = patch_direction::error;    /* be pessimistic   */
    if (! patch.empty())
    {
        std::string::size_type leftposend = patch.find_first_of("<|>");
        std::string::size_type rightposstart = patch.find_last_of("<|>");
        if (util::not_npos(leftposend) && util::not_npos(rightposstart))
        {
            std::string leftpart = patch.substr(0, leftposend - 1);
            std::string rightpart = patch.substr(rightposstart + 1);
            std::string::size_type sepcount = rightposstart - leftposend + 1;
            std::string separator = patch.substr(leftposend, sepcount);
            leftpart = util::trim(leftpart);
            rightpart = util::trim(rightpart);
            if (separator == "<|")
                result = patch_direction::left;
            else if (separator == "||")
                result = patch_direction::duplex;
            else if (separator == "|>")
                result = patch_direction::right;

            if (result != patch_direction::error)
            {
                leftside = leftpart;
                rightside = rightpart;
            }
        }
    }
    return result;
}

/**
 *  Extracts the two names from the ALSA/JACK client/port name format.
 *  For a jackpatch patch line, it assumes we're using the left or
 *  right client:port as extracted by extract_patch_line().
 *
 *      [0] 128:0 client name:port name
 *
 *  When a2jmidid is running:
 *
 *      [2] 0:2 a2j:Midi Through [14] (playback): Midi Through Port-0
 *
 *  with "a2j" as the client name and the rest, including the second colon, as
 *  the port name. On line 131 or so is the snprintf() format string when
 *  the -u option is used:
 *
 *    "%s (%s): %s"
 *      ^   ^    ^
 *      |   |    |
 *      |   |     --------- port name from ALSA
 *      |    -------------- "capture" or "playback"
 *       ------------------ client name from ALSA
 *
 *  Without the -u option, unique numbers are added to the client and port
 *  names:
 *
 *    "%s [%d] (%s): [%d] %s"
 *
 *  Here are some concrete examples; the client and port components are
 *  underlined. Note that spaces are part of the name:
 *
 *      Left:  seq66.nPSLM:a2j:nanoKEY2 (playback): nanoKEY2 nanoKEY2 _ CTRL
 *             ----------------------------------- -------------------------
 *      Right: a2j:nanoKEY2 (playback): nanoKEY2 nanoKEY2 _ CTRL
 *             ----------------------- -------------------------
 *
 *      Left:  seq66.nPSLM:fluidsynth-midi:midi_00
 *             ----------- -----------------------
 *      Right: fluidsynth-midi:midi_00
 *             --------------- -------
 *
 *  Note that a2jmidid adds a space after the client-name's terminating
 *  colon character.
 *
 * \param fullname
 *      The full client:port specification to be split.
 *
 * \param [out] clientname
 *      The destination for the client name portion, "clientname".
 *
 * \param [out] portname
 *      The destination for the port name portion, "portname".
 *
 * \return
 *      Returns true if all items are non-empty after the process.
 */

bool
extract_client_port
(
    const std::string & fullname,
    std::string & clientname,
    std::string & portname
)
{
    bool result = ! fullname.empty();
    clientname.clear();
    portname.clear();
    if (result)
    {
        std::string::size_type colonpos = 0;
        std::string a2j { "a2j:" };
        bool is_a2j = util::contains(fullname, a2j);
        if (is_a2j)
            colonpos = fullname.find(a2j) + a2j.length();

        std::string cname;
        std::string pname;
        colonpos = fullname.find_first_of(":", colonpos);
        if (colonpos != std::string::npos)
        {
            /*
             * The client name consists of all characters up the the first
             * colon. Period. The port name consists of all characters
             * after that colon. Period.
             */

            cname = fullname.substr(0, colonpos);
            pname = fullname.substr(colonpos +1);
            result = ! cname.empty() && ! pname.empty();
        }
        else
            pname = fullname;

        clientname = cname;
        portname = pname;
    }
    return result;
}

patch_direction
process_patch
(
    const std::string & patch,
    std::string & left_client,
    std::string & left_port,
    std::string & right_client,
    std::string & right_port
)
{
    left_client.clear();
    left_port.clear();
    right_client.clear();
    right_port.clear();

    std::string leftside;
    std::string rightside;
    patch_direction result = extract_patch_line(patch, leftside, rightside);
    if (result != patch_direction::error)
    {
        std::string clientname;
        std::string portname;
        bool ok = extract_client_port(leftside, clientname, portname);
        if (ok)
        {
            left_client = clientname;
            left_port = portname;
            ok = extract_client_port(rightside, clientname, portname);
            if (ok)
            {
                right_client = clientname;
                right_port = portname;
            }
            else
                result = patch_direction::error;
        }
        else
            result = patch_direction::error;
    }
    return result;
}

#if defined USE_SSCANF_FREE_CLIENT_PORT_PARSING

/**
 *  Extracts the buss name from "bus:port".  Sometimes we don't need both
 *  parts at once.
 *
 *  However, when a2jmidid is active. the port name will have a colon in it.
 *
 * \param fullname
 *      The "bus:port" name.
 *
 * \return
 *      Returns the "bus" portion of the string.  If there is no colon, then
 *      it is assumed there is no buss name, so an empty string is returned.
 */

std::string
extract_bus_name (const std::string & fullname)
{
    std::size_t colonpos = fullname.find_first_of(":");  /* not last! */
    return (colonpos != std::string::npos) ?
        fullname.substr(0, colonpos) : std::string("");
}

/**
 *  Extracts the port name from "bus:port".  Sometimes we don't need both
 *  parts at once.
 *
 *  However, when a2jmidid is active. the port name will have a colon in it.
 *
 * \param fullname
 *      The "bus:port" name.
 *
 * \return
 *      Returns the "port" portion of the string.  If there is no colon, then
 *      it is assumed that the name is a port name, and so \a fullname is
 *      returned.
 */

std::string
extract_port_name (const std::string & fullname)
{
    std::size_t colonpos = fullname.find_first_of(":");  /* not last! */
    return (colonpos != std::string::npos) ?
        fullname.substr(colonpos + 1) : fullname ;
}

#endif      // defined USE_SSCANF_FREE_CLIENT_PORT_PARSING

}           // namespace nsm

/*
 * helpers.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

