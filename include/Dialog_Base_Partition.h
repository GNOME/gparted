/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010 Curtis Gedak
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

#ifndef GPARTED_DIALOG_BASE_PARTITION_H
#define GPARTED_DIALOG_BASE_PARTITION_H


#include "Device.h"
#include "Frame_Resizer_Base.h"
#include "FileSystem.h"
#include "OptionComboBox.h"
#include "Partition.h"
#include "Utils.h"

#include <gtkmm/dialog.h>
#include <gtkmm/label.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/grid.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <sigc++/connection.h>
#include <memory>


namespace GParted
{


class Dialog_Base_Partition : public Gtk::Dialog
{
public:
	
	Dialog_Base_Partition(const Device& device);
	~Dialog_Base_Partition( ) ;

	Dialog_Base_Partition(const Dialog_Base_Partition& src) = delete;             // Copy construction prohibited
	Dialog_Base_Partition& operator=(const Dialog_Base_Partition& rhs) = delete;  // Copy assignment prohibited

	void Set_Resizer( bool extended ) ;
	const Partition & Get_New_Partition();

protected:
	enum SPINBUTTON {
		BEFORE	= 0,
		SIZE	= 1,
		AFTER	= 2
	};	
	
	enum CONFIRMBUTTON {
		RESIZE_MOVE	= 0,
		NEW		= 1,
		PASTE		= 2
	};

	void prepare_new_partition();
	static void snap_to_alignment(const Device& device, Partition& partition);
	static void snap_to_cylinder(const Device& device, Partition& partition);
	static void snap_to_mebibyte(const Device& device, Partition& partition);

	void Set_Confirm_Button( CONFIRMBUTTON button_type ) ;
	void Set_MinMax_Text( Sector min, Sector max ) ;

	double MB_PER_PIXEL ;
	Sector TOTAL_MB ;
	std::unique_ptr<Frame_Resizer_Base> m_frame_resizer_base;
	std::unique_ptr<Partition>          m_new_partition;

	Sector START; //the first sector of the first relevant partition ( this is either current or current -1 )  needed in Get_Resized_Partition()
	Sector total_length ; //total amount of sectors ( this can be up to 3 partitions...)

	Gtk::Box hbox_main;
	Gtk::SpinButton spinbutton_before, spinbutton_size, spinbutton_after;
	OptionComboBox combo_alignment;

	sigc::connection before_change_connection, size_change_connection, after_change_connection ;

	//used to enable/disable OKbutton...
	int ORIG_BEFORE, ORIG_SIZE, ORIG_AFTER ;

	//used to reserve space for Master or Extended Boot Record (1 MiB)
	int MIN_SPACE_BEFORE_MB ;

	static int MB_Needed_for_Boot_Record(const Partition& partition);

	//signal handlers
	void on_signal_move( int, int );
	void on_signal_resize( int, int, Frame_Resizer_Base::ArrowType );
	void on_spinbutton_value_changed( SPINBUTTON ) ;

	bool fixed_start, GRIP ;
	double before_value ;
	const Device& m_device;
	FS fs ;
	FS_Limits fs_limits;  // Working copy of file system min/max size limits

private:
	void update_button_resize_move_sensitivity();

	Gtk::Box vbox_resize_move;
	Gtk::Label label_minmax ;
	Gtk::Grid grid_resize;
	Gtk::Box hbox_grid;
	Gtk::Box hbox_resizer;
	Gtk::Button button_resize_move;  // Displayed only in the derived Resize/Move dialog
};


}  // namespace GParted


#endif /* GPARTED_DIALOG_BASE_PARTITION_H */
