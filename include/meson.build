#*****************************************************************************
# meson.build (nsm66/include)
#-----------------------------------------------------------------------------
##
# \file        include/meson.build
# \library     nsm66
# \author      Chris Ahlstrom
# \date        2025-01-29
# \updates     2025-04-05
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "nsm66" library. See the top-level meson.build
#  file for license information.
#
#-----------------------------------------------------------------------------

libnsm66_headers += files(
   'nsm66.hpp',
   'nsm/helpers.hpp',
   'nsm/nsmbase.hpp',
   'nsm/nsmclient.hpp',
   'nsm/nsmcodes.hpp',
   'nsm/nsmcontroller.hpp',
   'nsm/nsmctlclient.hpp',
   'nsm/nsmmessagesex.hpp',
   'nsm/nsmproxy.hpp',
   'nsm/nsmserver.hpp',
   'osc/endpoint.hpp',
   'osc/lowrapper.hpp',
   'osc/messages.hpp',
   'osc/method.hpp',
   'osc/osc_value.hpp',
   'osc/signal.hpp',
   'osc/thread.hpp'
   )

configure_file(
   input : 'nsm66-config.h.in',
   output : 'nsm66-config.h',
   install: true,
   install_dir : nsm66_includedir,
   configuration : cdata
   )

#****************************************************************************
# meson.build (nsm66/include)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
