/**
 * \file          messages.cpp
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
 *
 *  This file is a lean version of the nsm namespace's module nsmmessagesex, used by
 *  the applications, not the "nsm" library from Seq66.
 */

#include <algorithm>                    /* std::find() for vector range     */
#include <iomanip>                      /* std::setw() manipulator          */
#include <sstream>                      /* std::stringstream                */

#include "osc/messages.hpp"             /* osc::tag, etc.                   */

namespace osc
{

/**
 *  This map of message/pattern pairs provides all the messages and patterns
 *  used in the "/nsm/client/xxxxx" series of messages, including client
 *  variations of "/error" and "/reply".
 *
 *  Also see the static values in the nsm/nsmmessageex.cpp module.
 *
 *  Examine the message/pattern (path/typespec) table in
 *  extras/notes/add_method-calls.text.
 *
 * /reply
 *
 *      Looking at nsmd, there are these varieties of replies:
 *
 *          "s"     This string value is a simple message, such as
 *                  "Client stopped." It can also be a tag message, as
 *                  in the case of a ping response.
 *
 *          "ss"    osc::tag::reply. The first string is the tag message
 *                  being replied to, such as "/nsm/server/announce".
 *                  The second string is a comment about the response,
 *                  such as "Launched".
 *
 *          "ssss"  osc::tag::replyex. This one is added in jackpatch and
 *                  nsm-proxy, but there seems to be no handler for this
 *                  message in either application. These are the two
 *                  applications that don't use the osc::endpoint class.
 *
 * /error
 *
 *      There is only one variety of error response:
 *
 *          "sis"   osc::tag::error: The first string is the tag message
 *                  being replied to, such as "/nsm/server/announce". The
 *                  integer is a error code from the int-compatible
 *                  enumeration nsm::error. The last string is an
 *                  informative message.
 *
 * Flag messages:
 *
 *      Messages like "/nsm/client/save" + "" are simply flags to indicate
 *      either a state or a simple command.
 *
 * Optional-GUI:
 *
 *      The "optional-gui" + "s" takes a client ID, but the typespec
 *      is "" in nsm-proxy, so "cli" versions of that are provided.
 *
 * /nsm/gui/gui_announce and /nsm/gui/server_announce:
 *
 *      nsm-legacy-gui: in announce(), sends gui_announce (tag::announce)
 *      with no parameters. In osc_handler(), a gui_announce reply means
 *      that a pre-existing server is replying. A server_announce
 *      indicates a server "we" launched.
 *
 *      nsmd: osc_gui_announce() gets the incoming URL from the message
 *      as a reply, and announce_gui() sends the gui_announce greeting
 *      message, "hi". In main(), if a GUI URL is provided (a GUI
 *      client spawned nsmd), then announce_gui() sends the
 *      server_announce greeting message, "hi".
 *
 * Note:
 *
 *  We're changing some names. See the #if 0/#endif section near the top
 *  of include/osc/messages.hpp. This sections logs all the name changes
 *  to keep track of.
 */

const lookup &
all_messages ()
{
    static lookup s_all_messages
    {
        { tag::announce,       { "/nsm/gui/gui_announce",             ""        } },
        { tag::cliclean,       { "/nsm/client/is_clean",              ""        } },
        { tag::clidirty,       { "/nsm/client/is_dirty",              ""        } },
        { tag::clihide,        { "/nsm/client/hide_optional_gui",     ""        } },
        { tag::clilabel,       { "/nsm/client/label",                 "s"       } },
        { tag::cliloaded,      { "/nsm/client/session_is_loaded",     ""        } },
        { tag::climessage,     { "/nsm/client/message",               "is"      } },
        { tag::cliopen,        { "/nsm/client/open",                  "sss"     } },
        { tag::cliprogress,    { "/nsm/client/progress",              "f"       } },
        { tag::clisave,        { "/nsm/client/save",                  ""        } },
        { tag::clishow,        { "/nsm/client/show_optional_gui",     ""        } },
        { tag::ctlannounce,    { "/nsm/gui/server/announce",          "s"       } },
        { tag::error,          { "/error",                            "sis"     } },
        { tag::generic,        { NIL,                                 NIL       } },

        /*
         * The original nsmd sends "/nsm/gui/gui_announce" + "hi, but adds a
         * method expecting that path + "". The nsm-legacy-gui adds that
         * path's method, but with "s".
         */

        { tag::guiannounce,    { "/nsm/gui/gui_announce",             ""        } },
        { tag::gui_announce,   { "/nsm/gui/gui_announce",             "s"       } },
        { tag::guidirty,       { "/nsm/gui/client/dirty",             "si"      } },
        { tag::guihidden,      { "/nsm/client/gui_is_hidden",         ""        } },
        { tag::guihide,        { "/nsm/gui/client/hide_optional_gui", "s"       } },
        { tag::guilabel,       { "/nsm/gui/client/label",             "ss"      } },
        { tag::guimessage,     { "/nsm/gui/client/message",           "s"       } },
        { tag::guinew,         { "/nsm/gui/client/new",               "ss"      } },
        { tag::guioption,      { "/nsm/gui/client/has_optional_gui",  "s"       } },
        { tag::guiprogress,    { "/nsm/gui/client/progress",          "sf"      } },
        { tag::guiremove,      { "/nsm/gui/client/remove",            "s"       } },
        { tag::guiresume,      { "/nsm/gui/client/resume",            "s"       } },
        { tag::guisave,        { "/nsm/gui/client/save",              "s"       } },
        { tag::guisession,     { "/nsm/gui/session/session",          "s"       } },
        { tag::guisessionname, { "/nsm/gui/session/name",            "ss"       } },
        { tag::guishow,        { "/nsm/gui/client/show_optional_gui", "s"       } },
        { tag::guishown,       { "/nsm/client/gui_is_shown",          ""        } },
        { tag::guisrvannounce, { "/nsm/gui/server_announce",         "s"        } },
        { tag::guistatus,      { "/nsm/gui/client/status",            "ss"      } },
        { tag::guistop,        { "/nsm/gui/client/stop",              "s"       } },
        { tag::guiswitch,      { "/nsm/gui/client/switch",            "ss"      } },
        { tag::guivisible,     { "/nsm/gui/client/gui_visible",       "si"      } },
        { tag::nonaddstrip,    { "/non/mixer/add_strip",              ""        } },
        { tag::nonhello,       { "/non/hello",                        "ssss"    } },
        { tag::null,           { NIL,                                 NIL       } },
        { tag::oscping,        { "/osc/ping",                         ""        } },
        { tag::oscreply,       { "",                                  ""        } },
        { tag::proxyargs,      { "/nsm/proxy/arguments",              "s"       } },
        { tag::proxycfgfile,   { "/nsm/proxy/config_file",            "s"       } },
        { tag::proxyerror,     { "/nsm/proxy/client_error",           "s"       } },
        { tag::proxyexe,       { "/nsm/proxy/executable",             "s"       } },
        { tag::proxykill,      { "/nsm/proxy/kill",                   ""        } },
        { tag::proxylabel,     { "/nsm/proxy/label",                  "s"       } },
        { tag::proxysave,      { "/nsm/proxy/save_signal",            "i"       } },
        { tag::proxystart,     { "/nsm/proxy/start",                  "sss"     } },
        { tag::proxystop,      { "/nsm/proxy/stop_signal",            "i"       } },
        { tag::proxyupdate,    { "/nsm/proxy/update",                 ""        } },
        { tag::reply,          { "/reply",                            "ss"      } },
        { tag::replyex,        { "/reply",                            "ssss"    } },
        { tag::sessionlist,    { "/nsm/session/list",                 "?"       } },
        { tag::sessionname,    { "/nsm/session/name",                 "ss"      } },
        { tag::sessionroot,    { "/nsm/gui/session/root",             "s"       } },
        { tag::sigconnect,     { "/signal/connect",                   "ss"      } },
        { tag::sigcreated,     { "/signal/created",                   "ssfff"   } },
        { tag::sigdisconnect,  { "/signal/disconnect",                "ss"      } },
        { tag::sighello,       { "/signal/hello",                     "ss"      } },
        { tag::siglist,        { "/signal/list",                      NIL       } },
        { tag::sigremoved,     { "/signal/removed",                   "s"       } },
        { tag::sigrenamed,     { "/signal/renamed",                   "ss"      } },
        { tag::sigreply,       { "/reply",                            NIL       } },
        { tag::srvabort,       { "/nsm/server/abort",                 ""        } },
        { tag::srvadd,         { "/nsm/server/add",                   "s"       } },
        { tag::srvannounce,    { "/nsm/server/announce",              "sssiii"  } },
        { tag::srvbroadcast,   { "/nsm/server/broadcast",             NIL       } },
        { tag::srvclose,       { "/nsm/server/close",                 ""        } },
        { tag::srvduplicate,   { "/nsm/server/duplicate",             "s"       } },
        { tag::srvlist,        { "/nsm/server/list",                  ""        } },
        { tag::srvmessage,     { "/nsm/gui/server/message",           "s"       } },
        { tag::srvnew,         { "/nsm/server/new",                   "s"       } },
        { tag::srvopen,        { "/nsm/server/open",                  "s"       } },
        { tag::srvquit,        { "/nsm/server/quit",                  ""        } },
        { tag::srvreply,       { "/reply",                            "s"       } },
        { tag::srvsave,        { "/nsm/server/save",                  ""        } },
        { tag::stripbynumber,  { "",                                  ""        } }
    };
    return s_all_messages;
}

#if defined USE_THIS_CODE

/**
 *  Used by Non itself.
 */

static lookup s_non_msgs
{
    {  tag::addstrip,        { "/non/mixer/add_strip",          ""        } },
    {  tag::hello,           { "/non/hello",                    "ssss"    } },
    {  tag::oscreply,        { "",                              ""        } },
    {  tag::stripbynumber,   { "",                              ""        } }
};

#endif

/**
 *  Generic osc::tag lookup function for a lookup table.
 *
 * \param table
 *      Provides the particular table (e.g. client versus server) to be looked
 *      up.
 *
 * \param t
 *      Provides the "tag" enumeration value to be used to look up the desired
 *      message and its message pattern.
 *
 * \param [out] message
 *      The destination for the message text that was found.
 *
 * \param [out] pattern
 *      The destination for the pattern text that was found.
 *
 * \return
 *      Returns true if the tag \a t was found.  If false is returned, do not
 *      use the message and pattern.
 */

bool
tag_lookup
(
    const lookup & table,
    tag t, std::string & message, std::string & pattern
)
{
    bool result = false;
    auto lci = table.find(t);           /* lookup::const_iterator   */
    if (lci != table.end())
    {
        result = true;
        message = lci->second.msg_text;
        pattern = lci->second.msg_pattern;
    }
    return result;
}

/**
 *  Does an osc::tag lookup from the "all messages" table.
 */

bool
tag_lookup
(
    tag t, std::string & message,
    std::string & pattern
)
{
    return tag_lookup(all_messages(), t, message, pattern);
}

/**
 *  Yet another overload, a simplified lookup using a vector of tags.
 */

bool
tag_lookup
(
    const taglist & tl,
    tag t, std::string & message,
    std::string & pattern
)
{
    auto ti = std::find(tl.begin(), tl.end(), t);
    bool result = ti != tl.end();
    if (result)
        result = tag_lookup(*ti, message, pattern);

    return result;
}

/**
 *  This tag lookup is useful when all we want is the message
 *  string (the path) for the given tag.
 */

const std::string &
tag_message (tag t)
{
    static std::string s_empty;
    auto lci = all_messages().find(t);          /* lookup::const_iterator   */
    if (lci != all_messages().end())
        return lci->second.msg_text;

    return s_empty;
}

/**
 *  Inverse lookup.  Given the message and pattern names, return the tag.
 *
 * \param table
 *      The particular category of <tag, message, pattern> items to
 *      look up.
 *
 * \param message
 *      The OSC message, such as "/nsm/gui/announce".
 *
 * \param pattern
 *      The value pattern, such as "sss", "", NIL ("-") etc.
 *      The default is "?", which means to ignore the pattern and
 *      match only on the message. This is useful when getting reply or
 *      error messages.
 */

tag
tag_reverse_lookup
(
    const lookup & table,
    const std::string & message,
    const std::string & pattern
)
{
    tag result = tag::illegal;
    for (const auto & m : table)
    {
        bool match = m.second.msg_text == message;
        if (match)
        {
            if (pattern != "?")
                match = m.second.msg_pattern == pattern;
        }
        if (match)
        {
            result = m.first;
            break;
        }
    }
    return result;
}

/**
 *  Inverse lookup, using the "all-message" lookup table.
 *
 * \param message
 *      The OSC message, such as "/nsm/gui/announce".
 *
 * \param pattern
 *      The value pattern, such as "sss" or "".
 */

tag
tag_reverse_lookup
(
    const std::string & message,
    const std::string & pattern
)
{
    return tag_reverse_lookup(all_messages(), message, pattern);
}

/**
 *  This map of names and tags is useful in specifying options for
 *  the nsmctl application. The "gui" tags also require a client name
 *  (e.g. "jackpatch" or "qseq66" to be supplied.
 */

static tagmap s_tag_names =
{
    {   "guisave",      { true,  tag::guisave        } },   /* GUI save     */
    {   "show",         { true,  tag::guishow        } },
    {   "hide",         { true,  tag::guihide        } },
    {   "remove",       { true,  tag::guiremove      } },
    {   "resume",       { true,  tag::guiresume      } },
    {   "stop",         { true,  tag::guistop        } },
    {   "abort",        { false, tag::srvabort       } },
    {   "close",        { false, tag::srvclose       } },
    {   "save",         { false, tag::srvsave        } },   /* session save */
    {   "open",         { false, tag::srvopen        } },
    {   "duplicate",    { false, tag::srvduplicate   } },
    {   "quit",         { false, tag::srvquit        } },
    {   "list",         { false, tag::srvlist        } },
    {   "new",          { false, tag::srvnew         } },
    {   "add",          { false, tag::srvadd         } }
};

tag
tag_name_lookup (const std::string & name)
{
    tag result = tag::illegal;
    auto it = s_tag_names.find(name);
    if (it != s_tag_names.end())
        result = it->second.osc_tag;

    return result;
}

bool
tag_name_is_client (const std::string & name)
{
    bool result = false;
    auto it = s_tag_names.find(name);
    if (it != s_tag_names.end())
        result = it->second.is_client_tag;

    return result;
}

bool
tag_needs_argument (const std::string & name)
{
    bool result = tag_name_is_client(name);
    if (! result)
        result = name == "open" || name == "new" || name == "duplicate";

    return result;
}

void
tag_name_action_list (lib66::tokenization & actions)
{
    actions.clear();
    for (const auto & a : s_tag_names)
    {
        bool isclient = a.second.is_client_tag;
        std::string path { tag_message(a.second.osc_tag) };
        std::string type { isclient ? "client" : "server" };
        std::ostringstream os;
        os
            << std::left << std::setw(10) << a.first
            << " [" << type << "]  "
            << path
            ;
        actions.push_back(os.str());
    }
}

}           // namespace osc

/*
* messages.cpp
*
* vim: sw=4 ts=4 wm=4 et nowrap ft=cpp
*/

