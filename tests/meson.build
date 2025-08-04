#*****************************************************************************
# meson.build (nsm66/tests)
#-----------------------------------------------------------------------------
##
# \file        tests/meson.build
# \library     nsm66
# \author      Chris Ahlstrom
# \date        2025-01-29
# \updates     2025-03-14
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "nsm66" library. See the top-level meson.build
#  file for license information.
#
#-----------------------------------------------------------------------------

nsm_test_exe = executable(
   'nsm_test',
   sources : [ 'nsm_test.cpp' ],
   dependencies : [
      liblib66_library_dep,
      libcfg66_library_dep,
      libnsm66_dep
      ]
   )

test('Nsm66 Test', nsm_test_exe)
   
#****************************************************************************
# meson.build (nsm66/tests)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------

