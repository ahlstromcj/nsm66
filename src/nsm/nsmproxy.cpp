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
 * \file          nsmproxy.cpp
 *
 *    This module refactors the nsmproxy class to replace C code with
 *    C++ code.
 *
 * \library       nsmproxy class
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-03-06
 * \updates       2025-03-08
 * \version       $Revision$
 * \license       GNU GPL v3 or above
 *
 *   To do.
 */

#include <cerrno>                       /* #include <errno.h>               */
#include <csignal>                      /* std::signal() and <signal.h>     */
#include <cstring>                      /* std::strerror()                  */
#include <cstdlib>                      /* std::getenv(), std::rand()       */

#include "cpp_types.hpp"                /* lib66::tokenization alias        */
#include "nsm/nsmproxy.hpp"             /* nsm66: nsm::nsmproxy class       */
#include "osc/lowrapper.hpp"            /* nsm66: LO_TT_IMMEDIATE_2 etc.    */
#include "osc/messages.hpp"             /* nsm66: osc::tag enumeration      */
#include "util/msgfunctions.hpp"        /* cfg66: util::info_message() ...  */
#include "util/filefunctions.hpp"       /* cfg66: util::file_write_string() */
#include "util/strfunctions.hpp"        /* cfg66: util::string_asprintf()   */

namespace nsm
{

/*
 *  To do.
 */

nsmproxy::nsmproxy () :
    m_lo_server         (nullptr),
    m_nsm_address       (nullptr),
//  m_gui_address       (nullptr),
    m_label             (),
    m_executable        (),
    m_arguments         (),
    m_config_file       (),
    m_client_error      (),
    m_save_signal       (0),
    m_stop_signal       (SIGTERM),
    m_pid               (0),
    m_nsm_client_id     (),
    m_nsm_display_name  ()
{
    // no code
}

nsmproxy::nsmproxy
(
    const std::string & client_id,
    const std::string & display_name
) :
    m_lo_server         (nullptr),
    m_nsm_address       (nullptr),
//  m_gui_address       (nullptr),
    m_label             (),
    m_executable        (),
    m_arguments         (),
    m_config_file       (),
    m_client_error      (),
    m_save_signal       (0),
    m_stop_signal       (SIGTERM),
    m_pid               (0),
    m_nsm_client_id     (client_id),
    m_nsm_display_name  (display_name)
{
    // no code
}

/*
 *  Proxied process died unexpectedly.
 */

void
nsmproxy::handle_client_death (int status)
{
    util::warn_message("proxied process died unexpectedly... not dying");
    m_client_error = util::string_asprintf
    (
        "The proxied process terminated abnormally during invocation; "
        "exit status: %i", status
    );

    /*
     * TODO: show_gui();
     */

    m_pid = 0;
}

void
nsmproxy::kill ()
{
    if (m_pid != 0)
        ::kill(m_pid, m_stop_signal);
}

bool
nsmproxy::start
(
    const std::string & executable,
    const std::string & arguments,
    const std::string & config_file
)
{
    m_executable = executable;
    m_arguments = arguments;
    m_config_file = config_file;
    return start();
}

bool
nsmproxy::start ()
{
    //    TODO ? dump(g_project_file);

    if (m_pid != 0)                 /* already running */
        return true;

    if (m_executable.empty())
    {
        util::warn_message("Executable is null");
        return false;
    }

    int pid;
    if (! (pid = fork()))
    {
        std::string cmd;
        if (m_arguments.empty())
        {
            cmd = util::string_asprintf
            (
                "exec %s > error.log 2>&1", V(m_executable)
            );
        }
        else
        {
            cmd = util::string_asprintf
            (
                "exec %s %s > error.log 2>&1",
                V(m_executable), V(m_arguments)
            );
        }

        char * args [] =
        {
            strdup("/bin/sh"), strdup("-c"), STR(cmd), NULL
        };
        setenv("ENV_NSM_CLIENT_ID", CSTR(m_nsm_client_id), 1);
        setenv("ENV_NSM_SESSION_NAME", CSTR(m_nsm_display_name), 1);
        if (! m_config_file.empty())
            setenv( "ENV_NSM_CONFIG_FILE", CSTR(m_config_file), 1);

        unsetenv("NSM_URL");
        util::info_message("Launching ", m_executable);
        if (execvp("/bin/sh", args) == (-1))
        {
            util::warn_printf("Error starting process: %s", strerror(errno));
            exit(1);
        }
    }
    m_pid = pid;
    return m_pid > 0;
}

void
nsmproxy::label (const std::string & s)
{
    m_label = s;
    (void) send(m_nsm_address, "/nsm/client/label", m_label);
#if 0
    lo_send_from            // g_nsm_lo_address, g_osc_server,
    (
        m_nsm_address, m_lo_server
        LO_TT_IMMEDIATE_2, "/nsm/client/label", "s", CSTR(m_label)
    );
#endif
}

void
nsmproxy::save ()
{
    util::info_message("Sending process save signal");
    if (m_pid != 0)
        ::kill(m_pid, m_save_signal);
}

/*
 *  Dump current config to file. This happens quite often.
 *  Currently, the format of the file is simple -- the
 *  name of the configuration variable, a newline, then
 *  a tab and the value on the next line:
 *
 *      executable
 *          my_executable_name
 *      arguments
 *          my_arguments
 *      config file
 *          my_config_file
 *      save signal
 *          <integer>
 *      stop signal
 *          <integer>
 *      label
 *          my label
 *
 * We will eventually add comment-lines at the top and bottom of
 * this file.
 */

bool
nsmproxy::dump (const std::string & path)
{
    std::string fname = util::string_asprintf
    (
        "%s/%s", V(path), NSM_CONFIG_FILE_NAME
    );
    std::string fdata;
    if (! m_executable.empty())
    {
        std::string data = util::string_asprintf
        (
            "executable\n\t%s\n", V(m_executable)
        );
        fdata += data;
    }
    if (! m_arguments.empty())
    {
        std::string data = util::string_asprintf
        (
            "arguments\n\t%s\n", V(m_arguments)
        );
        fdata += data;
    }
    if (! m_config_file.empty())
    {
        std::string data = util::string_asprintf
        (
            "config file\n\t%s\n", V(m_config_file)
        );
        fdata += data;
    }
    std::string sigdata = util::string_asprintf
    (
        "save signal\n\t%i\n" "stop signal\n\t%i\n",
        m_save_signal, m_stop_signal
    );
    if (! m_label.empty())
    {
        std::string data = util::string_asprintf
        (
            "label\n\t%s\n", V(m_label)
        );
        fdata += data;
    }

    bool result = util::file_write_string(fname, fdata);
    if (! result)
    {
        util::error_message
        (
            "Error opening file for saving", strerror(errno)
        );
    }
    return result;
}

/**
 *  Rather than monkey with fscanf(), we read in all the non-commented
 *  lines and process them. Note that util::file_read_lines() skips
 *  comments and empty lines, and here is set to trim white space at both
 *  ends.
 */

bool
nsmproxy::restore (const std::string & path)
{
    lib66::tokenization lines;
    bool result = util::file_read_lines(path, lines, true); /* trim white   */
    if (result)
    {
        bool odd = true;
        std::string varname;
        util::info_message("Loading config file", path);
        for (auto & line : lines)
        {
            if (odd)
            {
                varname = line;
            }
            else
            {
                if (varname == "executable")
                    m_executable = line;
                else if (varname == "arguments")
                    m_arguments = line;
                else if (varname == "config file")
                    m_config_file = line;
                else if (varname == "save signal")
                    m_save_signal = util::string_to_int(line);
                else if (varname == "stop signal")
                    m_stop_signal = util::string_to_int(line);
                else if (varname == "label")
                    m_label = line;
            }
            odd = ! odd;
        }
        start();
    }
    return result;
}

/*
 * Similar to the send() functions in the endpoint class.
 */

int
nsmproxy::send
(
    lo_address to,
    const std::string & oscpath,
    int signalvalue
)
{
    return lo_send_from
    (
        to, m_lo_server, LO_TT_IMMEDIATE_2, CSTR(oscpath), "i", signalvalue
    );
}

int
nsmproxy::send
(
    lo_address to,
    const std::string & oscpath,
    const std::string & stringvalue
)
{
    const char * value = stringvalue.empty() ? "" : CSTR(stringvalue) ;
    return lo_send_from
    (
        to, m_lo_server, LO_TT_IMMEDIATE_2, CSTR(oscpath), "s", value
    );
}

/*
 *  Each send triggers one osc_update in the Proxy-GUI.
 */

void
nsmproxy::update (lo_address to)
{
    util::info_message("Sending update");
    send(to, "/nsm/proxy/save_signal", m_save_signal);
    send(to, "/nsm/proxy/label", m_label);
    send(to, "/nsm/proxy/executable", m_executable);
    send(to, "/nsm/proxy/arguments", m_arguments);
    send(to, "/nsm/proxy/config_file", m_config_file);
    send(to, "/nsm/proxy/stop_signal", m_stop_signal);
    send(to, "/nsm/proxy/client_error", m_client_error);
}

}           // namespace nsm

/*
 * nsm-proxy66.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

