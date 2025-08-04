#if ! defined NSM66_OSC_LOWRAPPER_HPP
#define NSM66_OSC_LOWRAPPER_HPP

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
 * \file          lowrapper.hpp
 *
 *    This module is just a small wrapper to improve access to OSC
 *    operations in jackpatch66 and serve as a base class for osc::endpoint
 *    used in nsm66d.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-02-26
 * \updates       2025-04-24
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   It defines some elements similar to the endpoint class, but is
 *   much lighter and provides only some basics. It does not support
 *   the specific protocol that endpoint supports. No threads, no
 *   signals, no methods, no translations, no peers, just
 *   commonly-used OSC accessors.
 *
 *   It is meant to help with the work of jackpatch66 and nsm-proxy66.
 *
 *   There is also an lo_cpp.h file in the OSC library. But it is a
 *   freaking huge header-only library, too much for us at this time.
 *
 * Summary:
 *
 *      -   NSM_API_VERSION macros
 *      -   LO_TT_IMMEDIATE_2 to adapt LO_TT_IMMEDIATE to the Clang compiler.
 *      -   osc::method_handler function alias.
 *      -   lo_server and lo_address members and accessors.
 *      -   Functions:
 *          -   To handle and send error and reply messages.
 *          -   Get the OSC port and server URL.
 *          -   Virtual functions for extensibility.
 *          -   send() functions for a number of type-specifications
 *              (e.g. "sis" for "/error" messages).
 */

#include <map>
#include <string>

#include "cpp_types.hpp"                /* lib66::tokenization              */
#include "platform_macros.h"            /* PLATFORM_CLANG                   */
#include "nsm/nsmmessagesex.hpp"        /* nsm66::nsm new message functions */
#include "osc/messages.hpp"             /* osc::tag, etc.                   */
#include "osc/osc_value.hpp"            /* osc::osc_value_list              */

#include "nsm66-config.h"               /* feature (HAVE) macros            */
#if defined NSM66_HAVE_LO_H
#include <lo/lo.h>                      /* library for the OSC protocol     */
#else
#error Support for liblo required for this class, install liblo-dev
#endif

/**
 *  See the definitions in the new-session-manager version of nsmd.
 */

#define NSM_API_VERSION_MAJOR   1
#define NSM_API_VERSION_MINOR   1
#define NSM_API_VERSION_PATCH   2
#define NSM_API_VERSION         "1.1.2"

/*
 * CLANG and LO_TT_IMMEDIATE:
 *
 *      Warning: compound literals are a C99-specific feature [-Wc99-extensions]
 *      This is due to using, in lo_osc_types.h:
 *
 *      #define LO_TT_IMMEDIATE ((lo_timetag){0U,1U})
 *
 *      versus "lo_timetag lo_get_tt_immediate();" as LO_TT_IMMEDIATE
 */

#if defined PLATFORM_CLANG              /* __clang__                        */

#include "lo/lo_osc_types.h"            /* defines lo_timetag structure     */

#define LO_TT_IMMEDIATE_2   get_lo_timetag()
#else
#define LO_TT_IMMEDIATE_2   LO_TT_IMMEDIATE
#endif                                  /* end __clang__                    */

extern lo_timetag & get_lo_timetag();   /* defined in lowrapper.cpp         */

/*
 * Helper macros for defining OSC handlers. These are for free functions
 * that are not static. Note that static member functions can be used;
 * see lowrapper::add_methods().
 */

#define OSC_NAME(name)      osc_ ## name
#define OSC_HANDLER(name)   int OSC_NAME(name) \
 ( \
    const char * path, const char * types, lo_arg ** argv, \
    int argc, lo_message msg, void * user_data \
 )

namespace osc
{

/**
 *  This is the library's version of lo_method_handler defined in
 *  lo_types.h in the liblo library. It has essentially the
 *  same function signature.
 *
 *  This is for client code to use, we still use lo_method_handler internally.
 *
 *  Also see osc::method_handler in the endpoint module.
 *
 * Notes on lo_method_handler:
 *
 *     A callback function to receive notification of matching message arriving
 *     in the server or server thread.  The return value tells the method
 *     dispatcher whether this handler has dealt with the message correctly: a
 *     return value of 0 indicates that it has been handled, and it should not
 *     attempt to pass it on to any other handlers, non-0 means that it has not
 *     been handled and the dispatcher will attempt to find more handlers that
 *     match the path and types of the incoming message.
 *
 * path	    The path that the incoming message was sent to.
 * types	If you specided types in your method creation call, then this will
 *          match those and the incoming types will have been coerced to match,
 *          otherwise it will be the types of the arguments of the incoming
 *          message.
 * argv	    An array of lo_arg types containing the values, e.g. if the first
 *          argument of the incoming message is of type 'f', then the value
 *          will be found in argv[0]->f.
 * argc	    The number of arguments received.
 * msg	    A structure containing the original raw message as received. No
 *          type coercion occured, and the data will be in OSC byte order
 *          (bigendian).
 * userdata	Contains the user data value passed in the call to
 *          lo_server_thread_add_method().
 */

using method_handler = int (*)
(
    const char * path, const char * types,
    lo_arg ** argv, int argc, lo_message msg,
    void * userdata
);

/**
 *  Provides functionality that is useful in the most common operations in
 *  the NSM.
 */

class lowrapper
{

private:

    /**
     *  lo_server is defined in lo_types.h; it is a "struct lo_server_ *",
     *  a pointer to an incomplete structure type. It is defined in
     *  the source code, src/lo_type_internal.h. It has many items
     *  similar to _lo_address.
     *
     *  The weird thing is that lo_server_ is used, but only _lo_server
     *  is defined. Similar for the address.
     *
     *  It represents an instance of an OSC server.
     */

    lo_server m_server;

    /**
     *  lo_address is defined in lo_types.h; it is a "struct lo_address_ *",
     *  a pointer to an incomplete structure type. This big structure
     *  specifies the host name, socket items, port name, protocol, flags,
     *  error, etc. It also contains the structure discussed above.
     *
     *  It is a reference to an OSC service.
     */

    lo_address m_address;

    /**
     *  Our name.
     */

    std::string m_port_name;

    /**
     *  If true, we have gotten an announce reply and can be considered
     *  active.
     */

    bool m_active;

public:

    lowrapper ();
    virtual ~lowrapper ();

public:         /* virtual functions    */

    virtual bool handle_error
    (
        const std::string & err_path,
        int err_code,
        const std::string & err_message
    );
    virtual bool handle_reply
    (
        lib66::tokenization & args,
        const std::string & types,
        lo_message msg,
        void * userdata
    );
    virtual bool init
    (
        int proto,
        const std::string & portname    = "",
        bool usethis                    = false
    );

public:

    void add_osc_method
    (
        osc::tag t,
        method_handler f,
        void * userdata = nullptr
    );
    void error_send (const std::string & errmsg, int errcode);
    void error_send
    (
        lo_message msg,
        const std::string & errmsg, int errcode
    );
    void error_send
    (
        lo_address to,
        const std::string & errmsg, int errcode
    );
    void reply_send (const std::string & reply);
    void reply_send
    (
        lo_message msg,
        const std::string & reply
    );
    void reply_send (lo_address to, const std::string & reply);

    bool active () const
    {
        return m_active;
    }

    void active (bool f)
    {
        m_active = f;
    }

public:         /* send() functions */

    /*
     * Overloads for common message formats. Same order in hpp & cpp file.
     */

    int send    /* "f", "i", "s"    */
    (
        lo_address to, const std::string & path,
        osc_value_list & values
    );
    int send    /* ""   */
    (
        lo_address to, const std::string & path
    );
    int send    /* "i"  */
    (
        lo_address to, const std::string & path, int v
    );
    int send    /* "f"  */
    (
        lo_address to, const std::string & path, float v
    );
    int send    /* "d"  */
    (
        lo_address to, const std::string & path, double v
    );
    int send    /* "s"  */
    (
        lo_address to, const std::string & path, const std::string & v
    );
    int send    /* "sf" */
    (
        lo_address to, const std::string & path,
        const std::string & v1, float v2
    );
    int send    /* "ss" */
    (
        lo_address to, const std::string & path,
        const std::string & v1, const std::string & v2
    );
    int send    /* "sss"    */
    (
        lo_address to, const std::string & path,
        const std::string & v1, const std::string & v2,
        const std::string & v3
    );
    int send    /* "siii"   */
    (
        lo_address to, const std::string & path,
        const std::string & v1, int v2, int v3, int v4
    );
    int send    /* "ssiii"  */
    (
        lo_address to, const std::string & path,
        const std::string & v1, const std::string & v2,
        int v3, int v4, int v5
    );
    int send    /* "sssiii" */
    (
        lo_address to, const std::string & path,
        const std::string & v1, const std::string & v2,
        const std::string & v3, int v4, int v5, int v6
    );
    int send    /* "si" */
    (
        lo_address to, const std::string & path,
        const std::string & v1, int v2
    );
    int send    /* "is" */
    (
        lo_address to, const std::string & path,
        int v1, const std::string & v2
    );
    int send    /* "sis"    */
    (
        lo_address to, const std::string & path,
        const std::string & v1, int v2, const std::string & v3
    );
    int send    /* "isss"   */
    (
        lo_address to, const std::string & path,
        int v1, const std::string & v2, const std::string & v3,
        const std::string & v4
    );
    int send    /* "sisss"  */
    (
        lo_address to, const std::string & path,
        const std::string & v1, int v2, const std::string & v3,
        const std::string & v4, const std::string & v5
    );
    int send    /* "sssss"  */
    (
        lo_address to, const std::string & path,
        const std::string & v1, const std::string & v2,
        const std::string & v3, const std::string & v4,
        const std::string & v5
    );
    int send    /* "ssss"   */
    (
        lo_address to, const std::string & path,
        const std::string & v1, const std::string & v2,
        const std::string & v3, const std::string & v4
    );
    int send    /* lo msg   */
    (
        lo_address to, const std::string & path, lo_message msg
    );
    int send    /* "ssifff" */
    (
        lo_address to, const std::string & path,
        const std::string & v1, const std::string & v2,
        int v3, float v4, float v5, float v6
    );
    int send    /* "sssifff"    */
    (
        lo_address to, const std::string & path,
        const std::string & v1, const std::string & v2,
        const std::string & v3, int v4, float v5, float v6, float v7
    );
    int send    /* "sssfff"     */
    (
        lo_address to, const std::string & path,
        const std::string & v1, const std::string & v2,
        const std::string & v3, float v4, float v5, float v6
    );
    int send    /* "sii"    */
    (
        lo_address to, const std::string & path,
        const std::string & v1, int v2, int v3
    );
    int send    /* "ii" */
    (
        lo_address to, const std::string & path, int v1, int v2
    );
    int send    /* "if"  */
    (
        lo_address to, const std::string & path, int v1, float v2
    );
    int send    /* "siif"   */
    (
        lo_address to, const std::string & path,
        const std::string & v1, int v2, int v3, float v4
    );
    int send    /* "iif"    */
    (
        lo_address to, const std::string & path,
        int v1, int v2, float v3
    );
    int send    /* "l"  */          /* supported? */
    (
        lo_address to, const std::string & path, long v
    );

protected:      /* virtual functions    */

    virtual void add_methods (void * userdata);

protected:

    /*
     * Note that these objects are pointers to opaque structures.
     */

    lo_server server () const
    {
        return m_server;
    }

    lo_server server ()
    {
        return m_server;
    }

    void server (lo_server losrv)
    {
        m_server = losrv;
    }

    lo_address address () const
    {
        return m_address;
    }

    lo_address address ()
    {
        return m_address;
    }

    void address (lo_address loaddr)
    {
        m_address = loaddr;
    }

public:

    int port () const
    {
        return lo_server_get_port(m_server);
    }

    std::string url () const;

    void port_name (const std::string & name)
    {
        m_port_name = name;
    }

    const std::string & port_name ()
    {
        return m_port_name;
    }

protected:

    lo_address service_address ()
    {
        return m_address;
    }

    static void error_handler
    (
        int num, const char * msg, const char * path
    );

protected:

    static int osc_error
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );
    static int osc_reply
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );

};          // class lowrapper

/*--------------------------------------------------------------------------
 * Free functions
 *--------------------------------------------------------------------------*/

extern std::string extract_port_number (const std::string & portspec);
extern void osc_msg_summary
(
    const std::string & funcname,
    const std::string & path, const char * types,
    lo_arg ** argv, int argc, void * userdata
);
extern void process_announce
(
    lo_server srvr,
    const std::string & caps,
    const std::string & nsm_url,
    const std::string & client_name,
    const std::string & process_name
);

/**
 *  Tricky: argv[0] is a pointer to a lo_arg, a union of various simple data
 *  types like float and char. The s value's data type is "char", but it is
 *  also used for holding a null-terminated string. We need the address of
 *  this character and pretend it is a character pointer. The argv[i] value
 *  is a pointer to one of the elements in argv[].
 */

inline std::string
string_from_lo_arg (const lo_arg * arg)
{
    return std::string(&arg->s);
}

inline int
osc_msg_handled ()
{
    return 0;
}

inline int
osc_msg_unhandled ()
{
    return (-1);
}

}           // namespace osc

#endif      // defined NSM66_OSC_LOWRAPPER_HPP

/*
 * lowrapper.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

