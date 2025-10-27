#if ! defined NSM66_OSC_MESSAGES_HPP
#define NSM66_OSC_MESSAGES_HPP

/**
 * \file          messages.hpp
 *
 *    This module provides a kind of repository of all the possible OSC/NSM
 *    messages.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom
 * \date          2025-02-12
 * \updates       2025-04-23
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Upcoming support for the Non Session Manager.  This module provides a list
 *  of OSC paths (messages) for various purposes, as a way to keep track of
 *  them all and use them propertly.
 */

#include <map>                          /* std::map dictionary class        */
#include <string>                       /* std::string class                */

#include "cpp_types.hpp"                /* lib66::tokenization vector       */

/*
 *  The osc namespace is a place to squirrel away concepts that are
 *  not defined in the other nsm classes.
 */

namespace osc
{

/**
 *  A character string to be converted to a null pointer. Although sometimes
 *  empty strings are passed to OSC, there are many cases where NULL
 *  is passed. Does this matter to OSC? Cannot find documentation on
 *  it so far in either OSC or NSM documentation.
 */

const std::string NIL { "-" };

inline
const char * osc_message_ptr (const std::string & s)
{
    return s == NIL ? NULL : s.c_str() ;    /* CPTR(s) vs NULL              */
}

/**
 *  While CSTR() always returns a non-null pointer, .c_str(), there are
 *  a few messages that use a NULL instead. Use this macro in those cases
 *  to get NULL (the NIL value above) versus an empty string.
 */

#define OPTR(x)  osc::osc_message_ptr(x)    /* for brevity in various calls */

/**
 * This sections logs all the name changes to keep track of.
 */

#if 0

    addstrip (removed, it is for non)
    announce (removed in favor of existing gui_announce)
    abort           --> srvabort
    add             --> srvadd
    arguments       --> proxyargs
    broadcast       --> srvbroadcast
    clienterror     --> proxyerror
    close           --> srvclose
    configfile      --> proxycfgfile
    clean           --> cliclean
    dirty           --> clidirty
    disconnect (removed in favor of existing sigdisconnect)
    duplicate       --> srvduplicate
    executable      --> proxyexe
    hide            --> clihide
    kill            --> proxykill
    label           --> clilabel
    loaded          --> cliloaded (session is loaded)
    message         --> climessage
    name            --> sessionname
    open            --> cliopen
    optional        --> guioption
    ping            --> oscping
    progress        --> cliprogress
    quit            --> srvquit
    remove          --> guiremove
    resume          --> guiresume
    root            --> sessionroot
    save            --> clisave
    show            --> guishow
    showopt         --> clishow
    sigsave         --> proxysave
    srvreply        --> sigreply
    status          --> guistatus
    start           --> proxystart
    stop            --> guistop
    stopsignal      --> proxystop
    switchrc        --> guiswitch
    update          --> proxyupdate
    visible         --> guivisible

Note that there a multiple variations of the "error" and "reply" messages.

#endif

/**
 *  The nsm::tag enumeration is used in the lookup of the long
 *  strings that are sent and received by NSM.  We can use these tags to
 *  look up both the long name and the OSC formatting string to be used in
 *  a message.
 *
 *  Also note the value "illegal", useful as an error code.
 *
 *  Removed:
 *
 *      announce,
 *      disconnect,
 */

enum class tag
{
    announce,           // see nsmd
    cliclean,           // client
    clidirty,           // client, gui/client
    clihide,            // client, gui/client
    clilabel,           // client, gui/client, proxy
    cliloaded,          // client
    climessage,         // client, gui/client, gui/server
    cliopen,            // client, server
    cliprogress,        // client, gui/client
    clisave,            // client, gui/client, server
    clishow,            // client, gui/client
    ctlannounce,        // weird, in controller, gui/server/annouce
    error,              // used by many
    generic,            // signal
    guiannounce,        // nsmd
    gui_announce,       // nsm-legacy-gui
    guidirty,           // client, gui/client
    guihidden,          // client
    guihide,            // client, gui/client
    guilabel,           // client, gui/client, proxy
    guimessage,         // client, gui/client, gui/server
    guinew,             // gui/client
    guioption,          // gui/client, has_optional_gui
    guiprogress,        // client, gui/client
    guiremove,          // gui/client
    guiresume,          // gui/client
    guisave,            // client, gui/client, server
    guisession,         // gui/session
    guisessionname,     // gui/session, session
    guishow,            // client, gui/client
    guishown,           // client
    guisrvannounce,     // seems redundant...
    guistatus,          // gui/client
    guistop,            // gui/client
    guiswitch,          // gui/client
    guivisible,         // gui/client
    nonaddstrip,        // non
    nonhello,           // signal ?????????????
    null,               // client, all items null
    oscping,            // used by manu
    oscreply,           // osc, non
    proxyargs,          // proxy
    proxycfgfile,       // proxy
    proxyerror,         // proxy
    proxyexe,           // proxy
    proxykill,          // proxy
    proxylabel,         // client, gui/client, proxy
    proxysave,          // proxy
    proxystart,         // proxy
    proxystop,          // prox
    proxyupdate,        // proxy
    reply,              // used by many, signal has no args
    replyex,            // another variation
    sessionlist,        // server, session, signal
    sessionname,        // gui/session, session
    sessionroot,        // gui/session
    sigconnect,         // signal
    sigcreated,         // signal
    sigdisconnect,      // signal
    sighello,           // signal
    siglist,            // server, session, signal
    sigremoved,         // signal
    sigrenamed,         // signal
    sigreply,           // another variation
    srvabort,           // server
    srvadd,             // server
    srvannounce,        // server only
    srvbroadcast,       // server
    srvclose,           // server
    srvduplicate,       // server
    srvlist,            // server, session, signal
    srvmessage,         // client, gui/client, gui/server
    srvnew,             // server, was called "new" (C++ keyword)
    srvopen,            // client, server
    srvquit,            // server
    srvreply,           // another variation, see the nsmctl app
    srvsave,            // client, gui/client, server
    stripbynumber,      // non
    illegal             // indicates some kind of lookup error
};

/**
 *  This type holds the long OSC string for the message, and the data
 *  pattern string that describes the data being sent.
 */

using messagepair = struct
{
    std::string msg_text;
    std::string msg_pattern;
};

/**
 *  A lookup map for tags and message pairs.
 */

using lookup = std::map<tag, messagepair>;

/**
 *  A lookup vector for reducing repetition of osc::tag definitions.
 */

using taglist = std::vector<tag>;

/*
 *  Free functions for inverse table lookup.
 */

extern const lookup & all_messages ();
extern bool tag_lookup
(
    const lookup & table,
    tag t, std::string & message, std::string & pattern
);
extern bool tag_lookup
(
    tag t, std::string & message, std::string & pattern
);
extern bool tag_lookup
(
    const taglist & tl,
    tag t, std::string & message, std::string & pattern
);
extern const std::string & tag_message (tag t);
extern tag tag_reverse_lookup
(
    const lookup & table,
    const std::string & message,
    const std::string & pattern = "?"
);
extern tag tag_reverse_lookup
(
    const std::string & message,
    const std::string & pattern = "?"
);

/**
 *  This type is used for lookup up some of the tags so that the user
 *  can type the command and have it translated to a particular tag.
 *  The is_client_tag field indicates if the command is meant for
 *  a particular NSM client, or for the NSM "server".
 */

using tagspec = struct
{
    bool is_client_tag;
    tag osc_tag;
};

/**
 *  A lookup map for human-readable tag names and the osc::tag value.
 *  See message.cpp for the tags supplied; we don't need all of them.
 */

using tagmap = std::map<std::string, tagspec>;

/*
 *  Free function to convert a string to a client/server tag name.
 */

extern tag tag_name_lookup (const std::string & name);
extern bool tag_name_is_client (const std::string & name);
extern bool tag_needs_argument (const std::string & name);
extern void tag_name_action_list (lib66::tokenization & actions);

}           // namespace osc

#endif      // NSM66_OSC_MESSAGES_HPP

/*
 * messages.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

