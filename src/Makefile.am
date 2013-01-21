INCLUDES =						\
	$(GTHREAD_CFLAGS) 				\
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
	LVM2_PV_Info.cc			\
	Operation.cc			\
	OperationChangeUUID.cc		\
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
	TreeView_Detail.cc		\
	Utils.cc			\
	Win_GParted.cc			\
	btrfs.cc			\
	exfat.cc			\
	ext2.cc				\
	fat16.cc			\
	fat32.cc			\
	hfs.cc				\
	hfsplus.cc			\
	jfs.cc				\
	linux_swap.cc			\
	lvm2_pv.cc			\
	main.cc				\
	nilfs2.cc			\
	ntfs.cc				\
	reiser4.cc 			\
	reiserfs.cc			\
	ufs.cc				\
	xfs.cc				\
	PipeCapture.cc			\
	Copy_Blocks.cc

gpartedbin_LDFLAGS = -lparted

if USE_LIBPARTED_FS_RESIZE_LIBRARY
gpartedbin_LDFLAGS += -lparted-fs-resize
endif

gpartedbin_LDADD = $(GTHREAD_LIBS) $(GTKMM_LIBS)

