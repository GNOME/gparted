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
	test_dummy                      \
	test_BlockSpecial               \
	test_EraseFileSystemSignatures  \
	test_PasswordRAMStore           \
	test_PipeCapture                \
	test_SupportedFileSystems

# Test cases to be run by "make check"
TESTS = $(check_PROGRAMS)

noinst_HEADERS =  \
	common.h               \
	insertion_operators.h

gparted_core_OBJECTS =  \
	$(top_builddir)/src/BCache_Info.$(OBJEXT)           \
	$(top_builddir)/src/BlockSpecial.$(OBJEXT)          \
	$(top_builddir)/src/CopyBlocks.$(OBJEXT)            \
	$(top_builddir)/src/DMRaid.$(OBJEXT)                \
	$(top_builddir)/src/Device.$(OBJEXT)                \
	$(top_builddir)/src/FS_Info.$(OBJEXT)               \
	$(top_builddir)/src/FileSystem.$(OBJEXT)            \
	$(top_builddir)/src/GParted_Core.$(OBJEXT)          \
	$(top_builddir)/src/LUKS_Info.$(OBJEXT)             \
	$(top_builddir)/src/LVM2_PV_Info.$(OBJEXT)          \
	$(top_builddir)/src/Mount_Info.$(OBJEXT)            \
	$(top_builddir)/src/Operation.$(OBJEXT)             \
	$(top_builddir)/src/OperationCopy.$(OBJEXT)         \
	$(top_builddir)/src/OperationDetail.$(OBJEXT)       \
	$(top_builddir)/src/Partition.$(OBJEXT)             \
	$(top_builddir)/src/PartitionLUKS.$(OBJEXT)         \
	$(top_builddir)/src/PartitionVector.$(OBJEXT)       \
	$(top_builddir)/src/PasswordRAMStore.$(OBJEXT)      \
	$(top_builddir)/src/PipeCapture.$(OBJEXT)           \
	$(top_builddir)/src/Proc_Partitions_Info.$(OBJEXT)  \
	$(top_builddir)/src/ProgressBar.$(OBJEXT)           \
	$(top_builddir)/src/SupportedFileSystems.$(OBJEXT)  \
	$(top_builddir)/src/SWRaid_Info.$(OBJEXT)           \
	$(top_builddir)/src/Utils.$(OBJEXT)                 \
	$(top_builddir)/src/bcachefs.$(OBJEXT)              \
	$(top_builddir)/src/btrfs.$(OBJEXT)                 \
	$(top_builddir)/src/exfat.$(OBJEXT)                 \
	$(top_builddir)/src/ext2.$(OBJEXT)                  \
	$(top_builddir)/src/f2fs.$(OBJEXT)                  \
	$(top_builddir)/src/fat16.$(OBJEXT)                 \
	$(top_builddir)/src/hfs.$(OBJEXT)                   \
	$(top_builddir)/src/hfsplus.$(OBJEXT)               \
	$(top_builddir)/src/jfs.$(OBJEXT)                   \
	$(top_builddir)/src/linux_swap.$(OBJEXT)            \
	$(top_builddir)/src/luks.$(OBJEXT)                  \
	$(top_builddir)/src/lvm2_pv.$(OBJEXT)               \
	$(top_builddir)/src/minix.$(OBJEXT)                 \
	$(top_builddir)/src/nilfs2.$(OBJEXT)                \
	$(top_builddir)/src/ntfs.$(OBJEXT)                  \
	$(top_builddir)/src/reiser4.$(OBJEXT)               \
	$(top_builddir)/src/reiserfs.$(OBJEXT)              \
	$(top_builddir)/src/udf.$(OBJEXT)                   \
	$(top_builddir)/src/xfs.$(OBJEXT)

test_dummy_SOURCES        = test_dummy.cc

test_BlockSpecial_SOURCES = test_BlockSpecial.cc
test_BlockSpecial_LDADD   =  \
	$(top_builddir)/src/BlockSpecial.$(OBJEXT)  \
	$(LDADD)

test_EraseFileSystemSignatures_SOURCES =  \
	test_EraseFileSystemSignatures.cc  \
	common.cc                          \
	insertion_operators.cc
test_EraseFileSystemSignatures_LDADD =  \
	$(gparted_core_OBJECTS)                    \
	$(GTEST_LIBS)                              \
	$(top_builddir)/lib/gtest/lib/libgtest.la

test_PasswordRAMStore_SOURCES = test_PasswordRAMStore.cc
test_PasswordRAMStore_LDADD   =  \
	$(top_builddir)/src/PasswordRAMStore.$(OBJEXT)  \
	$(LDADD)

test_PipeCapture_SOURCES  =  \
	test_PipeCapture.cc  \
	common.cc
test_PipeCapture_LDADD    =  \
	$(top_builddir)/src/PipeCapture.$(OBJEXT)  \
	$(LDADD)

test_SupportedFileSystems_SOURCES =  \
	test_SupportedFileSystems.cc  \
	common.cc                     \
	insertion_operators.cc
test_SupportedFileSystems_LDADD   =  \
	$(gparted_core_OBJECTS)                    \
	$(GTEST_LIBS)                              \
	$(top_builddir)/lib/gtest/lib/libgtest.la
