#if ! defined NSM66_NSM_NSMMESSAGESEX_HPP
#define NSM66_NSM_NSMMESSAGESEX_HPP

/**
 * \file          nsmmessagesex.hpp
 *
 *    This module provides a kind of repository of all the possible OSC/NSM
 *    messages.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom
 * \date          2025-01-29
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

#include "osc/messages.hpp"             /* osc::tag                         */

/*
 *  The nsm namespace is a place to squirrel away concepts that are
 *  not defined in the other nsmbase classes.
 */

namespace nsm
{

/**
 *  This type holds the long OSC string for the message, and the data
 *  pattern string that describes the data being sent.
 */

using messagepair = struct messagepair
{
    std::string msg_text;
    std::string msg_pattern;
};

/**
 *  A lookup map for tags and message pairs.
 */

using lookup = std::map<osc::tag, messagepair>;

/*
 *  More free functions.
 */

extern const std::string & default_ext ();
extern const std::string & url ();

/*
 *  Free functions for table lookup.
 */

extern bool client_msg
(
    osc::tag t, std::string & message, std::string & pattern
);
extern bool server_msg
(
    osc::tag t, std::string & message, std::string & pattern
);
extern std::string get_dirtiness_msg (bool isdirty);
extern std::string get_visibility_msg (bool isvisible);
extern bool is_gui_announce (const std::string & s = "");

#if defined USE_THIS_CODE

/*
 * We doubt we really need these functions.
 */

extern bool gui_client_msg
(
    osc::tag t, std::string & message, std::string & pattern
);
extern bool gui_session_msg
(
    osc::tag t, std::string & message, std::string & pattern
);
extern bool proxy_msg (osc::tag t, std::string & message, std::string & pattern);
extern bool misc_msg (osc::tag t, std::string & message, std::string & pattern);


/*
 *  Free functions for inverse table lookup.
 */

extern osc::tag client_tag
(
    const std::string & message,
    const std::string & pattern = "X"
);
extern osc::tag server_tag
(
    const std::string & message,
    const std::string & pattern = "X"
);

#endif      // USE_THIS_CODE

}           // namespace nsm

#endif      // NSM66_NSM_NSMMESSAGESEX_HPP

/*
 * nsmmessagesex.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

