/* Copyright (C) 2010 Joan Lledó
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

/*
 * The dialog for mount old data
 * Reads the output of gpart and build the dialog
 * */

#ifndef GPARTED_DIALOG_RESCUE_DATA_H
#define GPARTED_DIALOG_RESCUE_DATA_H

#include "Device.h"
#include "Partition.h"
#include "PartitionVector.h"

#include <gtkmm/dialog.h>
#include <gtkmm/frame.h>
#include <vector>

namespace GParted
{

class Dialog_Rescue_Data : public Gtk::Dialog
{
public:
	Dialog_Rescue_Data();

	void init_partitions(Device *parentDevice, const Glib::ustring &buff);

	bool found_partitions();

private:
	void draw_dialog();
	void create_list_of_fs();
	bool is_overlaping(int nPart);
	void read_partitions_from_buffer();
	void check_overlaps(int nPart);
	void open_ro_view(Glib::ustring mountPoint);
	bool is_inconsistent(const Partition &part);

	Device *device; //Parent device
	PartitionVector partitions; //Partitions read from the buffer
	std::vector<int> overlappedPartitions;//List of guessed partitions that
										  //overlap active partitions
	Glib::ustring device_path;
	Sector device_length;
	bool inconsistencies; //If some of the guessed partitions is inconsistent
	int sector_size;
	std::vector<int>inconsistencies_list; //List of inconsistent partitions

	// Output of gpart command
	std::istringstream *buffer;

	//GUI stuff
	Gtk::Frame *frm;

	//Callback
	void on_view_clicked(int nPart);
};

} //GParted

#endif /* GPARTED_DIALOG_RESCUE_DATA_H */
