#if ! defined NSM66_NSM_NSMBASE_HPP
#define NSM66_NSM_NSMBASE_HPP

/**
 * \file          nsmbase.hpp
 *
 *    This module provides a reimplementation of the nsm.h header file as a
 *    class.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-01-29
 * \updates       2025-04-24
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Upcoming support for the Non Session Manager.
 */

#include <atomic>                       /* std::atomic<bool>                */
#include <vector>                       /* std::vector                      */

#include "c_macros.h"                   /* is_nullptr() & not_nullptr()     */
#include "cpp_types.hpp"                /* is_nullptr() & not_nullptr()     */
#include "nsm66-config.h"               /* feature (HAVE) macros            */
#include "nsm/nsmcodes.hpp"             /* ns::error enumeration            */
#include "nsm/nsmmessagesex.hpp"        /* osc::tag                         */
#include "osc/lowrapper.hpp"            /* osc::lowrapper base class        */

namespace nsm
{

/**
 *  Enumeration to represent the "capabilities" supported by the Non
 *  Session Manager (NSM).
 *
 * \var none
 *      Indicates no capabilities; provided for completeness or
 *      error-checking.
 *
 * \var cswitch
 *      The client is capable of responding to multiple open messages
 *      without restarting. The string for this value is "switch", but that
 *      is a reserved word in C/C++.
 *
 * \var dirty
 *      The client knows when it has unsaved changes.
 *
 * \var progress
 *      The client can send progress updates during time-consuming
 *      operations.
 *
 * \var message
 *      The client can send textual status updates.
 *
 * \var optional_gui
 *      The client has a optional GUI.  The server responds to "optional-gui"
 *      messages.  If this capability is not present, then clients with
 *      "optional-gui" must always keep themselves visible.
 *
 * \var server_control
 *      The server provides client-to-server control.
 *
 * \var broadcast
 *      The server responds to the "/nsm/server/broadcast" message.
 */

enum class caps
{
    none,               /* ""               */
    broadcast,
    cswitch,            /* ":switch:"       */
    dirty,              /* ":dirty:"        */
    message,            /* ":message:"      */
    optional_gui,       /* ":optional-gui:" */
    progress,           /* ":progress:"     */
    server_control,
};

/**
 *  nsmbase is an NSM OSC server/client base class.
 */

class nsmbase : public osc::lowrapper
{

private:

    static std::string sm_nsm_default_ext;

    /**
     *  Provides a reference (a void pointer) to an OSC service. See
     *  /usr/include/lo/lo_types.h.

    lo_address m_lo_address;
     */

    /**
     *  Provides a reference (a void pointer) to a thread "containing"
     *  an OSC server. See /usr/include/lo/lo_types.h.
     */

    lo_server_thread m_server_thread;

    /**
     *  Provides a reference (a void pointer) to an object representing an
     *  an OSC server. See /usr/include/lo/lo_types.h.
     */

    lo_server m_lo_server;

    /**
     *  This item is mutable because it can be falsified if the server and
     *  address are found to be null.  It is turned on when we receive the
     *  information about the session (including path to the session).
     */

    mutable std::atomic<bool> m_active;

    /**
     *  Additional data.
     */

    bool m_dirty;
    int m_dirty_count;
    std::string m_manager;
    std::string m_capabilities;
    std::string m_path_name;
    std::string m_display_name;
    std::string m_client_id;
    std::string m_nsm_file;
    std::string m_nsm_ext;
    std::string m_nsm_url;

public:

    nsmbase
    (
        const std::string & nsmurl,
        const std::string & nsmfile = "",
        const std::string & nsmext  = ""
    );
    virtual ~nsmbase ();

public:

    bool is_active() const              // session activation accessor
    {
        return m_active;
    }

    bool is_a_client (const nsmbase * p)
    {
        return not_nullptr(p) && p->is_active();
    }

    bool not_a_client (const nsmbase * p)
    {
        return is_nullptr(p) || ! p->is_active();
    }

    // Session manager accessors.

    const std::string & manager () const
    {
        return m_manager;
    }

    const std::string & capabilities () const
    {
        return m_capabilities;
    }

    // Session client accessors.

    const std::string & path_name () const
    {
        return m_path_name;
    }

    const std::string & display_name () const
    {
        return m_display_name;
    }

    const std::string & client_id () const
    {
        return m_client_id;
    }

    const std::string & nsm_file () const
    {
        return m_nsm_file;
    }

    const std::string & nsm_ext () const
    {
        return m_nsm_ext;
    }

    const std::string & nsm_url () const
    {
        return m_nsm_url;
    }

    bool dirty () const
    {
        return m_dirty;
    }

    void dirty (bool isdirty);          /* session managers call this one   */

protected:

    void path_name (const std::string & s)
    {
        m_path_name = s;
    }

    void display_name (const std::string & s)
    {
        m_display_name = s;
    }

    void client_id (const std::string & s)
    {
        m_client_id = s;
    }

    void is_active (bool f)
    {
        m_active = f;
    }

    void manager (const std::string & s)
    {
        m_manager = s;
    }

    void capabilities (const std::string & s)
    {
        m_capabilities = s;
    }

protected:

    bool msg_check (int timeoutms = 0);                     /* milliseconds */
    bool lo_is_valid () const;
    void nsm_debug (const std::string & tag);
    bool send_announcement
    (
        const std::string & appname,
        const std::string & exename,
        const std::string & capabilities
    );
    void start_thread ();
    void stop_thread ();
    void update_dirty_count (bool flag = true);

    // Session client reply methods

    bool open_reply
    (
        nsm::error errorcode = nsm::error::ok,
        const std::string & msg = "No info"
    );
    bool save_reply
    (
        nsm::error errorcode = nsm::error::ok,
        const std::string & msg = "No info"
    );
    bool send_nsm_reply
    (
        const std::string & path,
        nsm::error errorcode,
        const std::string & msg
    );
    bool send
    (
        const std::string & message,
        const std::string & pattern
    );
    bool send_from_client (osc::tag t);
    bool send_from_client
    (
        osc::tag t,
        const std::string & s1,
        const std::string & s2,
        const std::string & s3 = ""
    );

    void open_reply (bool loaded)
    {
        open_reply(loaded ? nsm::error::ok : nsm::error::general);
        if (loaded)
            m_dirty = false;
    }

    void save_reply (bool saved)
    {
        save_reply(saved ? nsm::error::ok : nsm::error::general);
        if (saved)
            m_dirty = false;
    }

public:             // virtual methods for callbacks in nsmbase

    virtual void nsm_reply                      /* generic replies */
    (
        const std::string & message,
        const std::string & pattern
    );
    virtual void nsm_error (int errcode, const std::string & mesg);

protected:          // virtual methods

    virtual bool progress (float percent);
    virtual bool is_dirty ();
    virtual bool is_clean ();
    virtual bool message (int priority, const std::string & mesg);
    virtual bool initialize ();         /* compare to lowrapper::init()     */
    virtual void add_thread_method      /* vs lowrapper::add_osc_method()   */
    (
        osc::tag t,
        osc::method_handler f,
        void * userdata = nullptr
    );

    /*
     * Used by the free-function OSC callbacks, and there are too many to make
     * as friends.
     */

public:

    virtual void announce_reply
    (
        const std::string & mesg,
        const std::string & manager,
        const std::string & capabilities
    ) = 0;

    virtual void open
    (
        const std::string & path_name,
        const std::string & display_name,
        const std::string & client_id
    ) = 0;
    virtual void save () = 0;
    virtual void label (const std::string & label) = 0;
    virtual void loaded () = 0;
    virtual void show (const std::string & path) = 0;
    virtual void hide (const std::string & path) = 0;
    virtual void handle_broadcast
    (
        const std::string & message,
        const std::string & pattern,
        const lib66::tokenization & argv
    ) = 0;
    virtual bool announce
    (
        const std::string & app_name,
        const std::string & exe_name,
        const std::string & capabilities
    ) = 0;

protected:

    /*
     * Prospective caller helpers a la qtractorMainForm.
     */

    virtual bool open_session ();
    virtual bool save_session ();
    virtual bool close_session ();

private:

    int send_from
    (
        const std::string & message,
        const std::string & pattern,
        const std::string & s1 = "",
        const std::string & s2 = "",
        const std::string & s3 = ""
    );

    /*
     * Other message args not covered:
     *
     *  -   int, string
     *  -   string, string, string, int, int, int
     */

private:

    /*
     * Static OSC callback functions.
     */

    static int osc_nsm_error
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );
    static int osc_nsm_reply
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );

};          // class nsmbase

/*
 *  External helper functions in the nsm namespace.
 */

extern std::string reply_string (error errorcode);
extern std::string get_url ();
extern void incoming_msg
(
    const std::string & cbname,
    const std::string & message,
    const std::string & pattern,
    bool iserror = false
);
extern void outgoing_msg
(
    const std::string & message,
    const std::string & pattern,
    const std::string & data = "sent"
);
extern lib66::tokenization convert_lo_args
(
    const std::string & pattern,
    int argc, lo_arg ** argv
);

}           // namespace nsm

#endif      // NSM66_NSM_NSMBASE_HPP

/*
 * nsmbase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

