/* Copyright (C) 2004 Bart
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

#ifndef GPARTED_DRAWINGAREAVISUALDISK_H
#define GPARTED_DRAWINGAREAVISUALDISK_H

#include "Partition.h"
#include "PartitionVector.h"

#include <gtkmm/drawingarea.h>

namespace GParted
{

class DrawingAreaVisualDisk : public Gtk::DrawingArea
{
public:
	DrawingAreaVisualDisk();
	~DrawingAreaVisualDisk();

	void load_partitions( const PartitionVector & partitions, Sector device_length );
	void set_selected( const Partition * partition_ptr );
	void clear() ;

	// Public signals for interclass communication
	sigc::signal<void, const Partition *, bool> signal_partition_selected;
	sigc::signal< void > signal_partition_activated ;
	sigc::signal< void, unsigned int, unsigned int > signal_popup_menu ;
	
private:
	struct visual_partition ; 

	//private functions	
	int get_total_separator_px( const PartitionVector & partitions );

	void set_static_data( const PartitionVector & partitions,
	                      std::vector<visual_partition> & visual_partitions,
	                      Sector length );
	int calc_length( std::vector<visual_partition> & visual_partitions, int length_px ) ;
	void calc_position_and_height( std::vector<visual_partition> & visual_partitions, int start, int border ) ;
	void calc_usage( std::vector<visual_partition> & visual_partitions ) ;
	void calc_text( std::vector<visual_partition> & visual_partitions ) ;

	void draw_partition(const Cairo::RefPtr<Cairo::Context>& cr,
	                    const visual_partition& vp);
	void draw_partitions(const Cairo::RefPtr<Cairo::Context>& cr,
	                     const std::vector<visual_partition>& visual_partitions);

	void set_selected( const std::vector<visual_partition> & visual_partitions, int x, int y ) ;
	void set_selected( const std::vector<visual_partition> & visual_partitions, const Partition * partition_ptr );

	int spreadout_leftover_px( std::vector<visual_partition> & visual_partitions, int pixels ) ;
	
	//overridden signalhandlers
	bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
	bool on_button_press_event( GdkEventButton * event ) ;
	void on_size_allocate( Gtk::Allocation & allocation ) ;

	//variables
	struct visual_partition
	{
		double fraction ;		//Partition size as a fraction of containing disk or extended partition size
		int x_start, length ;
		int y_start, height ;
		int x_used_start, used_length ;
		int x_unused_start, unused_length ;
		int x_unallocated_start, unallocated_length ;
		int y_usage_start, usage_height ;
		int x_text, y_text ;

		Gdk::RGBA color;
		Glib::RefPtr<Pango::Layout> pango_layout;

		// Pointer to real partition.  (Alias to element in Win_GParted::display_partitions[] vector).
		const Partition * partition_ptr;

		std::vector<visual_partition> logicals ;

		visual_partition()
		{
			fraction = 0.0 ;
			x_start = length =
			y_start = height =
			x_used_start = used_length =
			x_unused_start = unused_length =
			x_unallocated_start = unallocated_length =
			y_usage_start = usage_height =
			x_text = y_text = 0 ;
			
			pango_layout .clear() ;
			logicals .clear() ;
		}

		~visual_partition()
		{
			pango_layout .clear() ;
			logicals .clear() ;
		}
	};

	std::vector<visual_partition> visual_partitions ;
	const visual_partition * selected_vp ;
	int TOT_SEP, MIN_SIZE ;

	Gdk::RGBA color_used;
	Gdk::RGBA color_unused;
	Gdk::RGBA color_unallocated;
	Gdk::RGBA color_text;
};

} //GParted

#endif /* GPARTED_DRAWINGAREAVISUALDISK_H */
