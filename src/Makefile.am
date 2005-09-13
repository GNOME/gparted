INCLUDES =\
	$(GTKMM_CFLAGS) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 

AM_CFLAGS =\
	 -Wall\
	 -g

AM_CXXFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = gparted

gparted_SOURCES = \
	Device.cc\
	Dialog_About.cc\
	Dialog_Base_Partition.cc\
	Dialog_Disklabel.cc \
	Dialog_Filesystems.cc \
	Dialog_Partition_Copy.cc\
	Dialog_Partition_Info.cc\
	Dialog_Partition_New.cc\
	Dialog_Partition_Resize_Move.cc\
	Dialog_Progress.cc\
	FileSystem.cc\
	Frame_Resizer_Base.cc\
	Frame_Resizer_Extended.cc\
	GParted_Core.cc\
	Operation.cc\
	Partition.cc\
	TreeView_Detail.cc\
	VBox_VisualDisk.cc\
	Win_GParted.cc\
	ext2.cc\
	ext3.cc\
	fat16.cc\
	fat32.cc\
	hfs.cc\
	hfsplus.cc\
	jfs.cc\
	linux_swap.cc\
	main.cc\
	ntfs.cc\
	reiser4.cc \
	reiserfs.cc\
	xfs.cc
	
gparted_LDFLAGS = -lparted -lgthread-2.0 

gparted_LDADD = \
	$(GTKMM_LIBS)

