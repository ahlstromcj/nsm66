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
 * \file          endpoint.cpp
 *
 *    This module refactors the Endpoint class to replace C code with
 *    C++ code.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-02-05
 * \updates       2025-04-23
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 * Notes on /reply:
 *
 *  The replies listed in the NSM API manual:
 *
 *      -   /reply "/nsm/server/announce" s:message s:name_of_session_manager
 *              s:capabilities ["ssss", tag replyex]
 *      -   /reply s:path s:message ["ss}, tag reply]
 *
 *  Not listed is the response to an "/osc/ping":
 *
 *      -   /reply path ["s"]
 *
 *  The replies we handle:
 *
 *      -   endpoint::osc_reply() receives "/reply" + NULL [sigreply]
 *      -   lowrapper::osc_reply() receives:
 *          -   "/reply" + "ss" [reply]
 *          -   "/reply" + "s" [srvreplyex]
 *          -   "/reply" + "ssss" [replyex] NEW!
 */

#include "osc/endpoint.hpp"             /* osc::endpoint class              */
#include "util/msgfunctions.hpp"        /* util::info_message(), _print()   */
#include "util/strfunctions.hpp"        /* util::strncompare()              */

/*
 * CLANG and LO_TT_IMMEDIATE:
 *
 *      Warning: compound literals are a C99-specific feature [-Wc99-extensions]
 *      This is due to using, in lo_osc_types.h:
 *
 *      #define LO_TT_IMMEDIATE ((lo_timetag){0U,1U})       // i.e. "now"   //
 *
 *      versus "lo_timetag lo_get_tt_immediate();" as LO_TT_IMMEDIATE
 */

namespace osc
{

endpoint::endpoint () :
    lowrapper       (),
    m_owner         (nullptr),
    m_thread        (),
    m_peers         (),
    m_signals       (),
    m_methods       (),
    m_learning_path (),
    m_translations  (),
    m_name          (),
    m_peer_scan_complete_userdata       (),
    m_peer_signal_notification_userdata (),
    m_peer_scan_complete_callback       (),
    m_peer_signal_notification_callback ()
{
    /*
     * util::debug_printf("endpoint @ %p", this);
     */
}

/*
 *  Also see the freeing of the server and address done in ~lowwrapper().
 */

endpoint::~endpoint ()
{
    for (auto & mp : m_methods)
        delete mp;

    m_methods.clear();
}

/*
 * These messages are provide in the message structure lookup list
 * s_signal_msgs in the nsmmessagesex module. Example: nsm::tag::hello.
 *
 * Definitions from the seq66-based nsm subdirectories:
 *
 * nsmbase:
 *
 *      -   error
 *      -   reply
 *
 * nsmclient:
 *
 *      -   replyex
 *      -   open
 *      -   save
 *      -   loaded
 *      -   label
 *      -   show
 *      -   hide
 *      -   null
 *
 * \param proto
 *      Provides the OSC protocol(s) to use. It's a bit-mask set of
 *      #defines in lo_macros.h; values are LO_DEFAULT, LO_UDP (used
 *      in nsm66d), LO_UNIX, and LO_TCP.
 *
 * \param portname
 *      The name of the OSC port. It's the required --osc-port/-p
 *      command-line argument in nsm66d. If empty, a null pointer
 *      is used instead of c_str(). It can also be the port name
 *      of a looked-up port.
 *
 * \param usethis
 *      If true, the "this" pointer of this object is used in adding
 *      method callbacks. Otherwise, the null pointer is used.
 *
 * \return
 *      Returns true if successful, and false if it fails.
 */

bool
endpoint::init
(
    int proto,
    const std::string & portname,
    bool usethis
)
{
    if (lowrapper::init(proto, portname, usethis))
    {
        /*
         *  If init() is called via an endpoint pointer,
         *  endpoint::add_methods() is called.
         *
         *  void * userdata = usethis ? static_cast<void *>(this) : nullptr ;
         *  add_methods(userdata);
         */
    }
    else
    {
        /*
         * Already reported in lowrapper.
         *
         * util::warn_message("Error creating OSC server");
         */

         return false;
    }
    return true;
}

/**
 *  An override of lowrapper::add_methods(); it currently avoids calling
 *  the lowrapper::add_methods() function.
 */

void
endpoint::add_methods (void * userdata)
{
    add_osc_method
    (
        osc::tag::sighello, &endpoint::osc_sig_hello, userdata
    );
    add_osc_method
    (
        osc::tag::sigconnect, &endpoint::osc_sig_connect, userdata
    );
    add_osc_method
    (
        osc::tag::sigdisconnect, &endpoint::osc_sig_disconnect, userdata
    );
    add_osc_method
    (
        osc::tag::sigrenamed, &endpoint::osc_sig_renamed, userdata
    );
    add_osc_method
    (
        osc::tag::sigremoved, &endpoint::osc_sig_removed, userdata
    );
    add_osc_method
    (
        osc::tag::sigcreated, &endpoint::osc_sig_created, userdata
    );
    add_osc_method
    (
        osc::tag::siglist, &endpoint::osc_signal_lister, userdata
    );
    add_osc_method
    (
        osc::tag::sigreply, &endpoint::osc_reply, userdata
    );
    add_osc_method
    (
        osc::tag::generic, &endpoint::osc_generic, userdata
    );

    /*
     * Currently not enabled for work on nsmctl.
     *
     * lowrapper::add_methods()
     */
}

osc::signal *
endpoint::find_target_by_peer_address (signal_list * lst, lo_address addr )
{
    for (const auto & s : *lst)
    {
        if (address_matches(addr, s->m_peer->p_addr))
            return s;
    }
    return nullptr;
}

osc::signal *
endpoint::find_peer_signal_by_path (peer * p, const std::string & path)
{
    for (const auto & s : p->p_signals)
    {
        if (s->path() == path)
            return s;
    }
    return nullptr;
}

osc::signal *
endpoint::find_signal_by_path (const std::string & path)
{
    for (const auto & s : m_signals)
    {
        if (s->path() == path)
            return s;
    }
    return nullptr;
}

/**
 *  Send function for "/signal/hello" + "ss".
 */

void
endpoint::hello (const std::string & url)
{
    lo_address addr = lo_address_new_from_url(CSTR(url));
    std::string our_url = this->url();
    send(addr, tag_message(tag::sighello), name(), CSTR(our_url));
    lo_address_free(addr);
}

/**
 *  Send function for "/signal/list" + nothing.
 */

void
endpoint::handle_hello
(
    const std::string & peer_name,
    const std::string & peer_url
)
{
    util::info_message("Hello from", peer_name);

    peer * p = find_peer_by_name(peer_name);
    if (is_nullptr(p))
    {
        scan_peer(peer_name, peer_url);
    }
    else
    {
        /*
         * Maybe the peer has a new URL, so update the address.
         * Then scan it while we're at it.
         */

        lo_address addr = lo_address_new_from_url(CSTR(peer_url));
        if (address_matches(addr, p->p_addr))
        {
            free(addr);
            return;
        }
        if (p->p_addr)
            free(p->p_addr);

        util::info_message("Scanning peer", peer_name);
        p->p_addr = addr;
        p->p_scanning = true;
        send(p->p_addr, tag_message(tag::siglist));
    }
    if (! name().empty())
        hello(peer_url);
    else
        util::info_message("Not sending hello; we don't have a name yet");
}

/**
 *  Note that lo_arg is a union of various numeric types, characters and
 *  strings, an lo_timetag, 4-byte MIDI packet, and a blob structure.
 *  Here, "s" gets a standard null-terminated C string via the address
 *  of a character.
 */

int
endpoint::osc_sig_hello
(
    const char * path, const char * types,
    lo_arg ** argv, int argc,
    lo_message /*msg*/, void * userdata
)
{
    osc_msg_summary("endpoint::osc_sig_hello", path, types, argv, argc, userdata);
    if (argc >= 2)
    {
        endpoint * ep = static_cast<endpoint *>(userdata);
        if (not_nullptr(ep))
        {
            const char * peer_name = &argv[0]->s;
            const char * peer_url = &argv[1]->s;
            ep->handle_hello(peer_name, peer_url);
        }
        else
        {
            util::error_message("osc_sig_hello()", "null endpoint");
            return osc_msg_unhandled();
        }
    }
    return osc_msg_handled();
}

int
endpoint::osc_sig_disconnect
(
    const char * path, const char * types,
    lo_arg ** argv, int argc,
    lo_message /*msg*/, void * userdata
)
{
    osc_msg_summary
    (
        "endpoint::osc_sig_disconnect", path, types, argv, argc, userdata
    );
    if (argc >= 2)
    {
        std::string their_name { string_from_lo_arg(argv[0]) };
        std::string our_name { string_from_lo_arg(argv[1]) };
        endpoint * ep = static_cast<endpoint *>(userdata);
        if (is_nullptr(ep))
        {
            util::error_message("osc_disconnect()", "null endpoint");
            return osc_msg_unhandled();
        }

        signal * s = ep->find_signal_by_path(our_name);
        if (is_nullptr(s))
            return osc_msg_handled();           /* no such signal to handle */

        if (s->m_direction == signal::input)
        {
            std::string dummymsg = util::string_format
            (
                "Peer %s disconnected from signal %s",
                V(our_name), V(their_name)
            );
            util::info_printf
            (
                "Peer %s disconnected from signal %s",
                V(our_name), V(their_name)
            );
            ep->del_translation(their_name);
            if (s->m_connection_state_callback)
            {
                s->m_connection_state_callback
                (
                    s, s->m_connection_state_userdata
                );
            }
        }
    }
    return osc_msg_handled();
}

int
endpoint::osc_sig_connect
(
    const char * path, const char * types,
    lo_arg ** argv, int argc,
    lo_message /*msg*/, void * userdata
)
{
    osc_msg_summary
    (
        "endpoint::osc_sig_connect", path, types, argv, argc, userdata
    );
    if (argc >= 2)
    {
        std::string src_path { string_from_lo_arg(argv[0]) };
        std::string dst_path { string_from_lo_arg(argv[1]) };
        endpoint * ep = static_cast<endpoint *>(userdata);
        if (is_nullptr(ep))
        {
            util::error_message("osc_sig_connect()", "null endpoint");
            return osc_msg_unhandled();
        }

        signal * dst_s = ep->find_signal_by_path(dst_path);
        if (is_nullptr(dst_s))
        {
            util::warn_message
            (
                "Unknown destination signal in connection attempt",
                dst_path
            );
            return osc_msg_handled();
        }
        if (dst_s->m_endpoint != ep)
        {
            util::warn_message
            (
                "Connection request for a destination signal we don't own"
            );
            return osc_msg_handled();
        }
        util::info_printf
        (
            "Has requested signal connection %s |> %s",
            V(src_path), V(dst_s->path())
        );
        ep->add_translation(src_path, dst_s->path());
    }
    return osc_msg_handled();
}

int
endpoint::osc_sig_removed
(
    const char * path, const char * types,
    lo_arg ** argv, int argc,
    lo_message msg, void * userdata
)
{
    osc_msg_summary
    (
        "endpoint::osc_sig_removed", path, types, argv, argc, userdata
    );
    if (argc >= 1)
    {
        std::string name { string_from_lo_arg(argv[0]) };
        endpoint * ep = static_cast<endpoint *>(userdata);
        if (is_nullptr(ep))
        {
            util::error_message("osc_sig_removed()", "null endpoint");
            return osc_msg_unhandled();
        }

        peer * p = ep->find_peer_by_address(lo_message_get_source(msg));
        if (is_nullptr(p))
        {
            util::warn_message("Signal-removed message from unknown peer");
            return osc_msg_handled();
        }

        signal * o = ep->find_peer_signal_by_path(p, name);
        if (is_nullptr(o))
        {
            util::warn_message("Unknown signal", name);
            return osc_msg_handled();
        }
        util::info_printf
        (
            "signal %s:%s was removed", V(o->m_peer->p_name), V(o->path())
        );
        if (ep->m_peer_signal_callback)
        {
            ep->m_peer_signal_callback
            (
                o, signal::removed, ep->m_peer_signal_userdata
            );
        }
        p->p_signals.remove(o);
        delete o;
    }
    return osc_msg_handled();
}

int
endpoint::osc_sig_created
(
    const char * path, const char * types,
    lo_arg ** argv, int argc,
    lo_message msg, void * userdata
)
{
    osc_msg_summary
    (
        "endpoint::osc_sig_created", path, types, argv, argc, userdata
    );
    if (argc >= 5)
    {
        endpoint * ep = static_cast<endpoint *>(userdata);
        if (is_nullptr(ep))
        {
            util::error_message("osc_sig_created()", "null endpoint");
            return osc_msg_unhandled();
        }

        std::string name { string_from_lo_arg(argv[0]) };
        std::string direction { string_from_lo_arg(argv[1]) };
        const float min = argv[2]->f;
        const float max = argv[3]->f;
        const float default_value = argv[4]->f;
        peer * p = ep->find_peer_by_address(lo_message_get_source(msg));
        if (is_nullptr(p))
        {
            util::warn_message("Signal creation message from unknown peer");
            return osc_msg_handled();
        }

        signal::direction dir = signal::bidirectional;
        if (direction == "in")
            dir = signal::input;
        else if (direction == "out")
            dir = signal::output;

        signal * s = new (std::nothrow) signal(name, dir);
        if (not_nullptr(s))
        {
            s->m_peer = p;
            s->set_parameter_limits(min, max, default_value);
            p->p_signals.push_back(s);
            util::info_printf
            (
                "Peer %s created signal %s (%s %f %f %f)",
                V(p->p_name), V(name), V(direction), min, max, default_value
            );
        }
        if (ep->m_peer_signal_callback)
        {
            ep->m_peer_signal_callback
            (
                s, signal::created, ep->m_peer_signal_userdata
            );
        }
    }
    return osc_msg_handled();
}

int
endpoint::osc_sig_renamed
(
    const char * path, const char * types,
    lo_arg ** argv, int argc,
    lo_message msg, void * userdata
)
{
    osc_msg_summary
    (
        "endpoint::osc_sig_renamed", path, types, argv, argc, userdata
    );
    if (argc >= 2)
    {
        std::string old_name { string_from_lo_arg(argv[0]) };
        std::string new_name { string_from_lo_arg(argv[1]) };
        endpoint * ep = static_cast<endpoint *>(userdata);
        if (is_nullptr(ep))
        {
            util::error_message("osc_sig_renamed()", "null endpoint");
            return osc_msg_unhandled();
        }

        peer * p = ep->find_peer_by_address(lo_message_get_source(msg));
        if (is_nullptr(p))
        {
            util::warn_message("Signal-rename message from unknown peer");
            return osc_msg_handled();
        }

        signal * o = ep->find_peer_signal_by_path(p, old_name);
        if (is_nullptr(o))
        {
            util::warn_message("Unknown signal", old_name );
            return osc_msg_handled();
        }

        util::info_printf
        (
            "Signal %s renamed to %s", V(o->m_path), V(new_name)
        );
        ep->rename_translation_source(o->m_path, new_name);
        o->m_path = new_name;
    }
    return osc_msg_handled();
}

/*
 * Need to check types here. Maybe an argument checker?
 *
 * In any case, the types parameter can be either null or empty, and then we
 * reply with the current floating-point value, using the same path as
 * message. There is currently no "official" "/reply" + "sf". The only
 * message with "sf" is "/nsm/gui/client/progress".
 */

int
endpoint::osc_sig_handler
(
    const char * path, const char * types,
    lo_arg ** argv, int argc,
    lo_message msg, void * userdata
)
{
    osc_msg_summary
    (
        "endpoint::osc_sig_handler", path, types, argv, argc, userdata
    );
    if (argc >= 1)
    {
        signal * o = static_cast<signal *>(userdata);
        if (is_nullptr(o))
        {
            util::error_message("osc_sig_handler()", "null signal");
            return osc_msg_unhandled();
        }

        float f = 0.0;
        if (std::string(types) == "f" )     /* accept the float value       */
        {
            f = argv[0]->f;
        }
        else if (is_nullptr(types) || types[0] == 0)
        {
            o->m_endpoint->send
            (
                lo_message_get_source(msg), tag_message(tag::reply),
                path, o->value()
            );
            return osc_msg_handled();
        }
        else
            return osc_msg_unhandled();

        o->m_value = f;
        if (o->m_handler)
            o->m_handler(f, o->m_user_data);
    }
    return osc_msg_unhandled();
}

/**
 *  This function originally created an array of string pointers while
 *  traversing the translation map looking for matches to path.
 *  We will use a vector of strings, lib66::tokenization.
 */

lib66::tokenization
endpoint::get_connections (const std::string & path)
{
    lib66::tokenization result;
    for (const auto & t : m_translations)
    {
        if (t.second.m_path == path)
            result.push_back(t.first);
    }
    return result;
}

void
endpoint::clear_translations ()
{
    m_translations.clear();
}

void
endpoint::add_translation (const std::string & a, const std::string & b)
{
    m_translations[a].m_path = b;
}

void
endpoint::del_translation (const std::string & a)
{
    translation_map::iterator i = m_translations.find(a);
    if (i != m_translations.end())
        m_translations.erase(i);
}

void
endpoint::rename_translation_destination
(
    const std::string & a,
    const std::string & b
)
{
    for (auto & t : m_translations)
    {
        if (t.second.m_path == a)
        {
            t.second.m_path = b;
            break;                          // is this okay?
        }
    }
}

void
endpoint::rename_translation_source
(
    const std::string & a,
    const std::string & b
)
{
    translation_map::iterator i = m_translations.find(a);
    if (i != m_translations.end())
    {
        m_translations[b] = m_translations[a];
        m_translations.erase(i);
    }
}

int
endpoint::ntranslations ()
{
    return m_translations.size();
}

bool
endpoint::get_translation
(
    int n, std::string & from, std::string & to
)
{
    bool result = false;
    int j = 0;
    for (const auto & t : m_translations)
    {
        if (j == n)
        {
            from = t.first;
            to = t.second.m_path;
            result = true;
            break;
        }
        ++j;
    }
    return result;
}

int
endpoint::osc_generic
(
    const char * path, const char * types, lo_arg ** argv, int argc,
    lo_message msg, void * userdata
)
{
    endpoint * ep = static_cast<endpoint *>(userdata);
    osc_msg_summary
    (
        "endpoint::osc_generic", path, types, argv, argc, userdata
    );
    if (is_nullptr(ep))
    {
        util::error_message("osc_generic()", "null endpoint");
        return osc_msg_unhandled();
    }
    if (argc >= 1)
    {
        if (! ep->m_learning_path.empty())
        {
            ep->add_translation(path, ep->m_learning_path);
            util::info_printf
            (
                "Learned translation \"%s\" -> \"%s\"",
                path, V(ep->m_learning_path)
            );
            ep->m_learning_path.clear();
            return osc_msg_handled();
        }

        std::string ppath = std::string(path);
        translation_map::iterator i = ep->m_translations.find(path);
        if (i != ep->m_translations.end())
        {
            std::string dpath = i->second.m_path;
            if (std::string(types) == "f")
                i->second.m_current_value = argv[0]->f;

            i->second.m_suppress_feedback = true;
            lo_send_message(ep->address(), CSTR(dpath), msg);
            return osc_msg_handled();
        }
        if (ppath.back() != '/')
            return osc_msg_unhandled();

        for (const auto & m : ep->m_methods)        /* a list of pointers   */
        {
            if (m->path().empty())
                continue;

            if (util::strncompare(m->path(), ppath, ppath.length()))
            {
                ep->send
                (
                    lo_message_get_source(msg), tag_message(tag::reply),
                    path, m->path()
                );
            }
        }
        ep->send
        (
            lo_message_get_source(msg), tag_message(tag::srvreply), path
        );
    }
    return osc_msg_handled();
}

/*-------------------------------------------------------------------------
 * Static member functions used by the OSC service.
 *-------------------------------------------------------------------------*/

void
endpoint::error_handler (int num, const char * msg, const char * path)
{
    util::error_printf("OSC server error in endpoint");
    util::error_printf("OSC server error %d, path %s: %s\n", num, path, msg);
}

/**
 *  Check the main parameters. The other might be null.
 */

bool
endpoint::osc_params_check
(
    const char * path, const char * types, lo_arg ** argv, int argc
)
{
    bool result = not_nullptr(path) && not_nullptr(types);
    if (result)
    {
        if (argc > 0)
            result = not_nullptr(argv);
    }
    return result;
}

/**
 *  Handles "/signal/list" via the sigreply tag, which is a "/reply" with
 *  a NULL typespec.
 *
 *  Figure out why there can be 6 arguments to a "/reply". The type
 *  specification seems to be "sssfff". The argv's are:
 *
 *          0.  Signal command (e.g. "/signal/list").
 *          1.  Path name, i.e. the name of the signal.
 *          2.  Direction name, either "in" or "out".
 *          3.  Parameter limit minimum.
 *          4.  Parameter limit maximum.
 *          5.  Parameter limit default value.
 *
 *  Currently the closest match is "/signal/created" + "ssfff".
 */

int
endpoint::osc_reply
(
    const char * path, const char * types,
    lo_arg ** argv, int argc,
    lo_message msg, void * userdata
)
{
    endpoint * ep = static_cast<endpoint *>(userdata);
    osc_msg_summary("endpoint::osc_reply", path, types, argv, argc, userdata);
    if (is_nullptr(ep))
    {
        util::error_message("osc_reply()", "null endpoint");
        return osc_msg_unhandled();
    }

    std::string sigcmd = &argv[0]->s;
    if (argc > 0 && sigcmd == tag_message(tag::siglist))
    {
        peer * p = ep->find_peer_by_address(lo_message_get_source(msg));
        if (is_nullptr(p))
        {
            util::warn_message("Input list reply from unknown peer");
            return osc_msg_handled();
        }
        if (argc == 1)
        {
            p->p_scanning = false;
            util::info_message("Done scanning", p->p_name);
            if (ep->m_peer_scan_complete_callback)
            {
                ep->m_peer_scan_complete_callback
                (
                    ep->m_peer_scan_complete_userdata
                );
            }
        }
        else if (argc == 6 && p->p_scanning)
        {
            std::string pathname { string_from_lo_arg(argv[1]) };
            signal * s = ep->find_peer_signal_by_path(p, pathname);
            if (not_nullptr(s))
                return osc_msg_handled();

            std::string directionname { string_from_lo_arg(argv[2]) };
            util::info_printf
            (
                "Peer %s has signal %s (%s)",
                V(p->p_name), V(pathname), V(directionname)
            );

            signal::direction dir = signal::bidirectional;
            if (directionname == "in")
                dir = signal::input;
            else if (directionname == "out")
                dir = signal::output;

            s = new (std::nothrow) signal(pathname, dir);
            if (not_nullptr(s))
            {
                s->m_peer = p;
                s->set_parameter_limits(argv[3]->f, argv[4]->f, argv[5]->f);
                p->p_signals.push_back(s);
                if (ep->m_peer_signal_callback)
                {
                    ep->m_peer_signal_callback
                    (
                        s, signal::created, ep->m_peer_signal_userdata
                    );
                }
            }
        }
        return osc_msg_handled();
    }
    else
        return osc_msg_unhandled();
}

int
endpoint::osc_signal_lister
(
    const char * path, const char * types,
    lo_arg ** argv, int argc,
    lo_message msg, void * userdata
)
{
    endpoint * ep = static_cast<endpoint *>(userdata);
    osc_msg_summary
    (
        "endpoint::osc_signal_lister", path, types, argv, argc, userdata
    );
    if (is_nullptr(ep))
    {
        util::error_message("osc_signal_lister()", "null endpoint");
        return osc_msg_unhandled();
    }

    std::string prefix;
    if (argc > 0)
        prefix = &argv[0]->s;

    util::info_message("Listing signals...");
    for (const auto & s : ep->m_signals)
    {
        signal * o = s;
        if (util::strncompare(o->path(), prefix, prefix.length()))
        {
            const parameter_limits & pl = o->get_parameter_limits();
            ep->send
            (
                lo_message_get_source(msg), tag_message(tag::reply),
                path, o->path(),
                o->m_direction == signal::input ? "in" : "out",
                pl.pl_min, pl.pl_max, pl.pl_default_value
            );
        }
    }
    ep->send(lo_message_get_source(msg), tag_message(tag::srvreply),
    path);
    return osc_msg_handled();
}

/**
 *  Static function.
 */

bool
endpoint::address_matches (lo_address addr1, lo_address addr2)
{
    std::string purl = lo_address_get_port(addr1);
    std::string url = lo_address_get_port(addr2);
    return purl == url;
}

#if defined USE_LIST_PEER_SIGNALS

/**
 *  Not static. Not used (yet) even in jackpatch and nsmd.
 */

void
endpoint::list_peer_signals (void * v)
{
    for (const auto & mp : m_peers)
    {
        for (const auto & s : mp.m_signals)
        {
            if (m_peer_signal_notification_callback)
            {
                m_peer_signal_notification_callback
                (
                    s, osc::signal::created, v
                );
            }
        }
    }
}

#endif

peer *
endpoint::find_peer_by_address (lo_address addr)
{
    std::string url = lo_address_get_port(addr);
    peer * p = nullptr;
    for (const auto & mp : m_peers)
    {
        std::string purl = lo_address_get_port(mp->p_addr);
        if (purl == url)
        {
            p = mp;     /* *i; */
            break;
        }
    }
    return p;
}

peer *
endpoint::find_peer_by_name (const std::string & name)
{
    for (const auto & mp : m_peers)
    {
        if (name == mp->p_name)
            return mp;
    }
    return nullptr;
}

/**
 *  Send function for "/signal/connect" + "ss"
 */

bool
endpoint::connect_signal (osc::signal * s, const std::string & signal_path)
{
    if (s->m_direction == signal::output)
    {
        for (const auto & mp : m_peers)
        {
            send
            (
                mp->p_addr, tag_message(tag::sigconnect),
                s->path(), signal_path
            );
        }
    }
    return true;
}

/**
 *  Send function for "/signal/disconnect" + "ss"
 */

bool
endpoint::disconnect_signal (osc::signal * s, const std::string & signal_path)
{
    if (s->m_direction == signal::output)
    {
        for (const auto & mp : m_peers)
        {
            send
            (
                mp->p_addr, tag_message(tag::sigdisconnect),
                s->path(), signal_path
            );
        }
        return true;
    }
    return false;
}

/*
 *  This function is similar to the untested add_server_method() in nsmbase.
 *
 *  We need to make the method class true C++. strdup()? Jesus!
 *
 *  lo_server_add_method() returns an OSC Method, which is a small wrapper
 *  class that has lo_method as a constructor parameter and provides
 *  operator lo_method() to access it. Here, we do not use that parameter
 *
 * \param path
 *      Provides an operation to perform, such as "/nsm/server/announce".
 *
 * \param typespec
 *      Provides the set of data items to be handled, such as "sis".
 *      See the osc_value header file.
 *
 * \param handler
 *      Provides a free function or a static member function to be
 *      registered as an OSC callback.
 *
 * \param userdata
 *      Provides a pointer to an optional set of data. If set to
 *      nullptr (the default), then "this" is used.
 *
 * \param argument_description
 *      Provides a description of parameters, such as "err_code,msg".
 *      Defaults to an empty string. Note that the parameters must
 *      align with the typespec. For example, "/nsm/client/progress"
 *      requires a single floating-point parameter, so its typespec
 *      would be "f", and the argument description would be "progress".
 *
 *      WARNING: we need to reconcile this matchup with the various
 *               "error" and "reply" paths, and maybe some others.
 *
 * \return
 *      Returns a pointer to a new method object if this function
 *      succeeds.
 */

method *
endpoint::add_method
(
    const std::string & path,
    const std::string & typespec,
    lo_method_handler handler,
    void * userdata,
    const std::string & argument_description
)
{
    if (is_nullptr(userdata))
        userdata = this;

    (void) /* Method meth = */ lo_server_add_method
    (
        server(), OPTR(path), OPTR(typespec),
        handler, userdata
    );
    method * md = new (std::nothrow) method;
    if (not_nullptr(md))
    {
        md->m_path = path;
        md->m_typespec = typespec;
        md->m_documentation = argument_description;
        m_methods.push_back(md);
    }
    return md;
}

/**
 *  Add a signal handler via lo_server_add_method() and tell our peers
 *  about it.
 *
 *  Sends "/signal/created" + "ssfff"
 */

signal *
endpoint::add_signal
(
    const std::string & path,
    signal::direction dir,
    float min, float max, float default_value,
    signal_handler handler,
    void * userdata
)
{
    std::string s = util::string_format("%s%s", V(name()), V(path));
    signal * o = new (std::nothrow) signal(s, dir);
    if (not_nullptr(o))
    {
        o->m_handler = handler;
        o->m_user_data = userdata;
        o->m_endpoint = this;
        o->set_parameter_limits(min, max, default_value);
        m_signals.push_back(o);
        lo_server_add_method
        (
            server(), OPTR(o->m_path), NULL, osc_sig_handler, o
        );
        for (const auto & mp : m_peers)
        {
            send
            (
                mp->p_addr, tag_message(tag::sigcreated),
                o->path(), o->m_direction == signal::input ? "in" : "out",
                min, max, default_value
            );
        }
    }
    return o;
}

#if defined USE_DEL_METHOD

/**
 *  lo_server_del_method() returns void.
 *
 *  This is a weird one. We tried to use the following, but could not
 *  erase the entry:
 *
 *      for (auto m : m_methods)
 */

void
endpoint::del_method (const std::string & path, const std::string & typespec)
{
    lo_server_del_method(server(), OPTR(path), OPTR(typespec));
    for (method_list::iterator i = m_methods.begin(); i != m_methods.end(); ++i)
    {
        method * p = *i;
        if (p->path().empty())
            continue;

        if (path == p->path() && typespec == p->typespec())
        {
            delete p;
            (void) m_methods.erase(i);
            break;
        }
    }
}

void
endpoint::del_method (method * m)
{
    lo_server_del_method(server(), m->path(), m->typespec());
    delete m;
    m_methods.remove(m);
}

#endif

/**
 *  Delete a signal and tell our peers about it.
 *
 *  Sends "/signal/removed" + "s"
 */

void
endpoint::del_signal (signal * o)
{
    lo_server_del_method(server(), OPTR(o->path()), NULL);
    for (const auto & mp : m_peers)
        send(mp->p_addr, tag_message(tag::sigremoved), OPTR(o->path()));

    /*
     * FIXME: clear loopback connections first!
     */

    m_signals.remove(o);
}

/**
 * Prepare to learn a translation for /path/. The next unhandled message to
 * come through will be mapped to /path/
 */

void
endpoint::learn (const std::string & path)
{
    m_learning_path = path;
}

/**
 *  if there's a translation with a destination of 'path', then send feedback
 *  for it to all peers.
 */

void
endpoint::send_feedback (const std::string & path, float v)
{
    for (auto & t : m_translations)
    {
        if (t.second.m_path == path)                        /* found it */
        {
            if
            (
                ! t.second.m_suppress_feedback &&
                t.second.m_current_value != v
            )
            {
                const std::string & spath = t.first;
                for (const auto & mp : m_peers)
                    send(mp->p_addr, OPTR(spath), v);

                t.second.m_current_value = v;
            }
            t.second.m_suppress_feedback = false;

            /* break; ??? */
        }
    }
}

peer *
endpoint::add_peer (const std::string & name, const std::string & url)
{
    peer * p = new (std::nothrow) peer;
    if (not_nullptr(p))
    {
        util::info_printf("Adding peer %s@%s...", V(name), V(url));
        p->p_name = name;
        p->p_addr = lo_address_new_from_url(CSTR(url));
        m_peers.push_back(p);
    }
    else
        util::error_printf("Could not add peer %s@%s...", V(name), V(url));

    return p;
}

/**
 *  Handles "/signal/list", for scanning peers.
 */

void
endpoint::scan_peer (const std::string & name, const std::string & url)
{
    peer * p = add_peer(name,url);
    p->p_scanning = true;
    util::info_message("Scanning peer", name);
    send(p->p_addr, tag_message(tag::siglist));
}

/**
 *  Static function.
 */

void *
endpoint::osc_thread (void * arg)
{
    static_cast<endpoint *>(arg)->osc_thread();
    return nullptr;
}

void
endpoint::osc_thread ()
{
    /*
     * This message gets broken up.
     */

    util::info_message("OSC Thread running");
    m_thread.name("OSC");
    run();
}

void
endpoint::start ()
{
    if (! m_thread.clone(&endpoint::osc_thread, this))
        util::error_message("Could not create OSC thread");

    /*
     * lo_server_thread_start(m_st);
     */
}

void
endpoint::stop ()
{
    m_thread.join();                    /* lo_server_thread_stop(m_st);     */
}

int
endpoint::port () const
{
    return lo_server_get_port(server());
}

/**
 *  Process any waiting events and return immediately.
 */

void
endpoint::check () const
{
    wait(0);
}

/**
 *  Process any waiting events and return after timeout.
 *
 *      lo_server_wait(lo_server, timeout)) returns 1 if a message
 *      is waiting, and 0 if not. A timeout of 0 returns immediately.
 *
 *      lo_server_recv_noblock(lo_server, timeout) receives a message,
 *      dispatches it to a matching method, if found,
 *      and returns the number of bytes in the message.
 */

void
endpoint::wait (int timeout) const
{
    const int s_recv_timeout = 0;           /* return immediately           */
    if (not_nullptr(server()) && lo_server_wait(server(), timeout))
    {
        for (;;)
        {
            int count = lo_server_recv_noblock(server(), s_recv_timeout);
            if (count == 0)
                break;

            if (! active())
                break;

#if defined PLATFORM_DEBUG
            util::info_printf("Recv'd %d bytes", count);
#endif
        }
    }
}

/**
 *  Process events forever. Does lo_server_recv() block?
 */

void
endpoint::run () const
{
    const int s_recv_timeout = 100;
    for (;;)
    {
        // lo_server_recv(server());

        lo_server_recv_noblock(server(), s_recv_timeout);
        if (! active())
            break;
    }
}

}           // namespace osc

/*
 * endpoint.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


