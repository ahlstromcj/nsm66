#if ! defined NSM66_OSC_THREAD_HPP
#define NSM66_OSC_THREAD_HPP

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
 * \file          thread.hpp
 *
 *    This module refactors the thread class to replace C code with
 *    C++ code. It still uses pthreads instead of std::thread.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-02-05
 * \updates       2025-10-27
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   To do.
 */

/*
 * Simple wrapper for pthreads with thread role checking
 */

#include <pthread.h>                    /* <thread> for std::thread?        */
#include <string>                       /* std::string                      */

#include "cpp_types.hpp"                /* CSTR() inline functions          */

#if defined USE_THREAD_ASSERT           /* nobody uses this macro           */
#define THREAD_ASSERT(n) \
 ASSERT( thread::is( #n ), \
 "Function called from wrong thread! (is %s, should be %s)", \
 thread::current()->name(), #n )
#endif

namespace osc
{

class thread
{
    using entry_point = void * (*) (void *);

    using thread_data = struct
    {
        entry_point td_entry_point;
        void * td_arg;
        void * td_t;
    };

    static pthread_key_t m_current;

    pthread_t m_thread;

    std::string m_name;

    volatile bool m_running;

private:

    static void * run_thread (void * arg);

public:

    thread ();
    thread (const std::string & name);

    static bool is (const std::string & name);
    static void init ();
    static thread * current (void );

    const char * name_pointer () const
    {
        return CSTR(m_name);
    }

    const std::string & name () const
    {
        return m_name;
    }

    void name (const std::string & name)
    {
        m_name = name;
        set();
    }

    void set (const std::string & n);

    void set ()
    {
        set(CSTR(m_name));
    }

    bool running () const
    {
        return m_running;
    }

    bool clone (entry_point ep, void * arg);
    void clear_thread ();
    void detach ();
    void join ();
    void cancel ();
    void exit (void * retval = nullptr);

};

}           // namespace osc

#endif      // NSM66_OSC__THREAD_HPP

/*
 * thread.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

