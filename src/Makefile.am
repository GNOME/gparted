INCLUDES =						\
	$(GTKMM_CFLAGS) 				\
	-DGPARTED_DATADIR=\""$(datadir)"\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

AM_CFLAGS = -Wall	

AM_CXXFLAGS = -Wall	

sbin_PROGRAMS = gpartedbin

gpartedbin_SOURCES = \
	Device.cc			\
	Dialog_Base_Partition.cc	\
	Dialog_Disklabel.cc 		\
	Dialog_Rescue_Data.cc		\
	Dialog_Partition_Copy.cc	\
	Dialog_Partition_Info.cc	\
	Dialog_Partition_Label.cc	\
	Dialog_Partition_New.cc		\
	Dialog_Partition_Resize_Move.cc	\
	Dialog_Progress.cc		\
	DialogFeatures.cc		\
	DialogManageFlags.cc		\
	DrawingAreaVisualDisk.cc	\
	DMRaid.cc				\
	FileSystem.cc			\
	Frame_Resizer_Base.cc		\
	Frame_Resizer_Extended.cc	\
	FS_Info.cc				\
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
	Proc_Partitions_Info.cc		\
	SWRaid.cc				\
	TreeView_Detail.cc		\
	Utils.cc			\
	Win_GParted.cc			\
	btrfs.cc			\
	exfat.cc			\
	ext2.cc				\
	ext3.cc				\
	ext4.cc				\
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

