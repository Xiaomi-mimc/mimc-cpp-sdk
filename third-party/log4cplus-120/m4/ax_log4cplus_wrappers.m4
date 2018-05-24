dnl LOG4CPLUS_CHECK_HEADER([header], [preprocessor symbol to define])
AC_DEFUN([LOG4CPLUS_CHECK_HEADER],
  [AH_TEMPLATE([$2])
   AC_CHECK_HEADER([$1], [AC_DEFINE([$2])])])

dnl LOG4CPLUS_CHECK_FUNCS([function], [preprocessor symbol to define])
AC_DEFUN([LOG4CPLUS_CHECK_FUNCS],
  [AH_TEMPLATE([$2])
   AC_CHECK_FUNCS([$1], [AC_DEFINE([$2])])])

dnl Define log4cplus_check_yesno_func().
AC_DEFUN([LOG4CPLUS_CHECK_YESNO_FUNC],
  [log4cplus_check_yesno_func() {
   AS_CASE(["$][1"],
    [yes], [],
    [no], [],
    [AC_MSG_ERROR([bad value "$][1" for "$][2"])])
   }])

dnl Check for use with AC_ARG_ENABLE macro.
AC_DEFUN([LOG4CPLUS_CHECK_YESNO],
  [AC_REQUIRE([LOG4CPLUS_CHECK_YESNO_FUNC])
   log4cplus_check_yesno_func "$1" "$2"])

dnl Define log4cplus_grep_cxxflags_for_optimization() shell function.
AC_DEFUN([LOG4CPLUS_GREP_CXXFLAGS_FOR_OPTIMIZATION],
  [AC_REQUIRE([AC_PROG_GREP])
   log4cplus_grep_cxxflags_for_optimization() {
     AS_ECHO_N(["$CXXFLAGS"]) dnl
       | $GREP -e ['\(^\|[[:space:]]\)-O\([^[:space:]]*\([[:space:]]\|$\)\)']dnl
       >/dev/null
   }])

dnl Add switch to CXXFLAGS if it does not contain any -Oxyz option.
AC_DEFUN([LOG4CPLUS_CXXFLAGS_ADD_IF_NO_OPTIMIZATION],
  [AC_REQUIRE([LOG4CPLUS_GREP_CXXFLAGS_FOR_OPTIMIZATION])
   AS_IF([log4cplus_grep_cxxflags_for_optimization],
     [],
     [AX_CXXFLAGS_GCC_OPTION([$1])])])
