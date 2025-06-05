# Nonstandard package files for distribution
EXTRA_DIST =  \
	LICENSE                                  \
	README.md                                \
	include/gtest/internal/custom/README.md

# Google Test source files that we don't compile directly.  (Instead
# they are #included by gtest-all.cc).
# List files with:
#     cd lib/gtest/
#     find src include '(' -name '*.h' -o -name '*.cc' ')' -print | \
#     egrep -v 'gtest-all.cc|gtest_main.cc' | sort
GTEST_SRC =  \
	include/gtest/gtest-assertion-result.h              \
	include/gtest/gtest-death-test.h                    \
	include/gtest/gtest.h                               \
	include/gtest/gtest-matchers.h                      \
	include/gtest/gtest-message.h                       \
	include/gtest/gtest-param-test.h                    \
	include/gtest/gtest_pred_impl.h                     \
	include/gtest/gtest-printers.h                      \
	include/gtest/gtest_prod.h                          \
	include/gtest/gtest-spi.h                           \
	include/gtest/gtest-test-part.h                     \
	include/gtest/gtest-typed-test.h                    \
	include/gtest/internal/custom/gtest.h               \
	include/gtest/internal/custom/gtest-port.h          \
	include/gtest/internal/custom/gtest-printers.h      \
	include/gtest/internal/gtest-death-test-internal.h  \
	include/gtest/internal/gtest-filepath.h             \
	include/gtest/internal/gtest-internal.h             \
	include/gtest/internal/gtest-param-util.h           \
	include/gtest/internal/gtest-port-arch.h            \
	include/gtest/internal/gtest-port.h                 \
	include/gtest/internal/gtest-string.h               \
	include/gtest/internal/gtest-type-util.h            \
	src/gtest-assertion-result.cc                       \
	src/gtest.cc                                        \
	src/gtest-death-test.cc                             \
	src/gtest-filepath.cc                               \
	src/gtest-internal-inl.h                            \
	src/gtest-matchers.cc                               \
	src/gtest-port.cc                                   \
	src/gtest-printers.cc                               \
	src/gtest-test-part.cc                              \
	src/gtest-typed-test.cc

EXTRA_DIST += $(GTEST_SRC)

# We define the global AM_CPPFLAGS as everything we compile includes from these
# directories.
AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/include $(GTEST_CPPFLAGS)
AM_CXXFLAGS = $(GTEST_CXXFLAGS)
AM_LDFLAGS = $(GTEST_LDFLAGS)
LDADD = $(GTEST_LIBS)

# Build rules for libraries.
check_LTLIBRARIES = lib/libgtest.la lib/libgtest_main.la

lib_libgtest_la_SOURCES = src/gtest-all.cc

lib_libgtest_main_la_SOURCES = src/gtest_main.cc
lib_libgtest_main_la_LIBADD = lib/libgtest.la
