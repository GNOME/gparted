AM_CPPFLAGS =						\
	-I$(top_srcdir)/include				\
	$(GTHREAD_CFLAGS) 				\
	$(GTKMM_CFLAGS) 				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

AM_CFLAGS = -Wall	

AM_CXXFLAGS = -Wall	

sbin_PROGRAMS = gpartedbin

gpartedbin_SOURCES = \
	BlockSpecial.cc			\
	CopyBlocks.cc			\
	DMRaid.cc			\
	Device.cc			\
	DialogFeatures.cc		\
	DialogManageFlags.cc		\
	Dialog_Base_Partition.cc	\
	Dialog_Disklabel.cc		\
	Dialog_FileSystem_Label.cc	\
	Dialog_Partition_Copy.cc	\
	Dialog_Partition_Info.cc	\
	Dialog_Partition_Name.cc	\
	Dialog_Partition_New.cc		\
	Dialog_Partition_Resize_Move.cc	\
	Dialog_Progress.cc		\
	Dialog_Rescue_Data.cc		\
	DrawingAreaVisualDisk.cc	\
	FS_Info.cc			\
	FileSystem.cc			\
	Frame_Resizer_Base.cc		\
	Frame_Resizer_Extended.cc	\
	GParted_Core.cc			\
	HBoxOperations.cc		\
	LVM2_PV_Info.cc			\
	LUKS_Info.cc			\
	Mount_Info.cc			\
	Operation.cc			\
	OperationChangeUUID.cc		\
	OperationCheck.cc		\
	OperationCopy.cc		\
	OperationCreate.cc		\
	OperationDelete.cc		\
	OperationDetail.cc		\
	OperationFormat.cc		\
	OperationLabelFileSystem.cc	\
	OperationNamePartition.cc	\
	OperationResizeMove.cc		\
	Partition.cc			\
	PartitionLUKS.cc		\
	PartitionVector.cc		\
	PipeCapture.cc			\
	Proc_Partitions_Info.cc		\
	ProgressBar.cc			\
	SWRaid_Info.cc			\
	TreeView_Detail.cc		\
	Utils.cc			\
	Win_GParted.cc			\
	btrfs.cc			\
	exfat.cc			\
	ext2.cc				\
	f2fs.cc				\
	fat16.cc			\
	hfs.cc				\
	hfsplus.cc			\
	jfs.cc				\
	linux_swap.cc			\
	lvm2_pv.cc			\
	luks.cc				\
	main.cc				\
	nilfs2.cc			\
	ntfs.cc				\
	reiser4.cc			\
	reiserfs.cc			\
	udf.cc				\
	ufs.cc				\
	xfs.cc

gpartedbin_LDADD = $(GTHREAD_LIBS) $(GTKMM_LIBS)

