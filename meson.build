#*****************************************************************************
# meson.build (nsm66)
#-----------------------------------------------------------------------------
##
# \file        meson.build
# \library     nsm66
# \author      Chris Ahlstrom
# \date        2025-01-29
# \updates     2025-10-05
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "nsm66" library. It was part of the libs66
#  collection of libraries, but is now separate and stands alone.
#
#  The "nsm66" library is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by the
#  Free Software Foundation, either version 3 of the License, or (at your
#  option) any later version.
#
#  The "nsm66" library is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
#  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
#  more details.
#
#  You should have received a copy of the GNU General Public License along with
#  the "nsm66" library. If not, see <https://www.gnu.org/licenses/>.
#
#-----------------------------------------------------------------------------

project('nsm66',
   [ 'c', 'cpp' ],
   version : '0.1',
   license : 'GPLv3',
   meson_version : '>=1.1.0',
   default_options : [
      'c_std=c11', 'cpp_std=c++14', 'warning_level=3',
      'default_library=both'
      ]
   )

#-----------------------------------------------------------------------------
# Conditional enabling of the Potext translation library.
#-----------------------------------------------------------------------------

nsm66_use_potext = false
if get_option('potext')

   nsm66_use_potext = true
   add_project_arguments('-DNSM66_USE_POTEXT', language : [ 'c', 'cpp' ])

endif

#-----------------------------------------------------------------------------
# Information for this sub-project.
#-----------------------------------------------------------------------------
#
#  Tricky: the *.pc file must have a base name matching the name of the
#  subproject *.wrap file.
#
#-----------------------------------------------------------------------------

nsm66_pkg_description = 'Refactors Seq66 NSM support modules.'
nsm66_pkg_version = meson.project_version()
nsm66_api_version = '0.1'
nsm66_info_date = '2025-03-08'
nsm66_info_build_type = get_option('buildtype')
nsm66_info_build_root = meson.project_build_root()
nsm66_info_lib_type = get_option('default_library')
nsm66_info_project_root = meson.project_source_root()
nsm66_debug = get_option('buildtype').contains('debug')
nsm66_project_base = 'nsm66-@0@'.format(nsm66_api_version)
nsm66_pkgconfig_name = 'libnsm66'
nsm66_subproj = meson.is_subproject()

# -fno-inline-functions does not work.

if nsm66_debug
   if not nsm66_subproj
      add_project_arguments(
         [ '-DDEBUG', '-fno-inline-functions' ],
         language : 'cpp'
      )
   endif
else
   if not nsm66_subproj
      add_project_arguments(
         [ '-DNDEBUG' ],
         language : 'cpp'
      )
   endif
endif

#-----------------------------------------------------------------------------
# Easy access to directory options. Commented out the unnecessary ones.
# Many of these definitions are simply informative.
#-----------------------------------------------------------------------------

nsm66_dir = 'nsm66-@0@'.format(nsm66_api_version)
nsm66_prefix = get_option('prefix')
nsm66_bindir = join_paths(nsm66_prefix, get_option('bindir'))
nsm66_includedir = join_paths(nsm66_prefix, get_option('includedir'), nsm66_dir)
nsm66_libdir = join_paths(nsm66_prefix, get_option('libdir'), nsm66_dir)
nsm66_datadir = join_paths(nsm66_prefix, get_option('datadir'), nsm66_dir)
nsm66_docdir = join_paths(nsm66_prefix, nsm66_datadir, 'doc', nsm66_dir)
libraries_top = meson.project_source_root()
libnsm66_include_top = join_paths(libraries_top, 'include')

#-----------------------------------------------------------------------------
# The *.pc file goes into $libdir/nsm66-0.x/pkgconfig instead of
# $libdir/pkgconfig as the meson documentation asserts.  Not sure why, but
# wiring the expected directory here works.
#-----------------------------------------------------------------------------

alt_pkgconfig_libdir = join_paths(
   nsm66_prefix, get_option('libdir'), 'pkgconfig'
   )

#-----------------------------------------------------------------------------
# libnsm66_headers provides the list of all headers in the project. These are
# currently all installable, public headers. They are set in
# include/meson.build.
#
# libnsm66_sources provides the source-code files. They are set in
# src/meson.build.
#-----------------------------------------------------------------------------

libnsm66_headers = []
libnsm66_sources = []
build_args = []

#-----------------------------------------------------------------------------
# Unlike the cfg66 library, this library will have platform-dependent
# characteristics.
#-----------------------------------------------------------------------------

os_unix     = false
os_freebsd  = false
os_linux    = false
os_win32    = false
os_darwin   = false

if host_machine.system() == 'darwin'
   os_darwin = true
elif host_machine.system() == 'windows'
   os_win32 = true
elif host_machine.system() == 'linux'
   os_linux = true
elif host_machine.system() == 'freebsd'
   os_freebsd = true
endif

os_unix = not os_win32

#-----------------------------------------------------------------------------
# Each include file directory is added to the list passed to the compiler.
#
# We prefer to use the style '#include "osc/header.hpp", hence no need
# for these:
#
#     'include/nsm',
#     'include/osc'
#
#-----------------------------------------------------------------------------

libnsm66_includes = include_directories('.', 'include')

#-----------------------------------------------------------------------------
# Used by the nsm66_version() function.
#-----------------------------------------------------------------------------

build_args += [
   '-DNSM66_NAME=' + '"' + meson.project_name() + '"',
   '-DNSM66_VERSION=' + '"' + meson.project_version() + '"'
   ]

#-----------------------------------------------------------------------------
# Only make public interfaces visible
#-----------------------------------------------------------------------------

'''
if target_machine.system() == 'windows' or target_machine.system() == 'cygwin'
   build_args += '-DLIBPUBLIC="__declspec(dllexport)"'
else
   build_args += '-DLIBPUBLIC=__attribute__((visibility("default")))'
endif
'''

#-----------------------------------------------------------------------------
# Set up some "HAVE" macros.  By default the header file is generated in the
# "build" directory. If configure_file() is moved to include/meson.build,
# the the header goes into "build/include", which is what we want.
#-----------------------------------------------------------------------------

cc = meson.get_compiler('cpp')
cdata = configuration_data()

cdata.set('limits_h', cc.has_header('limits.h'))
cdata.set('lo_h', cc.has_header('lo/lo.h'))

#-----------------------------------------------------------------------------
# Potential sub-projects
#-----------------------------------------------------------------------------

if nsm66_use_potext

   libpotext_dep = subproject('libpotext').get_variable('libpotext_dep')
   libpotext_library_dep = dependency(
      'libpotext', fallback : [ 'libpotext', 'libpotext_dep' ]
      )

endif

liblib66_dep = subproject('liblib66').get_variable('liblib66_dep')
liblib66_library_dep = dependency(
   'liblib66', fallback : [ 'liblib66', 'liblib66_dep' ]
   )

libcfg66_dep = subproject('libcfg66').get_variable('libcfg66_dep')
libcfg66_library_dep = dependency(
   'libcfg66', fallback : [ 'libcfg66', 'libcfg66_dep' ]
   )

# libxpc66_library_dep = dependency(
#    'libxpc66', fallback : [ 'libxpc66', 'libxpc66_dep' ]
#    )

liblo_dep = dependency('liblo', required : true)
threads_dep = dependency('threads', required : true)

#-----------------------------------------------------------------------------
# Sub-directories. 'tests' has to be descended into later.
#-----------------------------------------------------------------------------

subdir('include')
subdir('src')

#-----------------------------------------------------------------------------
# Dependencies on Linux
#-----------------------------------------------------------------------------
#
# 'liblo', version : '>=1.0.0', required : true
#
#-----------------------------------------------------------------------------

empty_depends = [ ]

if os_linux

system_depends = [ liblo_dep, threads_dep ]

endif

#-----------------------------------------------------------------------------
# We recommemd using a recent version of meson by installing it outside the
# Linux distro's repository.  For example, meson on Ubuntu is pretty damn
# old.
#
# The choice of library type is of some import for debugging:
#
#     library:          Defaults to a shared library. The User setting is
#                       'default_library'.  See project() above.
#     shared_library:   Cannot see source files until stepped into.
#     static_library:   Can see the source files when debugging.
#
# This one causes the linker to not see the library functions, wotta pain:
#
#     gnu_symbol_visibility: 'hidden'
#
# Restore it later. Also see the https://gcc.gnu.org/wiki/Visibility site.
#
#-----------------------------------------------------------------------------

if nsm66_use_potext

   nsm66_library_build = library(
      nsm66_project_base,
      libnsm66_sources,
      install : not nsm66_subproj,
      install_dir : nsm66_libdir,
      c_args : build_args,
      cpp_args : build_args,
      dependencies : [
         system_depends,
         libcfg66_library_dep,
         liblib66_library_dep,
         libpotext_library_dep
         ],
      include_directories : [ libnsm66_includes ]
      )

else

   nsm66_library_build = library(
      nsm66_project_base,
      libnsm66_sources,
      install : not nsm66_subproj,
      install_dir : nsm66_libdir,
      c_args : build_args,
      cpp_args : build_args,
      dependencies : [
         system_depends,
         libcfg66_library_dep,
         liblib66_library_dep,
         ],
      include_directories : [ libnsm66_includes ]
      )

endif

#-----------------------------------------------------------------------------
# Make this library usable as a Meson subproject.  This allows a subproject to
# easily specify how it should be used. This makes it interchangeable with the
# same dependency that is provided externally (i.e. installed) by the system.
#-----------------------------------------------------------------------------

libnsm66_dep = declare_dependency(
   include_directories : [ libnsm66_includes ],
   link_with : [ nsm66_library_build ]
   )

#-----------------------------------------------------------------------------
# Make this library usable from the system's package manager.
#-----------------------------------------------------------------------------

install_headers(libnsm66_headers, subdir : nsm66_dir, preserve_path : true)

#-----------------------------------------------------------------------------
# Package config
#-----------------------------------------------------------------------------
#  On Arch Linux, the .pc file is installed to /usr/local/lib/pkgconfig.
#  On Ubuntu, the .pc file is installed to
#     /usr/local/lib/x86_64-linux-gnu/pkgconfig.
#-----------------------------------------------------------------------------

if not nsm66_subproj

   pkg_mod = import('pkgconfig')
   pkg_mod.generate(
      nsm66_library_build,
      filebase : nsm66_pkgconfig_name,
      version : nsm66_pkg_version,
      name : nsm66_project_base,
      description : nsm66_pkg_description,
      install_dir : alt_pkgconfig_libdir,
      subdirs : nsm66_dir,
      libraries : nsm66_library_build,
      url : 'https://github.com/ahlstromcj/nsm66',
      )

# TODO: figure out where to put and install the logo

   install_data(
      files(
         'ChangeLog',
         'LICENSE.md',
         'README.md',
         'doc/nsm66-library-guide.pdf'
         ),
      install_dir : get_option('docdir')
      )

   if get_option('enable-tests')
      subdir('tests')
   endif

endif

#-----------------------------------------------------------------------------
# Info
#-----------------------------------------------------------------------------

summary(
   {
      'Date' : nsm66_info_date,
      'Filebase' : nsm66_project_base,
      'Version' : nsm66_pkg_version,
      'Base' : nsm66_project_base,
      'Description' : nsm66_pkg_description,
      'Source Root' : nsm66_info_project_root,
      'Headers Root' : libnsm66_include_top,
      'Sub Dir' : nsm66_project_base,
      'Build Type' : nsm66_info_build_type,
      'Build Root' : nsm66_info_build_root,
      'Library Type' : nsm66_info_lib_type,
      'Subproject' : nsm66_subproj
   },
   section : 'Nsm66 Project'
   )

summary(
   {
      'prefix' : nsm66_prefix,
      'includedir' : nsm66_includedir,
      'bindir' : nsm66_bindir,
      'libdir' : nsm66_libdir,
      'datadir' : nsm66_datadir,
      'docdir' : nsm66_docdir
   },
   section : 'Nsm66 Install Directories'
   )

#****************************************************************************
# meson.build (nsm66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
