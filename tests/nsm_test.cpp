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
 * \file          nsm_test.cpp
 *
 *      A test-file for the some of the more problematic NSM functions.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom
 * \date          2025-01-29
 * \updates       2025-03-20
 * \license       See above.
 *
 * Instructions:
 *
 *      To run this test properly, first make sure the current directory on
 *      the command-line is the project root (topmost directory). Then run
 *      the following command:  ./build/tests/nsmtest [options].
 */

#include <cstdlib>                      /* EXIT_SUCCESS, EXIT_FAILURE       */
#include <iostream>                     /* std::cout                        */
#include <string>                       /* std::string                      */

#include "nsm66.hpp"                    /* nsm66_version()                  */
#include "cfg/appinfo.hpp"              /* cfg::appinfo                     */
#include "cli/parser.hpp"               /* cli::parser, etc.                */
#include "nsm/helpers.hpp"              /* nsm::functions_to_test()         */
#include "util/filefunctions.hpp"       /* util::get_current_directory()    */
#include "util/ftswalker.hpp"           /* util::get_current_directory()    */
#include "util/msgfunctions.hpp"        /* util::error_message() etc.       */
#include "util/strfunctions.hpp"        /* the V() macro                    */

namespace       // anonymous
{

/**
 *  This enumeration allows for selection of all or one test.
 */

enum class test
{
    rand_id,                            /* nsm::generate_client_id()        */
    mkpath,                             /* nsm::mkpath()                    */
    lockfile_dir,                       /* nsm::make_xdg_runtime_lock_...() */
    lockfile_name,                      /* nsm::get_lock_file_name()        */
    triplets,                           /* nsm::parse_session_lines()       */
    daemon_dir,                         /* nsm::make_daemon_directory()     */
    session_root,                       /* nsm::make_session_root()         */
#if defined USE_PROCESS_PATCH_SSCANF
    process_patch_sscanf,               /* nsm::process_patch_sscanf()      */
#endif
    process_patch,                      /* nsm::process_patch()             */
    extract_patch_line,                 /* nsm::extract_patch_line()        */
    all
};

/*
 * The test functions themselves.
 */

/**
 *  Tests the generation of a client ID, which internally uses the
 *  cfg66 functions util::simple_hash() and util::string_asprintf().
 */

bool
run_test_rand_id ()
{
    std::string bad = nsm::generate_client_id("hello");
    bool result = bad.empty();
    if (result)
    {
        if (util::verbose())
            std::cout << "IDs: ";

        for (int i = 0; i < 5; ++i)
        {
            std::string rs = nsm::generate_client_id("n----");
            if (rs.empty())
                return false;

            if (util::verbose())
                std::cout << "'" << rs << "'     ";
        }
        if (util::verbose())
        {
            std::cout << std::endl;
            std::cout << "IDs: ";
        }
        for (int i = 0; i < 5; ++i)
        {
            std::string rs = nsm::generate_client_id("[-------]");
            if (rs.empty())
                return false;

            if (util::verbose())
                std::cout << "'" << rs << "' ";
        }
        if (util::verbose())
            std::cout << std::endl;
    }
    return result;
}

/*
 * Set up the path, which assumes we're running the test from
 * the project root/top. Note that "./build/tests" will already
 * exist after building the library.
 */

bool
run_test_mkpath ()
{

    static std::string s_path { "./build/tests/data/hello/" };
    static std::string s_path_partial { "./build/tests/data/" };
    bool result = nsm::mkpath(s_path, false);
    if (result)
    {
        result = util::file_is_directory(s_path_partial);
        if (result)
        {
            result = nsm::mkpath(s_path);
            if (result)
                result = util::file_is_directory(s_path);
        }
    }
    return result;
}

/**
 *  Gets the lock directory (e.g. "/run/user/1000/nsm"),
 *  creates it, shows it, then deletes it.
 *
 *  Another option besides nsm::make_xdg_runtime_lock_directory(xdgdir)
 *  is util::make_cdg_runtime_directory("nsm")
 */

bool
run_test_lockfile_dir ()
{
    std::string xdgdir;
    bool result = nsm::make_xdg_runtime_lock_directory(xdgdir);
    if (result)
    {
        if (util::verbose())
        {
            std::cout
                << "XDG runtime lock directory: '" << xdgdir << "'"
                << std::endl
                ;
        }
        result = util::file_is_directory(xdgdir);
        if (result)
        {
            /*
             * This does not work if "/run/user/1000/nsm" contains
             * the "d" directory.
             *
             * result = util::delete_directory(xdgdir);
             */

            result = util::fts_delete_directory(xdgdir);
            if (result)
                result = ! util::file_exists(xdgdir);
        }
    }
    return result;
}

/**
 *  Normally, the lock directory is obtained by
 *  nsm::get_xdg_runtime_lock_directory() [tested later].
 *  Here, we make up a local directory to be assembled from parts that
 *  we've just made up, that roughly correspond to the real thing.
 *  Here's the real values:
 *
 *      -   Session name:       "2025-01-26"
 *      -   Session hash:       "36116"
 *      -   Lock directory:     "/run/usr/1000/nsm"
 *      -   Session lock:       "/run/usr/1000/nsm/2025-01-2636116"
 *      -   Daemon directory:   "/run/usr/1000/nsm/d"
 *      -   Daemon file:        "/run/usr/1000/nsm/d/1995673" (PID)
 *      -   Session root:       "/home/username/.local/share/nsm"
 *      -   Full absolute path: "/home/username/.local/share/nsm/2025-01-26"
 *      -   Server URL:         "osc.udp://hostname:14143"
 *
 *  The full absolute path is used for obtaining the hash value.
 *
 *  The lock-file contains the full absolute path, the server URL, and
 *  the execution's PID, such as:
 *
 *          /home/username/.local/share/nsm/2025-01-26
 *          osc.udp://hostname:14143
 *          71322
 */

bool
run_test_lockfile_name ()
{
    static std::string s_lock_dir { "./build/tests/lock/1000/nsm" };
    static std::string s_session_name { "2025-01-26" };
    static std::string s_absolute_path
    {
        "/home/username/.local/share/nsm/2025-01-26"
    };
    static std::string s_url { "osc.udp://hostname:14143" };
    std::string lfn = nsm::get_lock_file_name
    (
        s_lock_dir,                 // const std::string & lockdirectory
        s_session_name,             // const std::string & sessionname,
        s_absolute_path             // const std::string & absolutesessionpath
    );
    bool result = ! lfn.empty();
    if (result)
    {
        /*
         * Since this files are all local to the user's HOME, we need
         * to make the lock directory first. We do not have delete it later,
         * because it is in the build directory, which is cleaned before
         * packing up the code. But we do it anyway for this test.
         * After this test the directory like this relative path example
         * should be empty: "build/tests/lock/1000/nsm/"
         */

        result = util::make_directory_path(s_lock_dir, 0771);
        if (result)
        {
            util::info_message("Lock file", lfn);
            result = nsm::write_lock_file(lfn, s_absolute_path, s_url);
            if (result)
            {
                std::string locktext = util::file_read_string(lfn);
                util::info_message("Lock contents", locktext);
                result = nsm::delete_lock_file(lfn);
            }
        }
    }
    return result;
}

/*
 * Tests session_triplet_to_string() and parse_session_lines().
 */

bool
run_test_triplets ()
{
    static std::string s_session_file { "tests/data/session.nsm" };
    nsm::session_triplets trips = nsm::parse_session_lines(s_session_file);
    bool result = ! trips.empty();
    if (result)
    {
        for (const auto & t : trips)
        {
            std::string triplet = nsm::session_triplet_to_string(t);
            result = ! triplet.empty();
            if (result)
            {
                if (util::verbose())
                    std::cout << triplet << std::endl;
            }
            else
                break;
        }
    }
    return result;
}

/**
 *  Creates (and then deletes) a fake daemon directory in the build/tests
 *  directory.
 */

bool
run_test_daemon_dir ()
{
    /*
     * This directory was created by run_test_mkpath() in an earlier test.
     */

    const std::string basedir { "build/tests/data/hello" };
    std::string daemondir;
    bool result = nsm::make_daemon_directory(basedir, daemondir);
    if (result)
    {
        std::string expected_dir = basedir + "/d";
        result = util::file_is_directory(expected_dir);
        if (result)
            result = util::delete_directory(expected_dir);  /* empty, so ok */
    }
    return result;
}

/**
 *
 */

bool
run_test_session_root ()
{
    std::string session_root_dir;
    bool result = nsm::make_session_root(session_root_dir);
    if (result)
    {
        /*
         * Nothing else to do. We don't want to delete the resulting
         * direction (either the old "NSM Sessions" one or
         * "$HOME/.local/share/nsm") because they might contain
         * actually session data.
         */
    }
    return result;
}

/**
 *  Taken from "tests/data/test.jackpatch"
 */

static const lib66::tokenization patches =
{
    "a2j:Launchpad Mini (capture): Launchpad Mini MIDI 1 "
        "|> seq66.nPSLM:a2j:Launchpad Mini (capture): Launchpad Mini MIDI 1",
    "a2j:MPK mini Play mk3 (capture): MPK mini Play mk3 MIDI 1 "
        "|> seq66.nPSLM:a2j:MPK mini Play mk3 (capture): MPK mini Play mk3 MIDI 1",
    "a2j:Midi Through (capture): Midi Through Port-0 "
        "|> seq66.nPSLM:a2j:Midi Through (capture): Midi Through Port-0",
    "a2j:Q25 (capture): Q25 MIDI 1            "
        "|> seq66.nPSLM:a2j:Q25 (capture): Q25 MIDI 1",
    "a2j:nanoKEY2 (capture): nanoKEY2 nanoKEY2 _ CTRL "
        "|> seq66.nPSLM:a2j:nanoKEY2 (capture): nanoKEY2 nanoKEY2 _ CTRL",
    "seq66.nPSLM:a2j:Launchpad Mini (playback): Launchpad Mini MIDI 1 "
        "|> a2j:Launchpad Mini (playback): Launchpad Mini MIDI 1",
    "seq66.nPSLM:a2j:MPK mini Play mk3 (playback): MPK mini Play mk3 MIDI 1 "
        "|> a2j:MPK mini Play mk3 (playback): MPK mini Play mk3 MIDI 1",
    "seq66.nPSLM:a2j:Midi Through (playback): Midi Through Port-0 "
        "|> a2j:Midi Through (playback): Midi Through Port-0",
    "seq66.nPSLM:a2j:Q25 (playback): Q25 MIDI 1 "
        "|> a2j:Q25 (playback): Q25 MIDI 1",
    "seq66.nPSLM:a2j:nanoKEY2 (playback): nanoKEY2 nanoKEY2 _ CTRL "
        "|> a2j:nanoKEY2 (playback): nanoKEY2 nanoKEY2 _ CTRL",
    "seq66.nPSLM:fluidsynth-midi:midi_00      |> fluidsynth-midi:midi_00"
};

#if defined USE_PROCESS_PATCH_SSCANF

/**
 *  Parses the list above, and displays each client:port pair as "client+port",
 *  which reveals that sscanf() parsing fails for the pairs created by
 *  a2jmidid.
 */

bool
run_test_process_patch_sscanf ()
{
    bool result = true;
    int index = 0;
    for (const auto & p : patches)
    {
        std::string left_client;
        std::string left_port;
        std::string right_client;
        std::string right_port;
        nsm::patch_direction pdresult = nsm::process_patch_sscanf
        (
            p, left_client, left_port, right_client, right_port
        );
        if (pdresult != nsm::patch_direction::error)
        {
            util::info_printf
            (
                "[%2d]\n"
                " Left: %s+%s\n"
                "Right: %s+%s\n",
                index, V(left_client), V(left_port),
                V(right_client), V(right_port)
            );
        }
        else
        {
            util::error_message("Bad patch", p);
            result = false;
            break;
        }
        ++index;
    }
    if (result && ! util::verbose())
    {
        util::status_message("This test passes, but sscanf() parsing has issues");
        util::status_message
        (
            "Run with --verbose to see the '+' client/port separators\n"
        );
    }
    return result;
}

#endif  // defined USE_PROCESS_PATCH_SSCANF

/**
 *  Parses the list above, and displays each client:port pair as "client+port",
 *  which reveals that sscanf() parsing fails for the pairs created by
 *  a2jmidid.
 */

bool
run_test_process_patch ()
{
    bool result = true;
    int index = 0;
    for (const auto & p : patches)
    {
        std::string left_client;
        std::string left_port;
        std::string right_client;
        std::string right_port;
        nsm::patch_direction pdresult = nsm::process_patch
        (
            p, left_client, left_port, right_client, right_port
        );
        if (pdresult != nsm::patch_direction::error)
        {
            util::info_printf
            (
                "[%2d]\n"
                " Left: %s+%s\n"
                "Right: %s+%s\n",
                index, V(left_client), V(left_port),
                V(right_client), V(right_port)
            );
        }
        else
        {
            util::error_message("Bad patch", p);
            result = false;
            break;
        }
        ++index;
    }
    return result;
}

bool
run_test_extract_patch_line ()
{
    bool result = true;
    int index = 0;
    for (const auto & p : patches)
    {
        std::string left_client_port;
        std::string right_client_port;
        nsm::patch_direction pdresult = nsm::extract_patch_line
        (
            p, left_client_port, right_client_port
        );
        if (pdresult != nsm::patch_direction::error)
        {
            util::info_printf
            (
                "[%2d]\n"
                " Left: '%s'\n"
                "Right: '%s'",
                index, V(left_client_port),
                V(right_client_port)
            );

            /*
             * Now extract the client name and port name.
             */

            std::string clientname;
            std::string portname;
            result = nsm::extract_client_port
            (
                left_client_port, clientname, portname
            );
            if (result)
            {
                util::info_printf
                (
                    "Left: '%s':'%s'",
                    V(clientname), V(portname)
                );
                result = nsm::extract_client_port
                (
                    right_client_port, clientname, portname
                );
                if (result)
                {
                    util::info_printf
                    (
                        "Right: '%s':'%s'",
                        V(clientname), V(portname)
                    );
                }
            }
        }
        else
        {
            util::error_message("Bad patch", p);
            result = false;
            break;
        }
        ++index;
    }
    return result;
}

/**
 *  Checks to see if a test can be run.
 */

bool
runtest (test desired, test actual)
{
    return desired == test::all || desired == actual;
}

using test_func = bool (*) ();

using test_info = struct
{
    std::string test_name;
    test test_number;
    test_func test_function;
};

using test_list = std::vector<test_info>;

/**
 *  Test list accessor function.
 */

const test_list &
all_tests ()
{
    static test_list s_tests
    {
        { "rand-id",        test::rand_id,          run_test_rand_id        },
        { "mkpath",         test::mkpath,           run_test_mkpath         },
        { "lockfile-dir",   test::lockfile_dir,     run_test_lockfile_dir   },
        { "lockfile-name",  test::lockfile_name,    run_test_lockfile_name  },
        { "triplets",       test::triplets,         run_test_triplets       },
        { "daemon-dir",     test::daemon_dir,       run_test_daemon_dir     },
        { "session-root",   test::session_root,     run_test_session_root   },
#if defined USE_PROCESS_PATCH_SSCANF
        {
            "process-patch-sscanf",
            test::process_patch_sscanf,
            run_test_process_patch_sscanf
        },
#endif
        {
            "process-patch",
            test::process_patch,
            run_test_process_patch
        },
        {
            "extract-patch-line",
            test::extract_patch_line,
            run_test_extract_patch_line
        },
    };
    return s_tests;
}

bool
run_all_tests (test desiredtest)
{
    bool result = true;
    for (auto t : all_tests())              /* note the accessor    */
    {
        if (runtest(desiredtest, t.test_number))
        {
            std::cout << "\n----TEST " << t.test_name << "\n\n" ;
            result = t.test_function();

            std::string outcome = result ? "PASSED" : "FAILED" ;
            std::cout << t.test_name << ": " << outcome << std::endl;
            if (! result)
                break;
        }
    }
    return result;
}

/*
 * Local options. Specified here for use with --help.
 */

cfg::options::container s_test_options
{
    /*
     *  Name
     *      Code,  Kind, Enabled,
     *      Default, Value, FromCli, Dirty,
     *      Description, Built-in
     */
    {
        {
            "rand-id",
            {
                'r', cfg::options::kind::boolean, cfg::options::enabled,
                "false", "", false, false,
                "If specified, the test of randomization is run by itself.",
                false
            }
        },
        {
            "mkpath",
            {
                'm', cfg::options::kind::boolean, cfg::options::enabled,
                "false", "", false, false,
                "If specified, the test of mkpath() is run by itself.",
                false
            }
        },
        {
            "lockfile-dir",
            {
                cfg::options::code_null,
                cfg::options::kind::boolean, cfg::options::enabled,
                "false", "", false, false,
                "If specified, the test of "
                "make_xdg_runtime_lock_directory() runs alone.",
                false
            }
        },
        {
            "lockfile-name",
            {
                'n', cfg::options::kind::boolean, cfg::options::enabled,
                "false", "", false, false,
                "If specified, the test of get_lock_file_name() is run by itself.",
                false
            }
        },
        {
            "triplets",
            {
                't', cfg::options::kind::boolean, cfg::options::enabled,
                "false", "", false, false,
                "If specified, the test of parse_session_lines() is run by itself.",
                false
            }
        },
        {
            "daemon-dir",
            {
                'd', cfg::options::kind::boolean, cfg::options::enabled,
                "false", "", false, false,
                "If specified, the test of make_daemon_directory() is run by itself.",
                false
            }
        },
        {
            "session-root",
            {
                's', cfg::options::kind::boolean, cfg::options::enabled,
                "false", "", false, false,
                "If specified, the test of make_session_root is run by itself.",
                false
            }
        },
        {
            "process-patch",
            {
                'p', cfg::options::kind::boolean, cfg::options::enabled,
                "false", "", false, false,
                "If specified, the test of process_patch() is run by itself.",
                false
            }
        },
        {
            "extract-patch-line",
            {
                'e', cfg::options::kind::boolean, cfg::options::enabled,
                "false", "", false, false,
                "If specified, the test of extract_patch_line is run by itself.",
                false
            }
        }
    }
};

/*
 * Explanation text.
 */

const std::string s_help_intro
{
    "This test program (in progress) illustrates/tests parts of the nsm66\n"
    "library.  The options available are as follows:\n\n"
};

const std::string s_description
{
    "We want to first test the various data formats used by nsmd and\n"
    "jackpatch.\n"
};

}               // namespace anonymous

/*
 * main() routine
 */

int
main (int argc, char * argv [])
{
    int rcode = EXIT_FAILURE;
    cfg::set_client_name("nsm66");                  /* for error_message()  */
    cfg::set_app_version("0.1.0");
    cli::parser clip(s_test_options, "", "");
    bool success = clip.parse(argc, argv);
    if (success)
    {
        bool runtests = true;
        rcode = EXIT_SUCCESS;

        /*
         *  The application can substitute its own code for the common
         *  options, which are always present.
         */

        if (clip.help_request())
        {
            std::cout << s_help_intro;              /* << std::endl;        */
            runtests = false;
        }
        if (clip.show_information_only())           /* clip.help_request()  */
        {
            if (clip.description_request())
            {
                std::cout << s_description;
            }
            runtests = false;
        }
        if (clip.version_request())
        {
            std::cout << nsm66_version() << std::endl;
        }
        if (clip.verbose_request())
        {
            // nothing yet
        }
        if (clip.inspect_request())
        {
            util::error_message("--investigate unsupported in this program");
            success = false;
        }
        if (clip.investigate_request())
        {
            util::error_message("--investigate unsupported in this program");
            success = false;
        }
        if (runtests)
        {
            std::string cwd = util::get_current_directory();
            test test_desired = test::all;
            cfg::options & opts = clip.option_set();
            if (opts.boolean_value("rand-id"))
                test_desired = test::rand_id;

            if (opts.boolean_value("mkpath"))
                test_desired = test::mkpath;

            if (opts.boolean_value("lockfile-dir"))
                test_desired = test::lockfile_dir;

            if (opts.boolean_value("lockfile-name"))
                test_desired = test::lockfile_name;

            if (opts.boolean_value("triplets"))
                test_desired = test::triplets;

            if (opts.boolean_value("daemon-dir"))
                test_desired = test::daemon_dir;

            if (opts.boolean_value("session-root"))
                test_desired = test::session_root;

            if (opts.boolean_value("process-patch"))
                test_desired = test::process_patch;

            if (opts.boolean_value("extract-patch-line"))
                test_desired = test::mkpath;

            std::cout << cwd << std::endl;
            success = run_all_tests(test_desired);
        }
    }
    else
        std::cerr << "Command-line parsing failed" << std::endl;

    if (success)
        std::cout << "NSM66 library test succeeded" << std::endl;
    else
        std::cerr << "NSM66 library test failed" << std::endl;

    return rcode;
}

/*
 * nsm_test.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

