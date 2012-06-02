dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ac_prog_javah.html
dnl
AC_DEFUN([AC_PROG_JAVAH],[
AC_REQUIRE([AC_CANONICAL_SYSTEM])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
if test "x$JAVAPREFIX" = x; then
        test "x$JAVAH" = x && AC_CHECK_PROGS(JAVAH, gcjh$EXEEXT javah$EXEEXT)
else
        test "x$JAVAH" = x && AC_CHECK_PROGS(JAVAH, gcjh$EXEEXT javah$EXEEXT, $JAVAPREFIX)
fi
test "x$JAVAH" = x && AC_MSG_ERROR([no acceptable JNI header file generator found in \$PATH])
AC_PROVIDE([$0])dnl
if test x"`eval 'echo $ac_cv_path_JAVAH'`" != x ; then
  AC_TRY_CPP([#include <jni.h>],,[
    ac_save_CPPFLAGS="$CPPFLAGS"
changequote(, )dnl
    ac_dir=`echo $ac_cv_path_JAVAH | sed 's,\(.*\)/[^/]*/[^/]*$,\1/include,'`
    ac_machdep=`echo $build_os | sed 's,[-0-9].*,,' | sed 's,cygwin,win32,'`
changequote([, ])dnl
    CPPFLAGS="$ac_save_CPPFLAGS -I$ac_dir -I$ac_dir/$ac_machdep"
    AC_TRY_CPP([#include <jni.h>],
               ac_save_CPPFLAGS="$CPPFLAGS",
               AC_MSG_WARN([unable to include <jni.h>]))
    CPPFLAGS="$ac_save_CPPFLAGS"])
fi
])
