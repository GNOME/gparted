INCLUDES =\
	$(GTKMM_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

AM_CXXFLAGS =\
	 -Wall\
	 -g

sbin_PROGRAMS = gparted

gparted_SOURCES = \
	main.cc\
	Win_GParted.cc\
	TreeView_Detail.cc\
	Dialog_Partition_Copy.cc\
	Dialog_Partition_Info.cc\
	Dialog_Partition_New.cc\
	Operation.cc\
	Partition.cc\
	Dialog_Progress.cc\
	Device.cc\
	VBox_VisualDisk.cc\
	Dialog_Partition_Resize_Move.cc\
	Dialog_About.cc\
	Frame_Resizer_Base.cc\
	Frame_Resizer_Extended.cc\
	Dialog_Base_Partition.cc

gparted_LDFLAGS = -lparted -lgthread-2.0 

gparted_LDADD = \
	$(GTKMM_LIBS)

