/**
 * \file          nsmmessagesex.cpp
 *
 *    This module provides a kind of repository of some of the OSC/NSM
 *    messages.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom
 * \date          2020-08-21
 * \updates       2025-04-23
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *      Seq66 support for the Non Session Manager.  Defines a number of free
 *      functions in the nsm66::nsm namespace. The actual tags have been
 *      moved to the osc/messages module.
 *
 * Commands handling by the server:
 *
 *      -#  add.  Adds a client process. Sends either an "/error" + path
 *          message, or a "/reply" + path + OK + "Launched" message.  ("New"
 *          doesn't send OK!)
 *      -#  announce. The client sends an "announce" message. If there is no
 *          session, "/error" + path + errcode + message is sent.
 *          Incompatible API versions are detected.
 *      -#  save. Commands all clients to save.  Sends either an "/error"
 *          message or a "/reply" + path + "Saved" message.
 *      -#  duplicate. Duplicates a session.  Sends an "/error" or a
 *          "/nsm/gui/session/session" message plus a "/reply" + path +
 *          "Duplicated" message.
 *      -#  new. Commands all clients to save, and then creates a new session.
 *          Sends an "/error" or a "/nsm/gui/session/session" message plus a
 *          "/reply" + path + "Session created" message.
 *      -#  list. Lists sessions.  Sends an empty "/reply", then an
 *          "/nsm/server/list" message with an empty message.
 *      -#  open. Opens a session.  Sends an "/error" message or a "/reply" +
 *          path + "Loaded" message.
 *      -#  quit.  Closes the session.  Sends "/nsm/gui/session/name" plus an
 *          empty session name.
 *      -#  abort. If a session is open and there is no operation pending (in
 *          which cases an "/error" is sent), then the session is quit as
 *          above.
 *      -#  close.  Similar to "abort", except that all clients are first
 *          commanded to save.
 *      -#  broadcast.  The server sends out a command to all clients.
 *      -#  progress.  Sends "/nsm/gui/client/progress" + Client-ID +
 *          progress.
 *      -#  is_dirty.  A client sends "/nsm/client/is_dirty" and the server
 *          sends out "/nsm/gui/client/dirty" + Client-ID + dirty.
 *      -#  is_clean.  A client sends "/nsm/client/is_clean" and the server
 *          sends out "/nsm/gui/client/dirty" + Client-ID + 0.
 *      -#  gui_is_hidden.  The client sends "/nsm/client/gui_is_hidden" and
 *          the server sends "/nsm/gui/client/gui_visible" + Client-ID + 0.
 *      -#  gui_is_shown.  The client sends "/nsm/client/gui_is_shown" and
 *          the server sends "/nsm/gui/client/gui_visible" + Client-ID + 1.
 *      -#  message.  The client sends "/nsm/client/message" + Client_ID +
 *          integer + string, and the server forwards this information to all
 *          clients via an "/nsm/gui/client/message".
 *      -#  label. The client sends an "/nsm/client/label" message, and the
 *          server sends out an "/nsm/gui/client/label" message.
 *      -#  error. The client sends an "/error" message ("sis" parameters),
 *          and the server sends out "/nsm/gui/client/status" + Client-ID +
 *          status.  MORE TO COME.
 *      -#  reply. The client sends a "/reply" message ("ssss" parameters),
 *          and the server sends out "/nsm/gui/client/status" + Client-ID +
 *          status.  MORE TO COME.
 *      -#  stop.  A GUI operation.
 *      -#  remove.  A GUI operation.
 *      -#  resume.  A GUI operation.
 *      -#  client_save.  A GUI operation.
 *      -#  client_show_optional_gui.  A GUI operation.
 *      -#  client_hide_optional_gui.  A GUI operation.
 *      -#  gui_announce.
 *      -#  ping.
 *      -#  null.
 */

#include "nsm/nsmmessagesex.hpp"        /* nsm66::nsm::tag, etc.            */

namespace nsm
{

/*
 * Helpful functions.
 */

const std::string &
default_ext ()
{
    static const std::string sm_default_ext("nsm");
    return sm_default_ext;
}

const std::string &
url ()
{
    static const std::string sm_url_var("NSM_URL");
    return sm_url_var;
}

/**
 *  This map of message/pattern pairs provides all the messages and patterns
 *  used in the "/nsm/client/xxxxx" series of messages, including client
 *  variations of "/error" and "/reply".
 *
 *  These definitions are in the osc/message.cpp module.
 *
 *  The rest of these free functions provide easy lookup of the various messages
 *  and their patterns.
 */

bool
client_msg (osc::tag t, std::string & message, std::string & pattern)
{
    static osc::taglist s_client_tags
    {
        osc::tag::cliclean,
        osc::tag::clidirty,
        osc::tag::clihide,
        osc::tag::clilabel,
        osc::tag::cliloaded,
        osc::tag::climessage,
        osc::tag::cliopen,
        osc::tag::cliprogress,
        osc::tag::clisave,
        osc::tag::clishow,
        osc::tag::error,
        osc::tag::guihidden,
        osc::tag::guishown,
        osc::tag::null,
        osc::tag::reply,
        osc::tag::replyex
    };
    return osc::tag_lookup(s_client_tags, t, message, pattern);
}

bool
server_msg (osc::tag t, std::string & message, std::string & pattern)
{
    static osc::taglist s_server_tags
    {
        osc::tag::sigreply,
        osc::tag::srvabort,
        osc::tag::srvadd,
        osc::tag::srvannounce,
        osc::tag::srvbroadcast,
        osc::tag::srvclose,
        osc::tag::srvduplicate,
        osc::tag::srvlist,
        osc::tag::srvnew,
        osc::tag::srvopen,
        osc::tag::srvquit,
        osc::tag::srvsave
    };
    return osc::tag_lookup(s_server_tags, t, message, pattern);
}

std::string
get_dirtiness_msg (bool isdirty)
{
    osc::tag t = isdirty ? osc::tag::clidirty : osc::tag::cliclean ;
    return osc::tag_message(t);
}

/*
 * Might need a gui version of this function as well.
 */

std::string
get_visibility_msg (bool isvisible)
{
    osc::tag t = isvisible ? osc::tag::guishown : osc::tag::guihidden ;
    return osc::tag_message(t);
}

bool
is_gui_announce (const std::string & s)
{
    return s == osc::tag_message(osc::tag::gui_announce);
}

#if defined USE_THIS_CODE

/*
 * We doubt we really need these functions.
 */

/*
 * Still need the rest of the gui messages.
 */

bool
gui_client_msg (osc::tag t, std::string & message, std::string & pattern)
{
    static osc::taglist s_gui_client_tags
    {
        osc::tag::gui_announce,
        osc::tag::guidirty,
        osc::tag::guihide,
        osc::tag::guilabel,
        osc::tag::guimessage,
        osc::tag::guinew,
        osc::tag::guioption,
        osc::tag::guiprogress,
        osc::tag::guiremove,
        osc::tag::guiresume,
        osc::tag::guisave,
        osc::tag::guishow,
        osc::tag::guistatus,
        osc::tag::guistop,
        osc::tag::guiswitch,
        osc::tag::guivisible
    };
    return osc::tag_lookup(s_gui_client_tags, t, message, pattern);
}

bool
gui_session_msg (osc::tag t, std::string & message, std::string & pattern)
{
    static osc::lookup s_gui_session_tags
    {
        osc::tag::gui_announce,
        osc::tag::guimessage,
        osc::tag::guisession,
        osc::tag::guisessionname,
        osc::tag::sessionroot
    };
    return osc::tag_lookup(s_gui_session_tags, t, message, pattern);
}

bool
proxy_msg (osc::tag t, std::string & message, std::string & pattern)
{
    static osc::lookup s_proxy_tags
    {
        osc::tag::proxyargs,
        osc::tag::proxycfgfile,
        osc::tag::proxyerror,
        osc::tag::proxyexe,
        osc::tag::proxykill,
        osc::tag::proxylabel,
        osc::tag::proxysave,
        osc::tag::proxystart,
        osc::tag::proxystop,
        osc::tag::proxyupdate,
    };
    return osc::tag_lookup(s_proxy_tags, t, message, pattern);
}

bool
misc_msg (osc::tag t, std::string & message, std::string & pattern)
{
    static osc::lookup s_misc_tags
    {
        osc::tag::error,
        osc::tag::oscping,
        osc::tag::reply,
        osc::tag::sessionlist,
        osc::tag::sessionname,
        osc::tag::sessionname,
        osc::tag::sessionroot,
    };
    return osc::tag_lookup(s_misc_tags, t, message, pattern);
}

/*
 * The rest of these free functions provide easy lookup of the various tags
 * from the given message.
 */

osc::tag
client_tag (const std::string & message, const std::string & pattern)
{
    return osc::tag_reverse_lookup(osc::s_client_msgs, message, pattern);
}

osc::tag
server_tag (const std::string & message, const std::string & pattern)
{
    return osc::tag_reverse_lookup(osc::s_server_msgs, message, pattern);
}

/*
 *  The nsm66::nsm namespace is a place to squirrel away concepts that are
 *  not defined in the other nsmbase classes.
 *
 *  Here, we now use the osc namespace for the extra sets of messages
 *  categorized by types.
 */

/**
 *  Used in creating an OSC server endpoint.
 */

static osc::taglist s_signal_taglist =
{
    osc::tag::generic,
    osc::tag::reply,
    osc::tag::sigconnect,
    osc::tag::sigcreated,
    osc::tag::sigdisconnect,
    osc::tag::sighello,
    osc::tag::siglist,
    osc::tag::sigremoved,
    osc::tag::sigrenamed
};

/**
 *  Used by NSM itself.
 */

static osc::taglist s_non_taglist =
{
    osc::tag::nonaddstrip,
    osc::tag::nonhello,
    osc::tag::oscreply,
    osc::tag::stripbynumber
};

#endif          // USE_THIS_CODE

}           // namespace nsm

/*
* nsmmessagesex.cpp
*
* vim: sw=4 ts=4 wm=4 et nowrap ft=cpp
*/

