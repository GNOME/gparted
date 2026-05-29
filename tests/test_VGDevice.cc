/* Copyright (C) 2026 Patrick Verner
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


#include "Device.h"
#include "Partition.h"
#include "VGDevice.h"
#include "gtest/gtest.h"

#include <memory>


namespace GParted
{


TEST(VGDeviceTest, DefaultValues)
{
	VGDevice vg;

	EXPECT_EQ(vg.pe_size,     -1);
	EXPECT_EQ(vg.total_pe,    -1);
	EXPECT_FALSE(vg.exported);
	EXPECT_FALSE(vg.partial);
	EXPECT_TRUE(vg.vg_name.empty());
	EXPECT_TRUE(vg.lv_paths.empty());
	EXPECT_EQ(vg.length,      0);
	EXPECT_EQ(vg.sector_size, 0);
	EXPECT_EQ(vg.cylinders,   0);
	EXPECT_TRUE(vg.disktype.empty());
}


TEST(DeviceTest, ClonePreservesFields)
{
	Device d;
	d.set_path("/dev/sda");
	d.length      = 1000;
	d.sector_size = 512;
	d.disktype    = "msdos";

	std::unique_ptr<Device> dup(d.clone());
	ASSERT_NE(dup.get(), nullptr);

	EXPECT_EQ(dup->get_path(),  "/dev/sda");
	EXPECT_EQ(dup->length,      1000);
	EXPECT_EQ(dup->sector_size, 512);
	EXPECT_EQ(dup->disktype,    "msdos");
}


TEST(VGDeviceTest, ClonePreservesAllFields)
{
	VGDevice vg;
	vg.set_path("Test_VG");
	vg.vg_name     = "Test_VG";
	vg.pe_size     = 4194304;
	vg.total_pe    = 100;
	vg.lv_paths.push_back("/dev/Test_VG/lvol0");
	vg.exported    = true;
	vg.partial     = false;
	vg.length      = 100;
	vg.sector_size = 4194304;

	std::unique_ptr<VGDevice> vg2(vg.clone());
	ASSERT_NE(vg2.get(), nullptr);

	EXPECT_EQ(vg2->get_path(),  "Test_VG");
	EXPECT_EQ(vg2->vg_name,     "Test_VG");
	EXPECT_EQ(vg2->pe_size,     4194304);
	EXPECT_EQ(vg2->total_pe,    100);
	ASSERT_EQ(vg2->lv_paths.size(), 1u);
	EXPECT_EQ(vg2->lv_paths[0], "/dev/Test_VG/lvol0");
	EXPECT_TRUE(vg2->exported);
	EXPECT_FALSE(vg2->partial);
	EXPECT_EQ(vg2->length,      100);
	EXPECT_EQ(vg2->sector_size, 4194304);
}


// Catches the regression where Device::clone() would be called without
// dispatching to VGDevice::clone(), producing a sliced Device with the
// VGDevice fields silently dropped.
TEST(VGDeviceTest, CloneViaBasePointerNoSlicing)
{
	VGDevice vg;
	vg.set_path("Test_VG");
	vg.vg_name = "Test_VG";
	vg.pe_size = 4194304;

	Device* dp = &vg;
	std::unique_ptr<Device> cp(dp->clone());
	ASSERT_NE(cp.get(), nullptr);

	VGDevice* vgcp = dynamic_cast<VGDevice *>(cp.get());
	ASSERT_NE(vgcp, nullptr) << "Slicing detected: clone() via Device* did "
	                            "not produce a VGDevice";
	EXPECT_EQ(vgcp->vg_name, "Test_VG");
	EXPECT_EQ(vgcp->pe_size, 4194304);
}


TEST(DeviceTest, CloneWithoutPartitionsPreservesFieldsAndDropsPartitions)
{
	Device d;
	d.set_path("/dev/sdz");
	d.length       = 1234567;
	d.heads        = 16;
	d.sectors      = 63;
	d.cylinders    = 1024;
	d.cylsize      = 8 * 63;
	d.model        = "Test Model";
	d.disktype     = "gpt";
	d.sector_size  = 512;
	d.max_prims    = 128;
	d.highest_busy = 0;
	d.readonly     = false;
	d.enable_partition_naming(36);
	d.partitions.push_back_adopt(new Partition());

	std::unique_ptr<Device> cp(d.clone_without_partitions());
	ASSERT_NE(cp.get(), nullptr);

	EXPECT_EQ(cp->get_path(),                      "/dev/sdz");
	EXPECT_EQ(cp->length,                          1234567);
	EXPECT_EQ(cp->heads,                           16);
	EXPECT_EQ(cp->sectors,                         63);
	EXPECT_EQ(cp->cylinders,                       1024);
	EXPECT_EQ(cp->cylsize,                         8 * 63);
	EXPECT_EQ(cp->model,                           "Test Model");
	EXPECT_EQ(cp->disktype,                        "gpt");
	EXPECT_EQ(cp->sector_size,                     512);
	EXPECT_EQ(cp->max_prims,                       128);
	EXPECT_EQ(cp->highest_busy,                    0);
	EXPECT_FALSE(cp->readonly);
	EXPECT_TRUE(cp->partition_naming_supported());
	EXPECT_EQ(cp->get_max_partition_name_length(), 36);
	EXPECT_EQ(cp->partitions.size(), 0u);
}


TEST(VGDeviceTest, CloneWithoutPartitionsPreservesAllFields)
{
	VGDevice vg;
	vg.set_path("Test_VG");
	vg.vg_name     = "Test_VG";
	vg.pe_size     = 4194304;
	vg.total_pe    = 100;
	vg.lv_paths.push_back("/dev/Test_VG/lvol0");
	vg.exported    = true;
	vg.partial     = false;
	vg.length      = 100;
	vg.sector_size = 4194304;
	vg.partitions.push_back_adopt(new Partition());

	std::unique_ptr<VGDevice> vg2(vg.clone_without_partitions());
	ASSERT_NE(vg2.get(), nullptr);

	EXPECT_EQ(vg2->get_path(),  "Test_VG");
	EXPECT_EQ(vg2->vg_name,     "Test_VG");
	EXPECT_EQ(vg2->pe_size,     4194304);
	EXPECT_EQ(vg2->total_pe,    100);
	ASSERT_EQ(vg2->lv_paths.size(), 1u);
	EXPECT_EQ(vg2->lv_paths[0], "/dev/Test_VG/lvol0");
	EXPECT_TRUE(vg2->exported);
	EXPECT_FALSE(vg2->partial);
	EXPECT_EQ(vg2->length,      100);
	EXPECT_EQ(vg2->sector_size, 4194304);
	EXPECT_EQ(vg2->partitions.size(), 0u);
}


// As above but for clone_without_partitions(); both virtuals must be
// overridden in VGDevice for the GUI's deep-copy path to work correctly.
TEST(VGDeviceTest, CloneWithoutPartitionsViaBasePointerNoSlicing)
{
	VGDevice vg;
	vg.set_path("Test_VG");
	vg.vg_name = "Test_VG";
	vg.pe_size = 4194304;
	vg.partitions.push_back_adopt(new Partition());

	const Device * dp = &vg;
	std::unique_ptr<const Device> cp(dp->clone_without_partitions());
	ASSERT_NE(cp.get(), nullptr);

	const VGDevice * vgcp = dynamic_cast<const VGDevice *>(cp.get());
	ASSERT_NE(vgcp, nullptr) << "Slicing detected: clone_without_partitions() "
	                            "via Device* did not produce a VGDevice";
	EXPECT_EQ(vgcp->vg_name, "Test_VG");
	EXPECT_EQ(vgcp->pe_size, 4194304);
	EXPECT_EQ(vgcp->partitions.size(), 0u);
}


}  // namespace GParted
