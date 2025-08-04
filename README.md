# README for Library Nsm66 0.1 2025-03-20

__Nsm66__ is a Non/New Session Manager support library based on the code
in the __Seq66__ project, but that is useful in other applications.
It provides more elegant methods in many cases. It is a work in
progress and the precursor to a potential Seq66 version 2.

Support sites (still in progress):

    *   https://ahlstromcj.github.io/
    *   https://github.com/ahlstromcj/ahlstromcj.github.io/wiki

# Major Features

The "nsm66" directory holds new-session-management code extracted from
the Non/New Session Manager applications.

    *   nsm:    Contains code in the "nsm" namespace. Some of the modules
                come from the Seq66 project. Additional modules support
                the refactored versions of nsmd, nsm-proxy, and jackpatch,
                plus an addition application, nsmctl, to provide some
                basic control from the command-line.
    *   osc:    Contains modules extracted directly from the NSM
                applications.
    *   tests:  Small test applications are provided to test and illustrate
                most of the classes.

    Note that a work.sh script is provided to simplify or clarify various
    operations such as cleaning, building, making a release, and installing
    or uninstalling the library.

##  Library Features

    *   Can be built using GNU C++ or Clang C++.
    *   Basic dependencies: Meson 1.1 and above; C++14 and above.
    *   The build system is Meson, and sample wrap files are provided
        for using Nsm66 as a C++ subproject.
    *   PDF documentation built from LaTeX.

##  Code

    *   The code is a mix of hard-core C++ and C-like functions.
    *   The C++ STL and advanced language features are used as much as
        possible
    *   C++14 is required for some of its features.
    *   The GNU and Clang C++ compilers are supported.
    *   Broken into modules for easier maintenance.
    *   Requires usage of the lib66 and cfg66 libraries (as Meson
        subprojects).

##  Fixes

    *   Improved the work.sh, added an --uninstall option.

##  Documentation

    *   A PDF developers guide is in progress.

## To Do

    *   Beef up testing.
    *   Beef up the LaTeX documentation.

## Recent Changes

    For all changes, see the NEWS file.

    *   Version 0.1.0:
        *   Usage of meson instead of autotools, cmake, or qmake.

// vim: sw=4 ts=4 wm=2 et ft=markdown
