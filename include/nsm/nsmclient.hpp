#if ! defined NSM66_NSM_NSMCLIENT_HPP
#define NSM66_NSM_NSMCLIENT_HPP

/**
 * \file          nsmclient.hpp
 *
 *    This module provides macros for generating simple messages, MIDI
 *    parameters, and more.
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

#include "nsm/nsmbase.hpp"              /* nsm::nsmbase base class          */

namespace nsm
{

/**
 *  nsmclient is an NSM OSC client agent.
 */

class nsmclient : public nsmbase
{

public:

protected:

    /**
     *  Provides the session manager object helping this client do session
     *  management.  Not, here. Do it in an app-specific derived class.
     */

private:

    std::atomic<bool> m_hidden;

public:

    nsmclient
    (
        const std::string & nsm_url,
        const std::string & nsm_file    = "",
        const std::string & nsm_ext     = ""
    );
    virtual ~nsmclient ();

    void send_visibility (bool isshown);

    bool hidden () const
    {
        return m_hidden;
    }

public:     // session client method overrides

    virtual bool initialize () override;
    virtual void announce_reply
    (
        const std::string & mesg,
        const std::string & manager,
        const std::string & capabilities
    ) override;
    virtual void open
    (
        const std::string & path_name,
        const std::string & display_name,
        const std::string & client_id
    ) override;
    virtual void save () override;
    virtual void loaded () override;
    virtual void label (const std::string & label) override;
    virtual void show (const std::string & path) override;
    virtual void hide (const std::string & path) override;
    virtual void handle_broadcast
    (
        const std::string & message,
        const std::string & pattern,
        const lib66::tokenization & argv
    ) override;
    virtual bool announce
    (
        const std::string & app_name,
        const std::string & exe_name,
        const std::string & capabilities
    ) override;

public:         // Other virtual functions

    virtual bool open_session () override; // helper a la qtractorMainForm

    virtual void session_manager_name (const std::string & mgrname);
    virtual void session_manager_path (const std::string & pathname);
    virtual void session_display_name (const std::string & dispname);
    virtual void session_client_id (const std::string & clid);

protected:

    void hidden (bool flag)
    {
        m_hidden = flag;
    }

private:

    /*
     * Static OSC callback functions.
     */

    static int osc_nsm_announce_reply
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );
    static int osc_nsm_open
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );
    static int osc_nsm_save
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );
    static int osc_nsm_session_loaded
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );
    static int osc_nsm_label
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );
    static int osc_nsm_show
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );
    static int osc_nsm_hide
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );
    static int osc_nsm_broadcast
    (
        const char * path, const char * types, lo_arg ** argv,
        int argc, lo_message msg, void * user_data
    );

/*
 *
signals:                            // Session client callbacks.

    void active (bool is_active);
    void open ();
    void save ();
    void loaded ();
    void show ();
    void hide ();
 *
 */

};          // class nsmclient

/*
 *  External helper functions.
 */

extern nsmclient * create_nsmclient
(
    const std::string & nsmurl,
    const std::string & nsmfile,
    const std::string & nsmext
);

}           // namespace nsm

#endif      // NSM66_NSM_NSMCLIENT_HPP

/*
 * nsmclient.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

