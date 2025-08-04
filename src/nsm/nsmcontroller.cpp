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
 * \file          nsmcontroller.cpp
 *
 *    This module refactors the nsmd application to replace C code with
 *    C++ code.
 *
 * \library       nsmctl application
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-03-21
 * \updates       2025-04-24
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *      This is more like a server messager than a client messager; see
 *      nsmctlclient for the latter.
 *
 * Tags implemented herein:
 *
 *      -   srvabort
 *      -   srvclose
 *      -   srvsave
 *      -   srvopen
 *      -   srvduplicate
 *      -   srvquit
 *      -   srvlist
 *      -   srvnew
 *      -   srvadd
 */

#include <cerrno>                       /* #include <errno.h>               */
#include <string.h>                     /* strcmp(3)                        */
#include <time.h>                       /* time(2)                          */
#include <unistd.h>                     /* getpid(2)                        */

#include "nsm/nsmcontroller.hpp"        /* nsm66: nsm::nsmcontroller        */
#include "osc/endpoint.hpp"             /* nsm66: osc::endpoint             */
#include "util/msgfunctions.hpp"        /* cfg66: util::xxxx_message()....  */
#include "util/strfunctions.hpp"        /* cfg66: util::string_asprintf()...*/

namespace nsm
{

nsmcontroller::nsmcontroller
(
    daemon_list & alldaemons,
    const std::string & appname,
    const std::string & exename,
    const std::string & capabilities,
    const std::string & apiversion,
    int pingct
) :
    m_osc_server            (),
    m_daemon_list           (alldaemons),
    m_session_list          (),
    m_clients_pack          (),
    m_last_ping_response    (0),
    m_ping_timeout          (10),
    m_ping_count            (pingct),
    m_app_name              (appname),
    m_exe_name              (exename),
    m_capabilities          (capabilities),
    m_api_version           (apiversion),
    m_session_name          ()
{
    // deactivate();
}

void
nsmcontroller::log_status (const std::string & s, bool iserror)
{
    time_t now = time(NULL);
    struct tm * tm = localtime(&now);
    std::string ts = util::string_asprintf
    (
        "%02i:%02i:%02i ", tm->tm_hour, tm->tm_min, tm->tm_sec
    );
    ts += s;
    if (iserror)
        util::error_message(ts);
    else
        util::info_message(ts);
}

/**
 *  A static function to handle sending commands to all attached daemons.
 *  It calls the non-static member function of the same name. It was
 *  needed in the original nsm-legacy-gui [under the name cb_handle()]
 *  as a callback for FLTK controls, but we can probably igore it here.
 */

void
nsmcontroller::send_server_message (void * v, osc::tag msg)
{
    static_cast<nsmcontroller *>(v)->send_server_message(msg);
}

/**
 *      "Are you sure you want to close this session? Unsaved changes
 *      will be lost.", "Close anyway", "Cancel", NULL ) )
 *
 *      Do we need parameters like name?
 */

bool
nsmcontroller::send_server_message
(
    osc::tag msg,
    const std::string & subjectname                 /* optional name        */
)
{
    std::string msgstr;
    std::string pattern;
    bool result = osc::tag_lookup(msg, msgstr, pattern);
    if (result)
    {
        result = false;
        if (msg == osc::tag::srvabort)              /* aborts a session     */
        {
            result = true;
            util::info_message("Sending abort");    /* no user query here   */
            for (const auto & d : m_daemon_list)
                m_osc_server->send(d.addr(), msgstr);
        }
        if (msg == osc::tag::srvclose)              /* closes a session     */
        {
            result = true;
            util::info_message("Sending close");
            for (const auto & d : m_daemon_list)
                m_osc_server->send(d.addr(), msgstr);
        }
        else if (msg == osc::tag::srvsave)          /* saves a session      */
        {
            result = true;
            util::info_message("Sending save");
            for (const auto & d : m_daemon_list)
                m_osc_server->send(d.addr(), msgstr);
        }
        else if (msg == osc::tag::srvopen)          /* open a session       */
        {
            if (! subjectname.empty())
            {
                result = true;
                util::info_message("Sending open for", subjectname);
                for (const auto & d : m_daemon_list)
                    m_osc_server->send(d.addr(), msgstr, subjectname);
            }
        }
        else if (msg == osc::tag::srvduplicate)     /* new dupe session     */
        {
            if (! subjectname.empty())
            {
                result = true;
                util::info_message("Sending duplicate for", subjectname);
                for (const auto & d : m_daemon_list)
                    m_osc_server->send(d.addr(), msgstr, subjectname);
            }
        }
        else if (msg == osc::tag::srvquit)
        {
            /*
             * Currently, nsmcontroller::quit() is called directly.
             */

            result = true;
        }
        else if (msg == osc::tag::srvlist)          /* list of sessions     */
        {
            result = true;
            util::info_message("Refreshing session list");
            for (const auto & d : m_daemon_list)
                m_osc_server->send(d.addr(), msgstr);
        }
        else if (msg == osc::tag::srvnew)           /* create a new session */
        {
            result = true;
            if (! subjectname.empty())
            {
                util::info_message("Sending new for", subjectname);
                for (const auto & d : m_daemon_list)
                    m_osc_server->send(d.addr(), msgstr, subjectname);
            }
        }
        else if (msg == osc::tag::srvadd)           /* add an executable    */
        {
            if (! m_daemon_list.empty())
            {
                /*
                 * Unlike nsm-legacy-gui, we can't (yet) select a specific
                 * NSM server, so we just pick the first one. See the
                 * original code; it's slightly puzzling.
                 */

                const auto & d = m_daemon_list[0];
                result = true;
                lo_address nsmaddr = lo_address_new_from_url(CSTR(d.url()));
                m_osc_server->send(nsmaddr, msgstr, subjectname);
            }
        }
    }
    return result;
}

/**
 *  Redirects a command to the appropriate nsmctlclient.
 */

bool
nsmcontroller::send_client_message
(
    osc::tag msg,
    const std::string & clientname
)
{
    nsmctlclient * c = client_by_name(clientname);
    bool result = not_nullptr(c);
    if (result)
        result = c->send_client_message(msg);
    else
        util::error_message("Client not found", clientname);

    return result;
}

/**
 *  The original clients_pack was an FLTK container (disclaimer, we know
 *  nothing about FLTK). Here, we have a map of IDs and client names.
 *  The client IDs are of the form "nXYZT".
 */

nsmctlclient *
nsmcontroller::client_by_id (const std::string & id) const
{
    nsmctlclient * result = nullptr;
#if defined PLATFORM_CPP_17
    if (auto c = m_clients_pack.find(id); c != m_clients_pack.end())
        result = c.second;
#else
    auto c = m_clients_pack.find(id);
    if (c != m_clients_pack.end())
        result = c->second;
#endif
    return result;
}

/**
 *  Brute-force lookup. It takes advantage of the fact that
 *  the nsmctlclient has a a name memeber.
 */

nsmctlclient *
nsmcontroller::client_by_name (const std::string & name) const
{
    nsmctlclient * result = nullptr;
    for (auto c : m_clients_pack)
    {
        if (c.second->client_name() == name)
        {
            result = c.second;
            break;
        }
    }
    return result;
}

void
nsmcontroller::client_stopped (const std::string & id)
{
    nsmctlclient * c = client_by_id(id);
    if (not_nullptr(c))
        c->stopped(true);
}

/**
 *  clients_pack was an Fl_Pack, but here we need a list of
 *  clients.
 */

void
nsmcontroller::client_quit (const std::string & id)
{
    auto cit = m_clients_pack.find(id);
    if (cit != m_clients_pack.end())
    {
        util::info_message(cit->second->info("Erased"));
        delete cit->second;
        m_clients_pack.erase(cit);
    }
}

/**
 *  Looks for the client ID. If found, the client name is set.
 *  Otherwise, a new client is created.
 */

bool
nsmcontroller::client_new
(
    const std::string & client_id,
    const std::string & client_name
)
{
    bool result = false;
    nsmctlclient * c = client_by_id(client_id);
    if (not_nullptr(c))
    {
        c->name(client_name);
        result = true;
    }
    else
    {
        c = new (std::nothrow) nsmctlclient
        (
            m_osc_server.get(), m_daemon_list,
            client_id, "", client_name
        );
        if (not_nullptr(c))
        {
            auto p = std::make_pair(client_id, c);
            auto r = m_clients_pack.insert(p);
            if (r.second)
            {
                util::info_printf
                (
                    "New client: ID %s, name %s",
                    V(client_id), V(client_name)
                );
                result = true;
            }
            else
            {
                util::warn_printf
                (
                    "Could not insert client: ID %s, name %s",
                    V(client_id), V(client_name)
                );
            }
        }
        else
        {
            util::error_printf
            (
                "Could not create client: ID %s, name %s",
                V(client_id), V(client_name)
            );
        }
    }
    return result;
}

void
nsmcontroller::client_pending_command
(
    nsmctlclient * c,
    const std::string & command
)
{
    if (not_nullptr(c))
    {
        if (command == "removed")
            client_quit(c->client_id());
        else
            c->pending_command(command);
    }
}

void
nsmcontroller::osc_wait (int timeout)
{
    m_osc_server->wait(timeout);
}

bool
nsmcontroller::osc_active () const
{
    if (bool(m_osc_server))
        return m_osc_server->active();
    else
        return false;
}

bool
nsmcontroller::deactivate ()
{
    bool result = bool(m_osc_server);
    if (result)
        m_osc_server->active(false);

    return result;
}

bool
nsmcontroller::ping ()
{
    bool result = ! m_daemon_list.empty();
    if (result)
    {
        m_last_ping_response = time(NULL);
        for (int i = 0; i < m_ping_count; ++i)
        {
            for (const auto & d : m_daemon_list)
                m_osc_server->send(d.addr(), "/osc/ping"); /* osc::tag::ping */

            if (m_last_ping_response > 0)
            {
                if ((time(NULL) - m_last_ping_response) > m_ping_timeout)
                {
                    log_status("Server not responding...", true);  /* error */
                    result = false;
                    break;
                }
                else
                    log_status("Server responds");
            }
            osc_wait(1000);
        }
    }
    return result;
}

/**
 *  See add_method in nsm66d. This is similar. Note that the userdata
 *  parameter is always NULL here, and the argument-description is
 *  stored in a osc::method object created in endpoint::add_method().
 *
 *  We would like to generalize it somehow.
 */

void
nsmcontroller::add_method
(
    osc::tag t,
    osc::method_handler f,
    const std::string & argument_description
)
{
    std::string msg, pattern;
    if (osc::tag_lookup(t, msg, pattern))
    {
        (void) m_osc_server->add_method     /* OSC server is osc::endpoint  */
        (
            msg, pattern, f, NULL, argument_description
        );
    }
}

/**
 *  In endpoint, we already add the oscreply and error tags, plus. generic,
 *  sigconnect, sigcreated, sigdisconnect, sighello, siglist, sigremoved,
 *  sigrenamed, and srvreply.
 *
 *  These are the original messages.
 *
 *  "/error",                           "sis",  "msg"   (done in lowrapper)
 *  "/reply",                           "ss",   "msg"   (done in lowrapper)
 *  "/reply",                           "s",    ""
 *  "/nsm/server/broadcast",            NULL,   "msg"
 *  "/nsm/gui/server_announce",         "s",    "msg"
 *  "/nsm/gui/server/message",          "s",    "msg"
 *  "/nsm/gui/gui_announce",            "s",    "msg"
 *  "/nsm/gui/session/session",         "s",    "path,display_name"
 *  "/nsm/gui/session/name",            "ss",   "path,display_name"
 *  "/nsm/gui/client/new",              "ss",   "path,display_name"
 *  "/nsm/gui/client/status",           "ss",   "path,display_name"
 *  "/nsm/gui/client/switch",           "ss",   "path,display_name"
 *  "/nsm/gui/client/progress",         "sf",   "path,display_name"
 *  "/nsm/gui/client/dirty",            "si",   "path,display_name"
 *  "/nsm/gui/client/has_optional_gui", "s",    "path,display_name"
 *  "/nsm/gui/client/gui_visible",      "si",   "path,display_name"
 *  "/nsm/gui/client/label",            "ss",   "path,display_name"
 *
 *  endpoint::init() adds the /error and /reply methods plus a set of
 *  /signal/xxxx methods, and a /reply with a NULL type-specification
 *  and a generic NULL+NULL method.
 *
 *  Here we add support for "/nsm/gui/session/root", which is not
 *  in the original NSM controller.
 *
 * \param portname
 *      This parameter defaults to "". Otherwise, it can be set (for
 *      example) to the looked-up address of a server port, such as
 *      "osc.udp://mlsleno:11086/".
 *
 * \return
 *      Returns true if the OSC server could be initialized.
 */

bool
nsmcontroller::init_osc (const std::string & portname)
{
    m_osc_server.reset(new (std::nothrow) osc::endpoint());
    bool result = bool(m_osc_server);
    if (result)
        result = m_osc_server->init(LO_UDP, portname, true);

    if (result)
    {
        const std::string msg { "msg" };
        const std::string pd { "path,display_name" };
        m_osc_server->owner(this);

        add_method(osc::tag::error,             osc_handler, msg);
        add_method(osc::tag::reply,             osc_handler, msg);
        add_method(osc::tag::replyex,           osc_handler, msg);
        add_method(osc::tag::srvreply,          osc_handler, msg);
        add_method(osc::tag::srvbroadcast,      osc_broadcast_handler, msg);
        add_method(osc::tag::guisrvannounce,    osc_handler, msg);
        add_method(osc::tag::srvmessage,        osc_handler, msg);
        add_method(osc::tag::gui_announce,      osc_handler, msg);  /* "s"  */
        add_method(osc::tag::guisession,        osc_handler, pd);
        add_method(osc::tag::guisessionname,    osc_handler, pd);
        add_method(osc::tag::guinew,            osc_handler, pd);
        add_method(osc::tag::guistatus,         osc_handler, pd);
        add_method(osc::tag::guiswitch,         osc_handler, pd);
        add_method(osc::tag::guiprogress,       osc_handler, pd);
        add_method(osc::tag::guidirty,          osc_handler, pd);
        add_method(osc::tag::guioption,         osc_handler, pd);
        add_method(osc::tag::guivisible,        osc_handler, pd);
        add_method(osc::tag::guilabel,          osc_handler, pd);
        add_method(osc::tag::sessionroot,       osc_handler, pd);
        m_osc_server->start();
    }
    else
    {
        /*
         * should we delete m_osc_server and null that pointer here????
         */
    }
    return result;
}

/**
 *  Commented code:
 *
 *      Daemon * d = new Daemon;
 *      d->url = nsm_url;
 *      d->is_child = true;
 *      m_daemon_list.push_back(d);
 *
 *  Send the osc::tag::guiannounce message, which uses "" as a type
 *  specification.
 */

void
nsmcontroller::announce (const std::string & nsmurl, bool legacy)
{
    lo_address nsmaddr = lo_address_new_from_url(CSTR(nsmurl));
    if (m_app_name.empty() || legacy)
    {
        m_osc_server->send(nsmaddr, "/nsm/gui/gui_announce");
    }
    else
    {
        int major, minor, patch;
        bool ok = util::extract_api_numbers
        (
            m_api_version, major, minor, patch
        );
        if (ok)
        {
            int pid = int(getpid());
            m_osc_server->send
            (
                nsmaddr, "/nsm/gui/gui_announce",
                CSTR(m_app_name), CSTR(m_capabilities), CSTR(m_exe_name),
                major, minor, pid
            );
        }
    }
}

/**
 *  This announce() is meant for the daemons that have been registered.
 */

void
nsmcontroller::announce ()
{
    for (const auto & d : m_daemon_list)
        announce(d.url());
}

/**
 *  Returns true if there are no daemons left.
 */

bool
nsmcontroller::child_check () const
{
    int children = 0;
    for (const auto & d : m_daemon_list)
    {
        if (d.is_child())
            ++children;
    }
    if (children > 0)
    {
        if (! session_name().empty())
            util::warn_message("You should close the session before quitting");
    }
    return children == 0;
}

void
nsmcontroller::quit ()
{
    if (child_check())
    {
        std::string msgstr;             /* "/nsm/server/quit" message       */
        std::string pattern;            /* pattern will be empty            */
        bool ok = osc::tag_lookup(osc::tag::srvquit, msgstr, pattern);
        if (ok)
        {
            util::info_message("Telling server(s) to quit");
            for (const auto & d : m_daemon_list)
                m_osc_server->send(d.addr(), CSTR(msgstr));
        }
    }
}

/**
 *  Static broadcast handler.
 */

int
nsmcontroller::osc_broadcast_handler
(
    const char * path,
    const char * types,
    lo_arg ** argv, int argc,               /* note argv not used here          */
    lo_message msg, void * userdata
)
{
    osc::osc_msg_summary
    (
        "nsmcontroller::osc_broadcast_handler",
        path, types, argv, argc, userdata
    );
    if (argc > 0)                       /* need at least one argument...    */
        return 0;

    nsmcontroller * ctrler = nullptr;
    osc::endpoint * ept = static_cast<osc::endpoint *>(userdata);
    if (not_nullptr(ept))
        ctrler = static_cast<nsmcontroller *>(ept->owner());

    if (is_nullptr_2(ctrler, ept))
    {
        util::error_message("Bad user data pointer");
        return (-1);
    }
    util::info_message("Relaying OSC broadcast", path);
    for (const auto & d : ctrler->m_daemon_list)
    {
        char * u1 = lo_address_get_url(d.addr());
        if (not_nullptr(u1))
        {
            char * u2 = lo_address_get_url(lo_message_get_source(msg));
            if (not_nullptr(u2))
            {
                if (strcmp(u1, u2) != 0)
                    ept->send(d.addr(), path, msg);

                free(u2);
            }
            free(u1);
        }
    }
    return 0;
}

/**
 *  Static general handler. In nsm-legacy-gui, the messages and call sequence is
 *  something like this:
 *
 *       1. "/nsm/gui/server_announce" + "s"    [ osc::tag::guisrvannounce ]
 *          a.  ctrler->activate()
 *          b.  Push a new daemon
 *          c.  Send "/nsm/server/list".
 *       2. "/nsm/gui/session/name" + "ss"      [ osc::tag::guisessionname ]
 *       3. "/nsm/gui/server/message" " "s"     [ osc::tag::srvmessage ]
 *       4. "/reply" + "ss"                     [ osc::tag::reply ]
 *          argv[0]->s is the session name; add it to the list until
 *          an empty session name comes in
 *       5. "/reply" + "s"                      [osc::tag::srvreply]
 *          argv[0]->s = "/osc/ping", a ping response. Pings are
 *          done continuously.
 */

int
nsmcontroller::osc_handler
(
    const char * path,
    const char * types,
    lo_arg ** argv, int argc,
    lo_message msg, void * userdata
)
{
    nsmcontroller * ctrler = nullptr;
    osc::endpoint * ept = static_cast<osc::endpoint *>(userdata);
    osc::osc_msg_summary
    (
        "nsmcontroller::osc_handler",
        path, types, argv, argc, userdata
    );
    if (not_nullptr(ept))
    {
        ctrler = static_cast<nsmcontroller *>(ept->owner());
    }
    if (is_nullptr_2(ctrler, ept))
    {
        util::error_message("Bad user data pointer");
        return osc::osc_msg_unhandled();
    }

    std::string msgpath { path };
    std::string msgtypes { types };
    std::string s { argc > 0 ? osc::string_from_lo_arg(argv[0]) : "" };
    std::string s1 { argc > 1 ? osc::string_from_lo_arg(argv[1]) : "" };
    osc::tag msgtag = osc::tag_reverse_lookup(msgpath, msgtypes);
    if (msgtag == osc::tag::srvmessage)
    {
        ctrler->log_status(s);
    }
    else if (msgtag == osc::tag::sessionroot)
    {
        /*
         * This section is an addition to the original NSM controller code.
         */

        // TODO
    }
    else if (msgtag == osc::tag::guisession)
    {
        ctrler->add_session_to_list(s);
    }
    else if (msgtag == osc::tag::guiannounce)
    {
        /*
         * A pre-existing server is replying to our GUI announce message.
         * In the original, NSM_Controller is derived from Fl_Group,
         * which provides the activate() function:
         *
         *      ctrler->activate()
         */

        ept->active(true);

        lo_address nsm_addr = lo_message_get_source(msg);
        ept->send(nsm_addr, "/nsm/server/list");   // osc::tag::srvlist
    }
    else if (msgtag == osc::tag::guisrvannounce)
    {
        /*
         * It must be a server we launched. Similar note about activate()
         * as above. Note the path is "/nsm/gui/server_announce".
         */

        util::status_message("Controller recv'd", msgpath);
        ept->active(true);

        const char * url = lo_address_get_url(lo_message_get_source(msg));
        lo_address addr = lo_address_new_from_url(url);
        daemon d(url, addr, true);
        ctrler->m_daemon_list.push_back(d);
        ept->send(d.addr(), "/nsm/server/list");   // osc::tag::srvlist
    }
    else if (osc::tag_reverse_lookup(msgpath, "ss") == osc::tag::guisessionname)
    {
        if (s.empty())
        {
            util::warn_message("No session name");
            ctrler->session_name("None");
        }
        else
            ctrler->session_name(s);
    }
    else if (msgtag == osc::tag::error)
    {
        /*
         * Code basically copied from lowrapper::osc_error().
         * Note that function is called from the nsmclient::osc_nsm_error()
         * function as well.
         */

        if (std::string(types) != "sis")
        {
            util::error_message("Error types received is not 'sis'");
            return osc::osc_msg_unhandled();
        }
        if (argc >= 3)
        {
            std::string pathmsg { osc::string_from_lo_arg(argv[0]) };
            std::string errmsg { osc::string_from_lo_arg(argv[2]) };
            int err = argv[1]->i;
            if (err != 0)
            {
                util::error_printf
                (
                    "Command %s failed with error %d: %s",
                    V(pathmsg), err, V(errmsg)
                );
                if (pathmsg == std::string("/nsm/server/announce"))
                {
                    util::error_message("Failed to register with NSM", errmsg);
                    ept->active(false);
                }
            }
        }
    }
    else if (msgpath == "/reply" && /* argc > 3 && */ msgtypes.front() == 's')
    {
        if (msgtag == osc::tag::replyex)
        {
            /*
             * Again, see lowrapper::osc_reply()
             */

            if (msgtag == osc::tag::srvannounce)
            {
                util::status_printf
                (
                    "Server hello '%s' from NSM %s with caps %s",
                    V(s1), V(osc::string_from_lo_arg(argv[2])),
                    V(osc::string_from_lo_arg(argv[3]))
                );
            }
        }
        else if (msgtag == osc::tag::reply)
        {
            ctrler->log_status(s1);
            util::info_printf("%s says %s", V(s), V(s1));
        }
        else if (s == osc::tag_message(osc::tag::srvlist))
        {
            ctrler->add_session_to_list(s1);
        }
        else if (s == osc::tag_message(osc::tag::oscping))
        {
            int t = int(time(NULL));
            int delta = t - int(ctrler->m_last_ping_response);
            std::string dtext = std::to_string(delta);
            ctrler->m_last_ping_response = time_t(t);
            util::info_printf
            (
                "Received ping response after %s seconds", V(dtext)
            );
        }
    }
    if (util::strncompare(path, "/nsm/gui/client/"))
    {
        if (msgtag == osc::tag::guinew)
        {
             if (! ctrler->client_new(s, s1))
                 return osc::osc_msg_unhandled();
        }
        else
        {
            nsmctlclient * c = ctrler->client_by_id(s);
            if (not_nullptr(c))
            {
                if (msgtag == osc::tag::guistatus)
                {
                    ctrler->client_pending_command(c, s1);
                }
                else if ( msgtag == osc::tag::guiprogress)
                {
                    c->progress(argv[1]->f);
                }
                else if (msgtag == osc::tag::guidirty)
                {
                    c->dirty(bool(argv[1]->i));
                }
                else if (msgtag == osc::tag::guivisible)
                {
                    c->gui_visible(bool(argv[1]->i));
                }
                else if (msgtag == osc::tag::guilabel)
                {
                    c->client_label(s1);
                }
                else if (msgtag == osc::tag::guioption)
                {
                    // c->has_optional_gui();
                    util::warn_message("osc_handler()", "No optional GUI");
                }
                else if (msgtag == osc::tag::guiswitch)
                {
                    c->client_id(s1);
                }
            }
            else
            {
                util::info_printf
                (
                    "Message '%s' from unknown client '%s'", V(path), V(s)
                );
            }
        }
    }
    return osc::osc_msg_handled();
}

std::string
nsmcontroller::get_session_list () const
{
    std::string result;
    for (const auto & s : m_session_list)
    {
        result += "    ";
        result += s;
        result += "\n";
    }
    return result;
}

};          // namespace nsm

/*
* nsmcontroller.cpp
*
* vim: sw=4 ts=4 wm=4 et ft=cpp
*/


