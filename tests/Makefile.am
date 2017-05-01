AM_CPPFLAGS =  \
	-I$(top_srcdir)/include            \
	-I$(top_srcdir)/lib/gtest/include  \
	$(GTEST_CPPFLAGS)
AM_CXXFLAGS = -Wall $(GTEST_CXXFLAGS)
AM_LDFLAGS = $(GTEST_LDFLAGS)
LDADD =  \
	$(GTEST_LIBS)                                   \
	$(top_builddir)/lib/gtest/lib/libgtest_main.la  \
	$(top_builddir)/lib/gtest/lib/libgtest.la

# Programs to be built by "make check"
check_PROGRAMS =  \
	test_dummy         \
	test_BlockSpecial

# Test cases to be run by "make check"
TESTS = $(check_PROGRAMS)

test_dummy_SOURCES        = test_dummy.cc
test_BlockSpecial_SOURCES = test_BlockSpecial.cc ../src/BlockSpecial.cc
