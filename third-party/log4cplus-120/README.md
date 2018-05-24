% log4cplus README

Short Description
=================

[log4cplus] is a simple to use C++ logging API providing thread--safe,
flexible, and arbitrarily granular control over log management and
configuration.  It is modeled after the Java log4j API.

[log4cplus]: https://sourceforge.net/projects/log4cplus/


Latest Project Information
==========================

The latest up-to-date information for this project can be found at
[log4cplus] SourceForge project pages or [log4cplus wiki][4] on
SourceForge.  Please submit bugs, patches, feature requests, etc.,
there.

[4]: https://sourceforge.net/p/log4cplus/wiki/Home/


Mission statement
=================

The aim of this project is to develop log4j--like logging framework
for use in (primarily) C++. One of the major design goals is to avoid
huge dependencies (like Boost) in the core functionality and to use
standard C++ facilities instead. Where possible, the project takes
inspiration from other logging libraries, beside from log4j (e.g.,
from log4net, log4cxx, log4cpp).


Platform support
================

Log4cplus has been ported to and tested on the following platforms:

- Linux/AMD64 with 4.8.1 (Ubuntu/Linaro 4.8.1-10ubuntu8)
- Linux/AMD64 with Sun C++ 5.12 Linux_i386 2011/11/16
- Linux/AMD64 with Clang version 3.2-1~exp9ubuntu1
  (tags/RELEASE_32/final) (based on LLVM 3.2)
- Linux/AMD64 with Intel(R) C++ Intel(R) 64 Compiler XE for
  applications running on Intel(R) 64, Version 12.1 Build 20120410
- FreeBSD/AMD64 with GCC 3.4.6, 4.2.1 and 4.3.3
- Windows 7 with MS Visual Studio 2010 and 2012
- OpenSolaris 5.11/i386 with Sun C++ 5.10 SunOS_i386 128229-02
  2009/09/21, with `-library=stlport4`
- Solaris 5.10/Sparc with Sun C++ 5.8 2005/10/13, with
  `-library=stlport4` and with `-library=Cstd`.
- Solaris 5.10/Sparc with GCC 3.4.3 (csl-sol210-3_4-branch+sol_rpath)
- NetBSD 6.0/AMD64 with GCC 4.5.3 (NetBSD nb2 20110806)
- OpenBSD 5.2/AMD64 with GCC 4.2.1 20070719
- MacOS X 10.8 with GCC 4.2.1 (Apple Inc. build 5664)
- MacOS X 11.4.2 with GCC 4.2.1 (Based on Apple Inc. build 5658) (LLVM
  build 2336.11.00)
- HP-UX B.11.11 with HP ANSI C++ B3910B A.03.80
  (hppa2.0w-hp-hpux11.11)
- Haiku R1 Alpha 4.1 with GCC 4.6.3
- AIX 5.3 with IBM XL C/C++ for AIX, V11.1 (5724-X13)

The testing on the above listed platforms has been done at some point
in time with some version of source. Continuous testing is done only
on Linux platform offered by [Travis CI][11] service.


Configure script options
========================

`--enable-debugging`
--------------------

This option is disabled by default.  This option mainly affects GCC
builds but it also has some limited effect on non-GCC builds.  It
turns on debugging information generation, undefines `NDEBUG` symbol
and adds `-fstack-check` (GCC).


`--enable-warnings`
-------------------

This option is enabled by default.  It adds platform / compiler
dependent warning options to compiler command line.


`--enable-so-version`
---------------------

This option is enabled by default.  It enables SO version decoration
on resulting library file, e.g., the `.2.0.0` in
`liblog4cplus-1.2.so.2.0.0`.


`--enable-release-version`
--------------------------

This option is enabled by default.  It enables release version
decoration on the resulting library file, e.g., the `-1.2` in
`liblog4cplus-1.2.so.2.0.0`.


`--enable-symbols-visibility-options`
-------------------------------------

This option is enabled by default.  It enables use of compiler and
platform specific option for symbols visibility.  See also the
[Visibility][8] page on GCC Wiki.

[8]: http://gcc.gnu.org/wiki/Visibility


`--enable-profiling`
--------------------

This option is disabled by default.  This option adds profiling
information generation compiler option `-pg` to GCC and Sun CC /
Solaris Studio builds.


`--enable-threads`
------------------

This option is enabled by default.  It turns on detection of necessary
compiler and linker flags that enable POSIX threading support.

While this detection usually works well, some platforms still need
help with configuration by supplying additional flags to the
`configure` script.  One of the know deficiencies is Solaris Studio on
Linux.  See one of the later note for details.


`--with-working-locale`
-----------------------

This is one of three locale and `wchar_t`↔`char` conversion related
options.  It is disabled by default.

It is know to work well with GCC on Linux.  Other platforms generally
have lesser locale support in their implementations of the C++
standard library.  It is known not to work well on any BSDs.

See also docs/unicode.txt.


`--with-working-c-locale`
-------------------------

This is second of `wchar_t`↔`char` conversion related options.  It is
disabled by default.

It is known to work well on most Unix--like platforms, including
recent Cygwin.


`--with-iconv`
--------------

This is third of `wchar_t`↔`char` conversion related options.  It is
disabled by default.

The conversion using iconv() function always uses `"UTF-8"` and
`"WCHAR_T"` as source/target encoding.  It is known to work well on
platforms with GNU iconv.  Different implementations of `iconv()`
might not support `"WCHAR_T"` encoding selector.

Either system provided `iconv()` or library provided `libiconv()` are
detected and accepted.  Also both SUSv3 and GNU `iconv()` function
signatures are accepted.


`--with-qt`
-----------

This option is disabled by default.  It enables compilation of a
separate shared library (liblog4cplusqt4debugappender) that implements
`Qt4DebugAppender`.  It requires Qt4 and pkg-config to be installed.


Notes
=====

Compilation
-----------

On Unix--like platforms, [log4cplus] can be compiled using either
autotools based build system or using CMake build system. The
autotools based build system is considered to be primary for
Unix--like platforms.

On Windows, the primary build system is Visual Studio 2010 solution
and projects (`msvc10/log4cplus.sln`). This solution and associated
project files should update just fine to Visual Studio 2012 out of the
box. See also `scripts/msvc10_to_msvc11.cmd` and
`scripts/msvc10_to_msvc12.cmd` helper scripts that create
`msvc11/log4cplus.sln` and `msvc12/log4cplus.sln` respectively when
invoked on `msvc10/log4cplus.sln` from source root directory.

MinGW is supported by autotools based build system. CMake build system
is supported as well and it should be used to compile [log4cplus] with
older versions of Visual Studio or with less common compiler suites
(e.g., Embarcadero, Code::Blocks, etc.).


Cygwin/MinGW
------------

Some version of GCC (3.4.x and probably some of 4.x series too) on
Windows (both MinGW and Cygwin) produces lots of warnings of the form:

    warning: inline function 'void foo()' is declared as dllimport: attribute ignored

This can be worked around by adding `-Wno-attributes` option to GCC
command.  Unfortunately, not all affected version of GCC have this
option.


MinGW and MSVCRT version
------------------------

[log4cplus] can use functions like `_vsnprintf_s()` (Microsoft's
secure version of `vsnprintf()`). MinGW tool--chains (by default) link
to the system `MSVCRT.DLL`. Unfortunately, older systems, like Windows
XP, ship with `MSVCRT.DLL` that lacks these functions. It is possible
to compile [log4cplus] with MinGW tool--chains but _without_ using
Microsoft's secure functions by defining `__MSVCRT_VERSION__` to value
less than `0x900` and vice versa.

    $ ../configure CPPFLAGS="-D__MSVCRT_VERSION__=0x700"


Windows and TLS
---------------

[log4cplus] uses thread--local storage (TLS) for NDC, MDC and to
optimize use of some temporary objects.  On Windows there are two ways
to get TLS:

1. using `TlsAlloc()`, etc., functions
2. using `__declspec(thread)`

While method (2) generates faster code, it has
[some limitations prior to Windows Vista][tlsvista].  If
`log4cplus.dll` is loaded at run time using `LoadLibrary()` (or as a
dependency of such loaded library), then accessing
`__declspec(thread)` variables can cause general protection fault
(GPF) errors.  This is because Windows prior to Windows Vista do not
extend the TLS for libraries loaded at run time using `LoadLibrary()`.
To allow using the best available method, [log4cplus] enables the
method (2) by checking `_WIN32_WINNT >= 0x0600` condition, when
compiling [log4cplus] targeted to Windows Vista or later.

[tlsvista]: http://support.microsoft.com/kb/118816/en-us


Android, TLS and CMake
----------------------

[log4cplus] uses thread--local storage (TLS, see "Windows and TLS" for
details). On the Android platform, when [log4cplus] is being compiled using
the `android/android.toolchain.cmake`, you might get errors featuring the
`__emutls` symbol:


    global-init.cxx:268:46: error: log4cplus::internal::__emutls_t._ZN9log4cplus8internal3ptdE causes a section type conflict with log4cplus::internal::ptd

To work around this issue, invoke CMake with
`-DANDROID_FUNCTION_LEVEL_LINKING:BOOL=OFF` option.


iOS support
-----------

iOS support is based on CMake build. Use the scripts in `iOS` directory. The
iOS.cmake toolchain file was originally taken from [ios-cmake] project.

To build the library for iOS, being in current folder, perform the steps
below. For ARMv7 architecture:


~~~~{.bash}
./scripts/cmake_ios_armv7.sh
cmake --build ./build_armv7 --config "Release"
cmake --build ./build_armv7 --config "Debug"
~~~~

For i386 architecture:

~~~~{.bash}
./scripts/cmake_ios_i386.sh
cmake --build ./build_i386 --config "Release"
cmake --build ./build_i386 --config "Debug"
~~~~

Some versions of the iOS and/or its SDK have problems with thread-local storage
(TLS) and getting through CMake's environment detection phase. To work around
these issues, make these changes:

Edit the `iOS.cmake` file and add these two lines.

~~~~
set (CMAKE_CXX_COMPILER_WORKS TRUE)
set (CMAKE_C_COMPILER_WORKS TRUE)
~~~~

Add these lines. Customize them accordingly:

~~~~
set(MACOSX_BUNDLE_GUI_IDENTIFIER com.example)
set(CMAKE_MACOSX_BUNDLE YES)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
set(IPHONEOS_ARCHS arm64)
~~~~

If you have issues with TLS, also comment out these lines:

~~~~
set(LOG4CPLUS_HAVE_TLS_SUPPORT 1)
set(LOG4CPLUS_THREAD_LOCAL_VAR "__thread")
~~~~

[ios-cmake]: https://code.google.com/p/ios-cmake/


Threads and signals
-------------------

[log4cplus] is not safe to be used from asynchronous signals'
handlers.  This is a property of most threaded programmes in general.
If you are going to use [log4cplus] in threaded application and if you
want to use [log4cplus] from signal handlers then your only option is
to block signals in all threads but one that will handle all signals.
On POSIX platforms, this is possible using the `sigwait()` call.
[log4cplus] enables this approach by blocking all signals in any
threads created through its threads helpers.


IBM's XL C/C++ compiler
-----------------------

IBM's XL C/C++ compiler executable has [many variants][1].  To compile
[log4cplus] with threading support specify one of the compiler
variants that support threading using the `CXX` variable on
`configure` script command line.  E.g.:

    $ ../configure --enable-threads CXX=xlC_r

[1]: http://pic.dhe.ibm.com/infocenter/comphelp/v121v141/index.jsp?topic=%2Fcom.ibm.xlcpp121.aix.doc%2Fcompiler_ref%2Ftucmpinv.html


AIX reentrancy problem
----------------------

There appears to be a reentracy problem with AIX 5.3 and xlC 8 which
can result into a deadlock condition in some circumstances.  It is
unknown whether the problem manifests with other versions of either
the OS or the compiler, too.  The problem was initially reported in a
bug report [#103][2].

The core of the problem is that IBM's/xlC's standard C++ IOStreams
implementation uses global non recursive lock to protect some of its
state.  The application in the bug report was trying to do logging
using [log4cplus] from inside `overflow()` member function of a class
derived from `std::streambuf` class.  [log4cplus] itself uses
`std::ostringstream`.  This resulted into an attempt to recursively
lock the global non recursive lock and a deadlock.

[2]: http://sourceforge.net/p/log4cplus/bugs/103/


Solaris / SunOS
---------------

Some older version of this operating system might have problems
linking [log4cplus] due to [missing `__tls_get_addr`][3] in their
unpatched state.

[3]: https://groups.google.com/d/msg/comp.unix.solaris/AAMqkK0QZ6U/zlkVKA1L_QcJ


Solaris Studio
--------------

Solaris Studio compilers' default standard C++ library is very
non-standard.  It seems that it is not conforming enough in, e.g., Sun
C++ 5.12 Linux_i386 2011/11/16 (missing `std::time_t`, etc.), but it
works well enough on Solaris with Sun C++ 5.8 2005/10/13.  Thus
[log4cplus] adds `-library=stlport4` to the `CXXFLAGS` environment
variable, unless a switch matching `-library=(stlport4|stdcxx4|Cstd)`
is already present there.  If you want to override the default
supplied by [log4cplus], just set it into `CXXFLAGS` on `configure`
script command line.

Solaris Studio supports the `__func__` symbol which can be used by
[log4cplus] to record function name in logged events.  To enable this
feature, add `-features=extensions` switch to `CXXFLAGS` for
`configure` script.  Subsequently, you will have to add this switch to
your application's build flags as well.


Solaris Studio on GNU/Linux
---------------------------

The autotools and our `configure.ac` combo does not handle Solaris
Studio compiler on Linux well enough and needs a little help with
configuration of POSIX threads:

~~~~{.bash}
$ COMMON_FLAGS="-L/lib/x86_64-linux-gnu/ \
-L/usr/lib/x86_64-linux-gnu/ -mt=yes -O"

$ ../configure --enable-threads=yes \
CC=/opt/solarisstudio12.3/bin/cc \
CXX=/opt/solarisstudio12.3/bin/CC \
CFLAGS="$COMMON_FLAGS" \
CXXFLAGS="$COMMON_FLAGS" \
LDFLAGS="-lpthread"
~~~~


HP-UX with `aCC`
----------------

It is necessary to turn on C++98 mode of `aCC` by providing the `-AA`
flag:

    $ ../configure --enable-threads=yes CXXFLAGS="-AA"


HP-UX with `aCC` on IA64
------------------------

There is a problem on IA64 HP-UX with `aCC` (HP C/aC++ B3910B
A.06.20). The problem manifests as
[unsatisfied symbols during linking of `loggingserver`][9]:

    ld: Unsatisfied symbol "virtual table of loggingserver::ClientThread" in file loggingserver.o

The problem appears to be a deficiency in `aCC` and its support of
`__declspec(dllexport)`. To work around this issue, add
`--disable-symbols-visibility-options` to `configure` script command
line:

    $ ../configure --disable-symbols-visibility-options \
    --enable-threads=yes CXXFLAGS="-AA"

[9]: http://h30499.www3.hp.com/t5/Languages-and-Scripting/Building-Log4cplus-fails-with-quot-ld-Unsatisfied-symbol-virtual/td-p/6261411#.UoHtgPmet8G


Haiku
-----

Haiku is supported with GCC 4+. The default GCC version in Haiku is
set to version 2 (based on GCC 2.95.x). To change the default GCC
version to version 4, please run `setgcc gcc4` command. This is to
avoid linking errors like this:

    main.cpp:(.text.startup+0x54a): undefined reference to `_Unwind_Resume'

Running the command switches the _current_ GCC version to version 4.
This change is permanent and global. See also Haiku ticket
[#8368](http://dev.haiku-os.org/ticket/8368).


Qt4 / Win32 / MSVC
------------------

In order to use [log4cplus] in Qt4 programs it is necessary to set
following option: `Treat WChar_t As Built in Type: No (/Zc:wchar_t-)`

Set this option for [log4cplus] project and `Qt4DebugAppender` project
in MS Visual Studio.  Remember to use Unicode versions of [log4cplus]
libraries with Qt.  It is also necessary to make clear distinction
between debug and release builds of Qt project and [log4cplus].  Do
not use [log4cplus] release library with debug version of Qt program
and vice versa.

For registering Qt4DebugAppender library at runtime, call this
function: `log4cplus::Qt4DebugAppender::registerAppender()`

Add these lines to qmake project file for using [log4cplus] and
`Qt4DebugAppender`:

    INCLUDEPATH += C:\log4cplus\include
    win32 {
        CONFIG(debug, debug|release) {
            LIBS += -LC:\log4cplus\msvc10\Win32\bin.Debug_Unicode -llog4cplusUD
            LIBS += -LC:\log4cplus\msvc10\Win32\bin.Debug_Unicode -llog4cplus-Qt4DebugAppender
        } else {
            LIBS += -LC:\log4cplus\msvc10\Win32\bin.Release_Unicode -llog4cplusU
            LIBS += -LC:\log4cplus\msvc10\Win32\bin.Release_Unicode -llog4cplus-Qt4DebugAppender
        }
    }


Qt / GCC
--------

You might encounter the following error during compilation with
`--with-qt` option:

    qglobal.h:943: error: ISO C++ does not support 'long long'

This is caused by `-pedantic` option that [log4cplus] adds to
`CXXFLAGS` when compiling with GCC.  To work around this issue, add
`-Wno-long-long` GCC option to `CXXFLAGS`.


OpenBSD
-------

OpenBSD 5.2 and earlier have a bug in `wcsftime()` function in
handling of `%%` and `%N` where N is not a supported formatter. This
is fixed in OpenBSD 5.3 and later. This shows as failing
`timeformat_test` when [log4cplus] is compiled with `-DUNICODE` in
`CXXFLAGS`.


`LOG4CPLUS_*_FMT()` and UNICODE
-------------------------------

Beware, the `%s` specifier does not work the same way on Unix--like
platforms as it does on Windows with Visual Studio. With Visual Studio
the `%s` specifier changes its meaning conveniently by printing
`wchar_t` string when used with `wprintf()` and `char` strings when
used with `printf()`.  On the other hand, Unix--like platforms keeps
the meaning of printing `char` strings when used with both `wprintf()`
and `printf()`.  It is necessary to use `%ls` (C99) specifier or `%S`
(SUSv2) specifier to print `wchar_t` strings on Unix--like platforms.

The common ground for both platforms appears to be use of `%ls` and
`wchar_t` string to print strings with unmodified formatting string
argument on both Unix--like platforms and Windows. The conversion of
`wchar_t` back to `char` then depends on C locale.


C++11 support
-------------

[log4cplus] contains small amount of code that uses C++11 (ISO/IEC
14882:2011 standard) language features.  C++11 features are used only
if C++11 support is detected during compile time.  Compiling
[log4cplus] with C++11 compiler and standard library and using it with
C++03 (ISO/IEC 14882:2003 standard) application is not supported.


Unsupported compilers
---------------------

[log4cplus] does not support too old or broken C++ compilers:

- Visual C++ prior to 7.1
- GCC prior to 3.2
- Older versions of Borland/CodeGear/Embarcadero C++ compilers


Unsupported platforms
---------------------

[log4cplus] requires some minimal set of C and/or C++ library
functions. Some systems/platforms fail to provide these functions and
thus [log4cplus] cannot be supported there:

- Windows CE -- missing implementations of `<time.h>` functions


Bug reporting instructions
--------------------------

For successful resolution of reported bugs, it is necessary to provide enough information:

- [log4cplus]
    - What is the exact release version or Git branch and revision?
    - What is the build system that you are building [log4cplus] with
      (Autotools, Visual Studio solution and its version, CMake).
    - Autotools -- Provide `configure` script parameters and environment
      variables, attach generated `config.log` and `defines.hxx` files.
    - CMake -- Provide build configuration (`Release`, `Debug`,
      `RelWithDebInfo`) and non--default `CMAKE_*` variables values.
    - Visual Studio -- Provide project configuration (`Release`,
      `Release_Unicode`, `Debug`, `Debug_Unicode`) and Visual Studio version.
    - Provide target OS and CPU. In case of MinGW, provide its exact compiler
      distribution -- TDM? Nuwen? Other?

- [log4cplus] client application
    - Are you using shared library [log4cplus] or as static library [log4cplus]?
    - Is [log4cplus] linked into an executable or into a shared library (DLL or
      SO)?
    - If [log4cplus] is linked into a shared library, is this library
      loaded dynamically or not?
    - What library file you are linking your application with --
      `log4cplus.lib`, `log4cplusUSD.lib`, `liblog4cplus.dll.a`, etc., on
      Windows?
    - Is your application is using Unicode/`wchar_t` or not?
    - Provide any error messages.
    - Provide stack trace.
    - Provide [log4cplus] properties/configuration files.
    - Provide a self--contained test case, if possible.


License
=======

This library is licensed under the Apache Public License 2.0 and two
clause BSD license.  Please read the included LICENSE file for
details.


Contributions
=============

[log4cplus] (bug tracker, files, wiki) is hosted on SourceForge,
except for [log4cplus source][5], which is hosted on Github. This
allows the project to integrate with [Travis CI][11] service offered
by Github.

[5]: https://sourceforge.net/p/log4cplus/source-code-link/
[11]: https://sourceforge.net/p/log4cplus/travis-ci/


Patches
-------

Anybody can contribute to log4cplus development. If you are
contributing a source code change, use a reasonable form: a merge
request of a Git branch or a patch file attached to a ticket in
[Bugs tracker][6] or sent to [log4cplus-devel mailing list][7]. Unless
it is obvious, always state what branch or release tarball is your
patch based upon.

[6]: https://sourceforge.net/p/log4cplus/bugs/
[7]: mailto:log4cplus-devel@lists.sourceforge.net


Formatting
----------

Please use common sense. Follow the style of surrounding code. You can
use the following Emacs style that is based on Microsoft's style as a
guide line:

~~~~{.commonlisp}
;; Custom MS like indentation style.
(c-add-style "microsoft"
             '("stroustrup"
               (c-offsets-alist
                (innamespace . -)
                (inline-open . 0)
                (inher-cont . c-lineup-multi-inher)
                (arglist-cont-nonempty . +)
                (template-args-cont . +))))
~~~~
