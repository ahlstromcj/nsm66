#if ! defined NSM66_OSC_ENDPOINT_HPP
#define NSM66_OSC_ENDPOINT_HPP

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
 * \file          endpoint.hpp
 *
 *    This module refactors the Endpoint class to replace C code with
 *    C++ code.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-02-05
 * \updates       2025-04-08
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   The endpoint class extends lowrapper to add functionality needed
 *   by some NSM applications. The concepts added are:
 *
 *      -   An endpoint owner.
 *      -   A thread for message processing, with a flag to die.
 *      -   Translation map. Whatever that is.
 *      -   Lists:
 *          -   Peers and scanning for them.
 *          -   Signals.
 *          -   Methods.
 *      -   Various static OSC method handlers.
 *      -   And a lot more.
 *
 *  Used by nsmd and nsm-legacy-gui.
 */

#include <map>
#include <string>
#include <lo/lo.h>

#include "osc/lowrapper.hpp"            /* osc::lowrapper base class, funcs */
#include "osc/method.hpp"               /* osc::method, method_list alias   */
#include "osc/signal.hpp"               /* osc::signal, peer & signal lists */
#include "osc/thread.hpp"               /* osc::thread                      */

namespace osc
{

/**
 *  The endpoint class add the concepts of an endpoint owner, an
 *  execution thread, a list of peers, signals and methods and more.
 *
 *  To be fleshed out.
 */

class endpoint : public lowrapper
{
    friend class signal;

private:

    class translation_destination
    {
    public:

        std::string m_path;
        float m_current_value;
        bool m_suppress_feedback;

        translation_destination () :
            m_path              (),
            m_current_value     (-1.0f),
            m_suppress_feedback (false)
        {
            // no code
        }
    };

    using translation_map = std::map<std::string, translation_destination>;

private:

    /*
     * Data members.
     */

    void * m_owner;

    /*
     * Implemented via pthreads.
     */

    thread m_thread;

    /*
     * Defined as an std::list<peer *> in the signal module.
     */

    peer_list m_peers;

    /*
     * Defined as an std::list<signal *> in the signal module.
     */

    signal_list m_signals;

    /*
     * Defined as an std::list<method *> in the method module.
     */

    method_list m_methods;

    std::string m_learning_path;

    translation_map m_translations;

    std::string m_name;

    bool m_time_to_die;

    void * m_peer_scan_complete_userdata;

    void * m_peer_signal_notification_userdata;

    /*
     * Function members.
     */

    void (* m_peer_scan_complete_callback) (void *);

    void (* m_peer_signal_notification_callback)
    (
        osc::signal *, osc::signal::state, void *
    );

public:

    endpoint ();
    virtual ~endpoint ();

    void die ()
    {
        m_time_to_die = true;
    }

    const void * owner () const
    {
        return m_owner;
    }

    void * owner ()
    {
        return m_owner;
    }

    void owner (void * p)
    {
        m_owner = p;
    }

protected:      /* virtual functions    */

    virtual void add_methods (void * userdata) override;

private:

    /*---------------------------------------------------------------------
     * Static member functions used by the OSC service.
     *---------------------------------------------------------------------*/

    static void error_handler
    (
        int num, const char * msg, const char * path
    );
    static bool osc_params_check
    (
        const char * path, const char * types, lo_arg ** argv, int argc
    );
    static int osc_reply
    (
        const char * path, const char * types, lo_arg ** argv, int argc,
        lo_message msg, void * user_data
    );
    static int osc_signal_lister
    (
        const char * path, const char * types, lo_arg ** argv, int argc,
        lo_message msg, void * user_data
    );
    static int osc_generic
    (
        const char * path, const char * types, lo_arg ** argv, int argc,
        lo_message msg, void * user_data
    );
    static int osc_sig_handler
    (
        const char * path, const char * types, lo_arg ** argv, int argc,
        lo_message msg, void * user_data
    );
    static int osc_sig_renamed
    (
        const char * path, const char * types, lo_arg ** argv, int argc,
        lo_message msg, void * user_data
    );
    static int osc_sig_removed
    (
        const char * path, const char * types, lo_arg ** argv, int argc,
        lo_message msg, void * user_data
    );
    static int osc_sig_created
    (
        const char * path, const char * types, lo_arg ** argv, int argc,
        lo_message msg, void * user_data
    );
    static int osc_sig_disconnect
    (
        const char * path, const char * types, lo_arg ** argv, int argc,
        lo_message msg, void * user_data
    );
    static int osc_sig_connect
    (
        const char * path, const char * types, lo_arg ** argv, int argc,
        lo_message msg, void * user_data
    );
    static int osc_sig_hello
    (
        const char * path, const char * types, lo_arg ** argv, int argc,
        lo_message msg, void * user_data
    );
    static void * osc_thread (void * arg);
    static bool address_matches (lo_address addr1, lo_address addr2);
    static signal * find_target_by_peer_address
    (
        signal_list * lst, lo_address addr
   );

private:

    peer * add_peer (const std::string & name, const std::string & url);
    void scan_peer (const std::string & name, const std::string & url);
    void osc_thread ();
    osc::signal * find_peer_signal_by_path (peer * p, const std::string & path);
    osc::signal * find_signal_by_path (const std::string & path);
    peer * find_peer_by_name (const std::string & name);
    peer * find_peer_by_address (lo_address addr);

    void add_sig_methods (void * userdata);
    void del_signal (signal * signal);
    void send_signal_rename_notifications(signal * s);
    void (* m_peer_signal_callback)(osc::signal *,  osc::signal::state, void *);
    void * m_peer_signal_userdata;

public:

    void send_feedback (const std::string &path, float v);
    void learn (const std::string &path);

#if defined USE_OLD_CODE
    const std::string & * get_connections (const std::string & path);
#else
    lib66::tokenization get_connections (const std::string & path);
#endif
    void clear_translations ();
    void del_translation (const std::string & a);
    void add_translation (const std::string & a, const std::string & b);
    void rename_translation_destination
    (
        const std::string & a, const std::string & b
    );
    void rename_translation_source
    (
        const std::string & a, const std::string & b
    );
    int ntranslations ();
    bool get_translation
    (
        int n, std::string & from, std::string & to
    );

    void peer_signal_notification_callback
    (
        void (* cb) (osc::signal *, osc::signal::state, void *),
        void * userdata
    )
    {
        m_peer_signal_notification_callback = cb;
        m_peer_signal_notification_userdata = userdata;
    }

#if defined USE_LIST_PEER_SIGNALS
    void list_peer_signals (void * v);
#endif

    virtual bool init
    (
        int proto,
        const std::string & portname    = "",
        bool usethis                    = false
    ) override;

#if defined THESE_OVERLOADS_ARE_CODED
    bool connect_signal (osc::signal * s, osc::signal * d);
    bool connect_signal
    (
        osc::signal * s,
        const std::string & peer_name,
        const std::string & signal_path
   );
    bool disconnect_signal (osc::signal * s, osc::signal * d);
#endif

    bool connect_signal (osc::signal * s, const std::string &  peer_and_path);
    bool disconnect_signal (osc::signal * s, const std::string & signal_path);

    method * add_method
    (
        const std::string & path,
        const std::string & typespec,
        lo_method_handler handler,
        void * user_data                            = nullptr,
        const std::string & argument_description    = ""
    );
    signal * add_signal
    (
        const std::string & path,
        signal::direction dir,
        float min, float max, float default_value,
        signal_handler handler,
        void * user_data
    );

#if defined USE_DEL_METHOD
    void del_method (const std::string & path, const std::string & typespec);
    void del_method (method * method);
#endif
    void start ();
    void stop ();
    int port () const;
    void check () const;
    void wait (int timeout ) const;
    void run () const;

    void name (const std::string & name)
    {
        m_name = name;
    }

    const std::string & name ()
    {
        return m_name;
    }

    void hello (const std::string & url);
    void handle_hello
    (
        const std::string & peer_name,
        const std::string & peer_url
    );

    void peer_scan_complete_callback
    (
        void (* cb) (void *),
        void * userdata
    )
    {
        m_peer_scan_complete_callback = cb;
        m_peer_scan_complete_userdata = userdata;
    }

};

}           // namespace osc

#endif      // defined NSM66_OSC_ENDPOINT_HPP

/*
 * endpoint.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

