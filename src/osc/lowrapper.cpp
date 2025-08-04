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
 * \file          lowrapper.cpp
 *
 *    This module wrappers the most common code needed for communication
 *    via OSC.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-02-26
 * \updates       2025-04-23
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  This class is, in a sense, a greatly cut-down version of the
 *  endpoint class.
 */

#include <unistd.h>                     /* getpid()                         */

#include "nsm/nsmcodes.hpp"             /* nsm::error & nsm::command enums  */
#include "osc/lowrapper.hpp"            /* osc::lowwrapper base class       */
#include "util/msgfunctions.hpp"        /* util::info_message(), _print()   */
#include "util/strfunctions.hpp"        /* util::strncompare(), CSTR(), ... */

namespace osc
{

namespace
{

/*
 * Some helper functions and structures, currently internal.
 */

lo_timetag &
get_lo_timetag ()
{
    static lo_timetag s_lo_timetag { 0, 1 };    /* used in Clang builds     */
    return s_lo_timetag;
}

#if defined USE_STATIC_LOWRAPPER_CAST

const lowrapper *
lowrapper_cast (void * p)
{
    return static_cast<const lowrapper *>(p);
}

#endif

lowrapper *
lowrapper_const_cast (void * p)
{
    return static_cast<lowrapper *>(p);
}

}           // namespace anonymous

/*
 *  Default constructor.
 */

lowrapper::lowrapper () :
    m_server        (),             /* accessor: server()               */
    m_address       (),             /* accessor: address()              */
    m_port_name     (),
    m_active        (false)
{
    /*
     * util::debug_printf("lowrapper @ %p", this);
     */

    (void) get_lo_timetag();        /* used in clang builds             */
}

/*
 *  Virtual destructor.
 */

lowrapper::~lowrapper ()
{
    if (not_nullptr(server()))
    {
        lo_server_free(server());
        server(nullptr);
    }
    if (not_nullptr(address()))
    {
        lo_address_free(address());
        address(nullptr);
    }
}

std::string
lowrapper::url () const
{
    std::string result;
    char * u = lo_server_get_url(server());
    if (not_nullptr(u))
    {
        result = std::string(u);
        free(u);
    }
    return result;
}

/**
 *  This function in endpoint needs to be fixed.
 *
 *  These messages are provided in the message structure lookup list
 * s_signal_msgs in the nsmmessagesex module. Example: nsm::tag::hello.
 *
 * Definitions from the seq66-based nsm subdirectories:
 *
 * nsmbase:
 *
 *      -   error. Added by default.
 *      -   reply. Added by default.
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
 *      is used instead of c_str(). The documentation for
 *      lo_server_new_with_proto() states that If using UDP, then NULL
 *      may be passed to find an unused port. Otherwise a decimal port
 *      number or service name or may be passed. If using UNIX domain
 *      sockets then a socket path should be passed here. An example
 *      is "osc.udp://mlsleno:11086/". The default value is "".
 *
 * \param usethis
 *      If true, the "this" pointer of this object is used in adding
 *      method callbacks. Otherwise, the null pointer is used.
 *
 * \return
 *      Returns true if successful, and false if it fails.
 */

bool
lowrapper::init
(
    int proto,
    const std::string & portname,
    bool usethis
)
{
    const char * portptr = CPTR(portname);              /* null or c_str()  */
    util::info_message("Creating OSC server", portname);
    m_server = lo_server_new_with_proto                 /* server()         */
    (
        portptr, proto, &lowrapper::error_handler
    );
    if (not_nullptr(server()))
    {
        char * u = lo_server_get_url(server());         /* compare to url() */
        if (not_nullptr(u))
        {
            util::status_message("OSC URL", std::string(u));
            address(lo_address_new_from_url(u));
            free(u);
        }
        if (not_nullptr(address()))
        {
            /*
             *  If init() is called via an endpoint pointer,
             *  endpoint::add_methods() is called. This is generally
             *  desirable; the owner of the endpoint will set up
             *  the "/error" and "/reply" callbacks itself.
             */

            void * userdata = usethis ?
                static_cast<void *>(this) : nullptr ;

            add_methods(userdata);
        }
        else
        {
            util::error_message
            (
                "Error creating OSC server address", portname
            );
            return false;
        }
    }
    else
    {
        util::error_message("Error creating OSC server");
        return false;
    }
    return true;
}

/**
 *  Adds a callback function to be called by the OSC server.
 *  Note that the OPTR() macro can yield null pointer instead of
 *  the c_str().
 *
 * \param t
 *      Provides a pair of values, the msg/path value (e.g. "/error") and
 *      the message data types involved (e.g. "sis").
 *
 * \param f
 */

void
lowrapper::add_osc_method
(
    osc::tag t,
    method_handler f,
    void * userdata
)
{
    std::string msg, pattern;
    if (osc::tag_lookup(t, msg, pattern))
    {
        (void) lo_server_add_method
        (
            server(), OPTR(msg), OPTR(pattern),
            f, userdata
        );
    }
}

/**
 *  A virtual function to add OSC methods.
 *
 * See the discussion for lowrapper::osc_reply() below.
 *
 *      error:      "/error" + "sis"
 *      reply:      "/reply" + "ss"
 *      replyex:    "/reply" + "ssss"
 *
 *  Set up in nsmcontroller.
 *
 *      add_osc_method
 *      (
 *          osc::tag::srvreply, OSC_CALLBACK( reply ), userdata
 *      );
 */

void
lowrapper::add_methods (void * userdata)
{
    add_osc_method(osc::tag::error, &lowrapper::osc_error, userdata);
    add_osc_method(osc::tag::reply, &lowrapper::osc_reply, userdata);
    add_osc_method(osc::tag::replyex, &lowrapper::osc_reply, userdata);
}

/*-------------------------------------------------------------------------
 * Send functions
 *-------------------------------------------------------------------------*/

/**
 *  In all of these send() functions, it is the caller's responsibility
 *  to verify that the lo_address is not null.
 */

int
lowrapper::send
(
    lo_address to, const std::string & path,
    osc_value_list & values
)
{
    lo_message m = lo_message_new();
    for (auto i : values)
    {
        const osc_value * ov = &i;
        switch (ov->type())
        {
            case 'f':
                {
                    const osc_float * vf =
                        static_cast<const osc_float *>(ov);

                    lo_message_add_float(m, vf->value());
                }
                break;

            case 'i':
                {
                    const osc_int * vi
                        = static_cast<const osc_int *>(ov);

                    lo_message_add_int32(m, vi->value());
                }
                break;

            case 's':
                {
                    const osc_string * vs =
                        static_cast<const osc_string *>(ov);

                    lo_message_add_string(m, vs->value_ptr());
                }
                break;

            default:
                util::error_printf("Unknown OSC format: %c", ov->type());
                break;
        }
    }
    lo_bundle b = lo_bundle_new(LO_TT_IMMEDIATE_2);
    lo_bundle_add_message(b, OPTR(path), m);

    int r = lo_send_bundle_from(to, server(), b);

    /*
     * Cleanup should be needed.
     */

    lo_message_free(m);
    lo_bundle_free(b);
    return r;
}

int
lowrapper::send (lo_address to, const std::string & path)
{
    return lo_send_from(to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "");
}

int
lowrapper::send (lo_address to, const std::string & path, int v)
{
    return lo_send_from(to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "i", v);
}

int
lowrapper::send
(
    lo_address to, const std::string & path, long v
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "l", v
    );
}

int
lowrapper::send (lo_address to, const std::string & path, float v)
{
    return lo_send_from(to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "f", v);
}

int
lowrapper::send (lo_address to, const std::string & path, double v)
{
    return lo_send_from(to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "d", v);
}

int
lowrapper::send (lo_address to, const std::string & path, const std::string & v)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "s", CSTR(v)
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, float v2
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "sf", CSTR(v1), v2
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, const std::string & v2
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "ss",
        CSTR(v1), CSTR(v2)
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, const std::string & v2, const std::string & v3
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "sss",
        CSTR(v1), CSTR(v2), CSTR(v3)
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, int v2, int v3, int v4
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "siii",
        CSTR(v1), v2, v3, v4
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, const std::string & v2, int v3, int v4, int v5
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "ssiii",
        CSTR(v1), CSTR(v2), v3, v4, v5
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, const std::string & v2, const std::string & v3,
    int v4, int v5, int v6
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "sssiii",
        CSTR(v1), CSTR(v2), CSTR(v3), v4, v5, v6
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path, const std::string & v1, int v2
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "si", CSTR(v1), v2
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path, int v1, const std::string & v2
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "is", v1, CSTR(v2)
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, int v2, const std::string & v3
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "sis",
        CSTR(v1), v2, CSTR(v3)
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    int v1, const std::string & v2, const std::string & v3,
    const std::string & v4
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "isss",
        v1, CSTR(v2), CSTR(v3), CSTR(v4)
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, int v2, const std::string & v3,
    const std::string & v4, const std::string & v5
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "sisss",
        CSTR(v1), v2, CSTR(v3), CSTR(v4), CSTR(v5)
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, const std::string & v2,
    const std::string & v3, const std::string & v4,
    const std::string & v5
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "sssss",
        CSTR(v1), CSTR(v2), CSTR(v3), CSTR(v4), CSTR(v5)
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, const std::string & v2,
    const std::string & v3, const std::string & v4
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "ssss",
        CSTR(v1), CSTR(v2), CSTR(v3), CSTR(v4)
    );
}

int
lowrapper::send (lo_address to, const std::string & path, lo_message msg)
{
    return lo_send_message_from(to, server(), OPTR(path), msg);
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, const std::string & v2,
    int v3, float v4, float v5, float v6
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "ssifff",
        CSTR(v1), CSTR(v2), v3, v4, v5, v6
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, const std::string & v2,
    const std::string & v3, int v4, float v5, float v6, float v7
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "sssifff",
        CSTR(v1), CSTR(v2), CSTR(v3), v4, v5, v6, v7
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, const std::string & v2,
    const std::string & v3, float v4, float v5, float v6
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "sssfff",
        CSTR(v1), CSTR(v2), CSTR(v3), v4, v5, v6
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, int v2, int v3
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "sii",
        CSTR(v1), v2, v3
    );
}

int
lowrapper::send (lo_address to, const std::string & path, int v1, int v2)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "ii", v1, v2
    );
}

int
lowrapper::send (lo_address to, const std::string & path, int v1, float v2)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "if", v1, v2
    );
}

int
lowrapper::send
(
    lo_address to, const std::string & path,
    const std::string & v1, int v2, int v3, float v4
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "siif",
        CSTR(v1), v2, v3, v4 );
}

int
lowrapper::send
(
    lo_address to, const std::string & path, int v1, int v2, float v3
)
{
    return lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2, OPTR(path), "iif", v1, v2, v3
    );
}

/*-------------------------------------------------------------------------
 * Member functions used by the OSC service.
 *-------------------------------------------------------------------------*/

/*
 *  Static function using C strings. See its setup in the init() function.
 */

void
lowrapper::error_handler
(
    int num, const char * msg, const char * path
)
{
    util::error_printf("OSC server error %d, path %s: %s\n", num, path, msg);
}

/**
 *  Send the error code and error text to our logged address.
 */

void
lowrapper::error_send (const std::string & errmsg, int errcode)
{
    if (not_nullptr_2(address(), server()))
    {
#if defined USE_OLD_CODE
        lo_send_from
        (
            address(), server(), LO_TT_IMMEDIATE_2,
            "/error", "sis",                        /* tag::error   */
            "Error", errcode, CSTR(errmsg)
        );
#else
        (void) send(address(), "/error", "Error", errcode, errmsg);
#endif
    }
}

/*
 *  Get the message source via the msg parameter, and send the error
 *  code and the error text.
 */

void
lowrapper::error_send
(
    lo_message msg,
    const std::string & errmsg, int errcode
)
{
    lo_address to = lo_message_get_source(msg);
#if defined USE_OLD_CODE
    lo_send_from
    (
        to, server(), LO_TT_IMMEDIATE_2,
        "/error", "sis",                            /* tag::error   */
        "Error", errcode, CSTR(errmsg)
    );
#else
        (void) send(to, "/error", "Error", errcode, errmsg);
#endif
}

/**
 *  Send the error code and error text to the provided address.
 */

void
lowrapper::error_send
(
    lo_address to,
    const std::string & errmsg, int errcode
)
{
    if (not_nullptr(to))
    {
#if defined USE_OLD_CODE
        lo_send_from
        (
            to, server(), LO_TT_IMMEDIATE_2,
            "/error", "sis",                        /* tag::error   */
            "Error", errcode, CSTR(errmsg)
        );
#else
        (void) send(to, "/error", "Error", errcode, errmsg);
#endif
    }
}

/**
 *  Send the reply code and reply text to our logged address.
 */

void
lowrapper::reply_send (const std::string & reply)
{
    if (not_nullptr_2(address(), server()))
    {
#if defined USE_OLD_CODE
        lo_send_from
        (
            address(), server(), LO_TT_IMMEDIATE_2,
            "/reply", "ss", "Reply", CSTR(reply)    /* tag::reply   */
        );
#else
        (void) send(address(), "/reply", "Reply", reply);
#endif
    }
}

/*
 *  Get the message source via the msg parameter, and send the reply
 *  code and the reply text.
 */

void
lowrapper::reply_send (lo_message msg, const std::string & reply)
{
    lo_address to = lo_message_get_source(msg);
    reply_send(to, reply);
}

/**
 *  Send the reply code and reply text to the provided address.
 */

void
lowrapper::reply_send (lo_address to, const std::string & reply)
{
    if (not_nullptr_2(to, server()))
    {
#if defined USE_OLD_CODE
        lo_send_from
        (
            to, server(), LO_TT_IMMEDIATE_2,
            "/reply", "ss", "Reply", CSTR(reply)    /* tag::reply   */
        );
#else
        (void) send(to, "/reply", "Reply", reply);
#endif
    }
}

/**
 *  Virtual function to handle "/error" + "sis" [osc::tag::error]
 *  Unlike "/reply", there is only one type-spec to worry about.
 *  (There is proxy-error, though.)
 */

bool
lowrapper::handle_error
(
    const std::string & err_path,
    int err_code,
    const std::string & err_message
)
{
    util::warn_printf
    (
        "Client error: %s; error %i (%s)",
        V(err_path), err_code, V(err_message)
    );
    return err_code != nsm::error::ok;
}

/**
 *  Virtual function to handle the various "/reply" types. See
 *  lowrapper::osc_reply(). Up to 4 strings can be supplied by
 *  the tokenization string vector. A null pointer is represented
 *  by the string "NIL", defined in the messages header file.
 *
 *  This is for ordinary NSM clients, not controllers like
 *  nsm-legacy-gui or nsmctl66. See the guisrvannounce handling in
 *  nsmcontroller::osc_handler().
 *
 * \param args
 *      Provides any arguments passed by OSC. The size of this vector
 *      of strings is the argument count.
 *
 * \param types
 *      Provides the type-specification; there are 4 varieties. See
 *      lowrapper::osc_reply().
 *
 * \param msg
 *      This lo_message parameters is often used to get addresses
 *      via lo_message_get_source() for looking up peers.
 */

bool
lowrapper::handle_reply
(
    lib66::tokenization & args,
    const std::string & /* types */,
    lo_message /* msg */,
    void * /* userdata */
)
{
    bool result = true;
    std::size_t sz = args.size();
    if (sz == 1)
    {
        /*
         * /signal/list handled by the endpoint class.
         */

        util::warn_message("NULL reply in lowrapper, not endpoint");
        result = false;
    }
    else if (sz == 2 || sz == 4)
    {
        std::string replypath { args[0] };
        std::string replymsg { args[1] };
        if (replypath == std::string("/nsm/server/announce"))
        {
            util::status_message("Successfully registered", replymsg);
            if (sz == 4)
            {
                util::status_message("NSM name", args[2]);
                util::status_message("Capabilities", args[3]);
            }
        }
        else
        {
            util::info_printf
            (
                "Client reply: %s; name %s (not yet handled)",
                V(replypath), V(replymsg)
            );
        }
    }
    else
    {
        util::error_message("Unsupported reply encountered");
        result = false;
    }
    return result;
}

/*-------------------------------------------------------------------------
 * Basic OSC handlers. Static member functions.
 *-------------------------------------------------------------------------*/

int
lowrapper::osc_error        // see jackpatch's osc_announce_error()
(
    const char * path, const char * types, lo_arg ** argv,
    int argc, lo_message msg, void * userdata
)
{
    (void) path; (void) msg; (void) userdata;
    if (std::string(types) != "sis")
    {
        util::error_message("Error types received is not 'sis'");
        return osc_msg_unhandled();
    }
    if (argc >= 3)
    {
        std::string pathmsg { string_from_lo_arg(argv[0]) };
        std::string message { string_from_lo_arg(argv[2]) };
        lowrapper * low { nullptr };
        if (not_nullptr(userdata))
            low = lowrapper_const_cast(userdata);

        if (pathmsg == std::string("/nsm/server/announce"))
        {
            util::error_message("Failed to register with NSM", message);
            if (not_nullptr(low))
                low->active(false);
        }
        if (not_nullptr(low))
            (void) low->handle_error(pathmsg, argv[1]->i, message);

        return osc_msg_handled();
    }
    else
        return osc_msg_unhandled();
}

/**
 *  This function can be used for announce replies or for other replies.
 *  We might consider virtual functions for the type of reply.
 *
 *  There are 4 types of replies:
 *
 *      -   NULL (NIL)  [osc::tag::sigreply]
 *
 *          Handled in lowrapper::osc_reply for message "/signal/list".
 *          User-data is "this".
 *
 *      -   "s"         [osc::tag::srvreply]
 *
 *          Used in nsm-legacy-gui (and hence in nsmctl's nsmcontroller
 *          object) to accept a reply to "/nsm/server/list" with one or
 *          more session names, with "" being the last name sent.
 *
 *      -   "ss"        [osc::tag::reply]
 *
 *          Used in nsmd to receive a client's reply to a control message
 *          sent to it. One such client is nsm-legacy-gui, which uses the
 *          strings to log the status. The strings are:
 *
 *              -   The message to which the client is replying.
 *              -   A textual message.
 *
 *      -   "ssss"      [osc::tag::replyex]
 *
 *          Jackpatch and nsm-proxy (and other NSM client applications)
 *          add a handler for this message, which comes in reply to
 *          a client's announce message. The strings are:
 *
 *              -   "/nsm/server/announce"
 *              -   A hello message (e.g. "hi" or "Acknowledged...").
 *              -   The name of the session manager (APP_TITLE).
 *              -   Its capability string.
 */

int
lowrapper::osc_reply
(
    const char * path, const char * types, lo_arg ** argv,
    int argc, lo_message msg, void * userdata
)
{
    lib66::tokenization args;
    int result { osc_msg_unhandled() };
    osc_msg_summary
    (
        "lowrapper::osc_reply", path, types, argv, argc, userdata
    );
    if (argc == 0)                          // is NULL bereft of arguments?
    {
        args.push_back(NIL);
    }
    else if (argc >= 1)
    {
        args.push_back(&argv[0]->s);
        if (argc >= 2)
        {
            args.push_back(&argv[1]->s);
            if (argc >= 3)
            {
                args.push_back(&argv[2]->s);
                if (argc >= 4)
                {
                    args.push_back(&argv[3]->s);
                }
            }
        }
    }
    if (not_nullptr(userdata) && ! args.empty())
    {
        lowrapper * low = lowrapper_const_cast(userdata);
        if (low->handle_reply(args, types, msg, userdata))
        {
            low->active(true);
            result = osc_msg_handled();
        }
    }
    return result;
}

/*--------------------------------------------------------------------------
 * Free functions
 *--------------------------------------------------------------------------*/

/**
 *  Extracts the port number, as a string, from the port name, such as
 *  "osc.udp://mlsleno:17439/". Useful when using the --lookup option
 *  rather than the --osc-port option.
 */

std::string
extract_port_number (const std::string & portspec)
{
    std::string result;
    if (! portspec.empty())
    {
        auto pos0 = portspec.find_first_of("0123456789");
        if (pos0 != std::string::npos)
        {
            auto pos1 = portspec.find_last_of("0123456789");
            result = portspec.substr(pos0, pos1 - pos0 + 1);
        }
    }
    return result;
}

/**
 *  Provides a brief description of an incoming message in a callback.
 *  Requires the -i / --investigate option to be used.
 */

void
osc_msg_summary
(
    const std::string & funcname,
    const std::string & path, const char * types,
    lo_arg ** argv, int argc, void * userdata
)
{
    if (util::investigate())
    {
        std::string typefix = not_nullptr(types) ? types : "NULL" ;
        if (util::investigate())
        {
            util::debug_printf
            (
                "%s(\"%s\"+\"%s\", args %d, user %p)",
                V(funcname), V(path), V(typefix), argc, userdata
            );
        }
        else
        {
            util::debug_printf
            (
                "%s(\"%s\"+\"%s\", args %d)",
                V(funcname), V(path), V(typefix), argc
            );
        }
        if (argc > 0)
        {
            printf("   ");
            for (int i = 0; i < argc; ++i)
            {
                std::string value;
                if (types[i] == 's')
                    value = string_from_lo_arg(argv[i]);
                else if (types[i] == 'i')
                    value = std::to_string(argv[i]->i);
                else if (types[i] == 'f')
                    value = std::to_string(argv[i]->f);
                else
                    value = "#";

                printf("[%d] \"%s\"; ", i, CSTR(value));
            }
            printf("\n");
        }
    }
}

/**
 *  A helper function with common code for an application to announce
 *  itself to NSM.
 */

void
process_announce
(
    lo_server srvr,
    const std::string & caps,
    const std::string & nsm_url,
    const std::string & client_name,
    const std::string & process_name
)
{
    lo_address to = lo_address_new_from_url(CSTR(nsm_url) );
    int pid = int(getpid());
    util::info_message("Announcing to NSM");
    lo_send_from
    (
        to, srvr, LO_TT_IMMEDIATE_2,
        "/nsm/server/announce", "sssiii",       /* osc::tag::srvannounce    */
        CSTR(client_name),
        CSTR(caps),                             /* ":optional-gui:"         */
        CSTR(process_name),
        NSM_API_VERSION_MAJOR,                  /* 1 api_major_version      */
        NSM_API_VERSION_MINOR,                  /* 0 api_minor_version      */
        pid
    );
    lo_address_free(to);
}

}           // namespace osc

/*
 * lowrapper.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


