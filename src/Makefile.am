INCLUDES =						\
	$(GTKMM_CFLAGS) 				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"		

AM_CFLAGS = -Wall	

AM_CXXFLAGS = -Wall	

sbin_PROGRAMS = gpartedbin

gpartedbin_SOURCES = \
	Device.cc			\
	Dialog_Base_Partition.cc	\
	Dialog_Disklabel.cc 		\
	Dialog_Partition_Copy.cc	\
	Dialog_Partition_Info.cc	\
	Dialog_Partition_Label.cc	\
	Dialog_Partition_New.cc		\
	Dialog_Partition_Resize_Move.cc	\
	Dialog_Progress.cc		\
	DialogFeatures.cc		\
	DialogManageFlags.cc		\
	DrawingAreaVisualDisk.cc	\
	FileSystem.cc			\
	Frame_Resizer_Base.cc		\
	Frame_Resizer_Extended.cc	\
	GParted_Core.cc			\
	HBoxOperations.cc		\
	Operation.cc			\
	OperationCopy.cc		\
	OperationCheck.cc		\
	OperationCreate.cc		\
	OperationDelete.cc		\
	OperationDetail.cc		\
	OperationFormat.cc		\
	OperationResizeMove.cc		\
	OperationLabelPartition.cc	\
	Partition.cc			\
	TreeView_Detail.cc		\
	Utils.cc			\
	Win_GParted.cc			\
	ext2.cc				\
	ext3.cc				\
	fat16.cc			\
	fat32.cc			\
	hfs.cc				\
	hfsplus.cc			\
	jfs.cc				\
	linux_swap.cc			\
	main.cc				\
	ntfs.cc				\
	reiser4.cc 			\
	reiserfs.cc			\
	ufs.cc				\
	xfs.cc
	
gpartedbin_LDFLAGS = -lparted -lgthread-2.0 

gpartedbin_LDADD = $(GTKMM_LIBS)

