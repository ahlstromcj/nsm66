#if ! defined NSM66_NSM_NSMSERVER_HPP
#define NSM66_NSM_NSMSERVER_HPP

/**
 * \file          nsmserver.hpp
 *
 *    This module provides macros for generating simple messages, MIDI
 *    parameters, and more.
 *
 * \library       nsm66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2025-01-29
 * \updates       2025-01-29
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Upcoming support for the Non Session Manager.
 */

#include "nsm/nsmbase.hpp"              /* nsm::nsmbase base class          */

namespace nsm
{

/**
 *  nsmbase is an NSM OSC server/client base class.
 */

class nsmserver : public nsmbase
{

public:

    /**
     *  These command values can indicate the pending operation.
     */

    enum class command
    {
        none,
        quit,
        kill,
        save,
        open,
        start,
        close,
        duplicate,
        cnew
    };

private:

    static std::string sm_nsm_default_ext;

public:

	nsmserver (const std::string & nsmurl);
	virtual ~nsmserver ()
    {
        // no code
    }

};          // class nsmserver

/*
 *  External helper functions.
 */

extern std::unique_ptr<nsmserver> create_nsmserver ();

}           // namespace nsm

#endif      // NSM66_NSM_NSMSERVER_HPP

/*
 * nsmserver.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

