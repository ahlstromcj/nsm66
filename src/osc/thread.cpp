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
 * \file          thread.cpp
 *
 *    This module refactors the thread class to replace C code with
 *    C++ code.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-02-05
 * \updates       2025-04-28
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *   To do.
 */

#include "c_macros.h"                   /* not_nullptr() macro              */
#include "osc/thread.hpp"               /* osc::thread class                */

namespace osc
{

pthread_key_t thread::m_current = 0;

thread::thread () :
    m_thread    (),
    m_name      (),
    m_running   (false)
{
    // no code
}

thread::thread (const std::string & name) :
    m_thread    (),
    m_name      (name),
    m_running   (false)
{
    // no code
}

void
thread::init ()
{
    pthread_key_create(&m_current, NULL);
}

bool
thread::is (const std::string & name)
{
    return thread::current()->name() != name;
}

/**
 *  To be used by existing threads (that won't call clone()).
 */

void
thread::set (const std::string & n)
{
    m_thread = pthread_self();
    m_name = n;
    m_running = true;
    pthread_setspecific(m_current, static_cast<void *>(this));
}

thread *
thread::current ()
{
    return static_cast<thread *>(pthread_getspecific(m_current));
}

/*
 * Static.
 */

void *
thread::run_thread (void * arg)
{
    thread_data * tdptr = static_cast<thread_data *>(arg);
    thread_data td = *tdptr;
    delete tdptr;                           /* delete (thread_data *) arg   */

    pthread_setspecific(m_current, td.td_t);

    thread * threadptr = static_cast<thread *>(td.td_t);
    threadptr->m_running = true;

    void * r = td.td_entry_point(td.td_arg);
    threadptr->m_running = false;
    return r;
}

/**
 *  This is very like non-portable.
 */

void
thread::clear_thread ()
{
    m_thread = 0;                   /* m_thread = nullptr; */
}

bool
thread::clone (entry_point ep, void * arg)
{
    thread_data * td = new (std::nothrow) thread_data;
    bool result = not_nullptr(td);
    if (result)
    {
        td->td_entry_point = ep;
        td->td_arg = arg;
        td->td_t = this;
        if (pthread_create(&m_thread, NULL, run_thread, td) != 0)
            result = false;
    }
    return result;
}

void
thread::detach ()
{
    pthread_detach(m_thread);
    clear_thread();
}

void
thread::cancel ( void )
{
    pthread_cancel(m_thread);
    clear_thread();
}

void
thread::join ( void )
{
    if (m_thread != 0)                          /* not portable             */
    {
        pthread_join(m_thread, NULL);           /* not joined yet, go ahead */
        clear_thread();
    }
}

/*
 *  Never call this unless some other thread will be calling 'join' on
 *  this one, otherwise, running() will return true even though the
 *  thread is dead.
 */

void
thread::exit (void * retval)
{
    m_running = false;
    pthread_exit(retval);
}

}           // namespace osc

/*
 * thread.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

