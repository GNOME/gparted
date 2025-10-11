/* Copyright (C) 2004 Bart
 * Copyright (C) 2010 Curtis Gedak
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

#include "DrawingAreaVisualDisk.h"
#include "Partition.h"
#include "PartitionLUKS.h"
#include "PartitionVector.h"
#include "Utils.h"

#include <gdkmm/general.h>

#define MAIN_BORDER 5
#define BORDER 4
#define SEP 4
#define HEIGHT 70 + 2 * MAIN_BORDER


namespace GParted
{


DrawingAreaVisualDisk::DrawingAreaVisualDisk()
 : m_selected_vp(nullptr), m_tot_sep(0), m_min_size(0), m_color_used(Utils::get_color(FS_USED)),
   m_color_unused(Utils::get_color(FS_UNUSED)), m_color_unallocated(Utils::get_color(FS_UNALLOCATED)),
   m_color_text("black")
{
	add_events( Gdk::BUTTON_PRESS_MASK );
	set_size_request( -1, HEIGHT ) ;
}


void DrawingAreaVisualDisk::load_partitions( const PartitionVector & partitions, Sector device_length )
{
	clear() ;	

	m_tot_sep = get_total_separator_px(partitions);
	set_static_data(partitions, m_visual_partitions, device_length);

	queue_resize() ;
}


void DrawingAreaVisualDisk::set_selected( const Partition * partition_ptr )
{
	set_selected_by_ptn(m_visual_partitions, partition_ptr);

	queue_draw() ;
}


void DrawingAreaVisualDisk::clear()
{
	m_visual_partitions.clear();
	m_selected_vp = nullptr;

	queue_resize() ;
}


int DrawingAreaVisualDisk::get_total_separator_px( const PartitionVector & partitions )
{
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
		if (partitions[t].type == TYPE_EXTENDED)
			return ( partitions[ t ] .logicals .size() -1 + partitions .size() -1 ) * SEP ;

	return ( partitions .size() -1 ) * SEP ;
}	


void DrawingAreaVisualDisk::set_static_data(const PartitionVector& partitions,
                                            std::vector<VisualPartition>& visual_partitions,
                                            Sector device_length)
{
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
	{
		visual_partitions.push_back(VisualPartition());

		visual_partitions.back().partition_ptr = & partitions[t];
		Sector partition_length = partitions[ t ] .get_sector_length() ;
		visual_partitions.back().fraction = partition_length / static_cast<double>(device_length);

		Glib::ustring color_str = Utils::get_color(partitions[t].get_filesystem_partition().fstype);
		visual_partitions.back().color.set( color_str );

		if (partitions[t].type == TYPE_EXTENDED)
			set_static_data( partitions[t].logicals,
			                 visual_partitions.back().logicals, partition_length );
		else
			visual_partitions .back() .pango_layout = create_pango_layout( 
				partitions[ t ] .get_path() + "\n" + Utils::format_size( partition_length, partitions[ t ] .sector_size ) ) ;
	}
}


int DrawingAreaVisualDisk::calc_length(std::vector<VisualPartition>& visual_partitions, int length_px)
{
	int calced_length = 0 ;

	for ( int t = 0 ; t < static_cast<int>( visual_partitions .size() ) ; t++ )
	{
		visual_partitions[ t ] .length = Utils::round( length_px * visual_partitions[ t ] .fraction ) ;
			
		if ( visual_partitions[ t ] .logicals .size() > 0 )
			visual_partitions[ t ] .length = 
				calc_length( visual_partitions[ t ] .logicals, 
					     visual_partitions[ t ] .length - (2 * BORDER) ) + (2 * BORDER) ;
		else if (visual_partitions[t].length < m_min_size)
			visual_partitions[t].length = m_min_size;

		calced_length += visual_partitions[ t ] .length ;
	}
	
	return calced_length + (visual_partitions .size() - 1) * SEP ;
}


void DrawingAreaVisualDisk::calc_position_and_height(std::vector<VisualPartition>& visual_partitions,
                                                     int start,
                                                     int border)
{
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		visual_partitions[ t ] .x_start = start ;
		visual_partitions[ t ] .y_start = border ;
		visual_partitions[ t ] .height = HEIGHT - ( border * 2 ) ;

		if ( visual_partitions[ t ] .logicals .size() > 0 )
			calc_position_and_height( visual_partitions[ t ] .logicals, 
						  visual_partitions[ t ] .x_start + BORDER,
  						  visual_partitions[ t ] .y_start + BORDER ) ;

		start += visual_partitions[ t ] .length + SEP ;
	}
}


void DrawingAreaVisualDisk::calc_usage(std::vector<VisualPartition>& visual_partitions)
{
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		if (visual_partitions[t].partition_ptr->fstype != FS_UNALLOCATED &&
		    visual_partitions[t].partition_ptr->type   != TYPE_EXTENDED    )
		{
			if ( visual_partitions[t].partition_ptr->sector_usage_known() )
			{
				visual_partitions[t].partition_ptr->get_usage_triple(
						visual_partitions[ t ] .length - BORDER *2,
						visual_partitions[ t ] .used_length,
						visual_partitions[ t ] .unused_length,
						visual_partitions[ t ] .unallocated_length ) ;
			}
			else
			{
				//Specifically show unknown figures as unused
				visual_partitions[ t ] .used_length        = 0 ;
				visual_partitions[ t ] .unused_length      = visual_partitions[ t ] .length - BORDER *2 ;
				visual_partitions[ t ] .unallocated_length = 0 ;
			}

			//used
			visual_partitions[ t ] .x_used_start = visual_partitions[ t ] .x_start + BORDER ;

			//unused
			visual_partitions[ t ] .x_unused_start = visual_partitions[ t ] .x_used_start
			                                       + visual_partitions[ t ] .used_length ;

			//unallocated
			visual_partitions[ t ] .x_unallocated_start = visual_partitions[ t ] .x_unused_start
			                                            + visual_partitions[ t ] .unused_length ;

			//y position and height
			visual_partitions[ t ] .y_usage_start = visual_partitions[ t ] .y_start + BORDER ;
			visual_partitions[ t ] .usage_height = visual_partitions[ t ] .height - (2 * BORDER) ;
		}

		if ( visual_partitions[ t ] .logicals .size() > 0 )
			calc_usage( visual_partitions[ t ] .logicals ) ;
	}
}
	

void DrawingAreaVisualDisk::calc_text(std::vector<VisualPartition>& visual_partitions)
{
	int length, height ;
	
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		if ( visual_partitions[ t ] .pango_layout )
		{
			//see if the text fits in the partition... (and if so, center the text..)
			visual_partitions[ t ] .pango_layout ->get_pixel_size( length, height ) ;
			if ( length < visual_partitions[ t ] .length - (2 * BORDER) - 2 )
			{
				visual_partitions[ t ] .x_text = visual_partitions[ t ] .x_start + 
					Utils::round( (visual_partitions[ t ] .length / 2) - (length / 2) ) ;

				visual_partitions[ t ] .y_text = visual_partitions[ t ] .y_start + 
					Utils::round( (visual_partitions[ t ] .height / 2) - (height / 2) ) ;
			}
			else
				visual_partitions[ t ] .x_text = visual_partitions[ t ] .y_text = 0 ;
		}

		if ( visual_partitions[ t ] .logicals .size() > 0 )
			calc_text( visual_partitions[ t ] .logicals ) ;
	}
}


void DrawingAreaVisualDisk::draw_partition(const Cairo::RefPtr<Cairo::Context>& cr,
                                           const VisualPartition& vp)
{
	//partition...
	Gdk::Cairo::set_source_rgba(cr, vp.color);
	cr->rectangle(vp.x_start,
	              vp.y_start,
	              vp.length,
	              vp.height);
	cr->fill();

	//used..
	if ( vp .used_length > 0 )
	{
		Gdk::Cairo::set_source_rgba(cr, m_color_used);
		cr->rectangle(vp.x_used_start,
		              vp.y_usage_start,
		              vp.used_length,
		              vp.usage_height);
		cr->fill();
	}
		
	//unused
	if ( vp .unused_length > 0 )
	{
		Gdk::Cairo::set_source_rgba(cr, m_color_unused);
		cr->rectangle(vp.x_unused_start,
		              vp.y_usage_start,
		              vp.unused_length,
		              vp.usage_height);
		cr->fill();
	}

	//unallocated
	if ( vp .unallocated_length > 0 )
	{
		Gdk::Cairo::set_source_rgba(cr, m_color_unallocated);
		cr->rectangle(vp.x_unallocated_start,
		              vp.y_usage_start,
		              vp.unallocated_length,
		              vp.usage_height);
		cr->fill();
	}

	//text
	if ( vp .x_text > 0 )
	{
		Gdk::Cairo::set_source_rgba(cr, m_color_text);
		cr->move_to(vp.x_text, vp.y_text);
		vp.pango_layout->show_in_cairo_context(cr);
	}
}


void DrawingAreaVisualDisk::draw_partitions(const Cairo::RefPtr<Cairo::Context>& cr,
                                            const std::vector<VisualPartition>& visual_partitions)
{
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		draw_partition(cr, visual_partitions[t]);

		if ( visual_partitions[ t ] .logicals .size() > 0 )
			draw_partitions(cr, visual_partitions[t].logicals);
	}
}


bool DrawingAreaVisualDisk::set_selected_by_coord(const std::vector<VisualPartition>& visual_partitions,
                                                  int x, int y)
{
	m_selected_vp = nullptr;

	for (unsigned int i = 0; i < visual_partitions.size(); i++)
	{
		if (visual_partitions[i].logicals.size() > 0)
		{
			if (set_selected_by_coord(visual_partitions[i].logicals, x, y))
				return true;  // Was selected in recursive call
		}

		if (visual_partitions[i].x_start <= x &&
		                    x < visual_partitions[i].x_start + visual_partitions[i].length &&
		    visual_partitions[i].y_start <= y &&
		                    y < visual_partitions[i].y_start + visual_partitions[i].height   )
		{
			m_selected_vp = &visual_partitions[i];
			return true;  // Selected
		}
	}

	return false;  // Not selected
}


bool DrawingAreaVisualDisk::set_selected_by_ptn(const std::vector<VisualPartition>& visual_partitions,
                                                const Partition* partition_ptr)
{
	m_selected_vp = nullptr;

	for (unsigned int i = 0; i < visual_partitions.size(); i++)
	{
		if (visual_partitions[i].logicals.size() > 0)
		{
			if (set_selected_by_ptn(visual_partitions[i].logicals, partition_ptr))
				return true;  // Was selected in recursive call
		}

		if (*visual_partitions[i].partition_ptr == *partition_ptr)
		{
			m_selected_vp = &visual_partitions[i];
			return true;  // Selected
		}
	}

	return false;  // Not selected
}


bool DrawingAreaVisualDisk::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	bool ret_val = Gtk::DrawingArea::on_draw(cr);

	cr->set_line_width(2.0);
	cr->set_line_join(Cairo::LINE_JOIN_MITER);  // default
	cr->set_line_cap(Cairo::LINE_CAP_BUTT);     // default
	cr->set_dash(std::vector<double>(1, 4.0), 0.0);

	draw_partitions(cr, m_visual_partitions);

	//selection 
	if (m_selected_vp)
	{
		Gdk::Cairo::set_source_rgba(cr, m_color_used);
		cr->rectangle(m_selected_vp->x_start + BORDER/2,
		              m_selected_vp->y_start + BORDER/2,
		              m_selected_vp->length - BORDER,
		              m_selected_vp->height - BORDER);
		cr->stroke();
	}

	return ret_val ;
}
	
bool DrawingAreaVisualDisk::on_button_press_event( GdkEventButton * event ) 
{
	bool ret_val = Gtk::DrawingArea::on_button_press_event( event ) ;

	set_selected_by_coord(m_visual_partitions, static_cast<int>(event->x), static_cast<int>(event->y));
	queue_draw() ;

	if (m_selected_vp)
	{
		signal_partition_selected.emit(m_selected_vp->partition_ptr, false);

		if ( event ->type == GDK_2BUTTON_PRESS ) 
			signal_partition_activated .emit() ;
		else if ( event ->button == 3 )  
			signal_popup_menu .emit( event ->button, event ->time ) ;
	}
	
	return ret_val ;
}

void DrawingAreaVisualDisk::on_size_allocate( Gtk::Allocation & allocation ) 
{
	Gtk::DrawingArea::on_size_allocate( allocation ) ;

	m_min_size = BORDER * 2 + 2;

	int available_size = allocation.get_width() - (2 * MAIN_BORDER);
	int calced = 0;
	int px_left;

	do
	{
		px_left = available_size - m_tot_sep;
		calced = available_size ; //for first time :)
		do 
		{
			px_left -= ( calced - available_size ) ;
			calced = calc_length(m_visual_partitions, px_left);
		}
		while ( calced > available_size && px_left > 0 ) ; 

		m_min_size--;
	}
	while (px_left <= 0 && m_min_size > 0);

	//due to rounding a few px may be lost. here we salvage them.. 
	if (m_visual_partitions.size() && calced > 0)
	{
		px_left = available_size - calced ;
		
		while ( px_left > 0 )
			px_left = spreadout_leftover_px(m_visual_partitions, px_left);
	}

	//and calculate the rest..
	calc_position_and_height(m_visual_partitions, MAIN_BORDER, MAIN_BORDER);  // 0, 0);
	calc_usage(m_visual_partitions);
	calc_text(m_visual_partitions);
	queue_draw();
}


int DrawingAreaVisualDisk::spreadout_leftover_px(std::vector<VisualPartition>& visual_partitions, int pixels)
{
	int extended = -1 ;

	for ( unsigned int t = 0 ; t < visual_partitions .size() && pixels > 0 ; t++ )
		if ( ! visual_partitions[ t ] .logicals .size() )
		{
			visual_partitions[ t ] .length++ ;
			pixels-- ;
		}
		else
			extended = t ;

	if ( extended > -1 && pixels > 0 )
	{	
		int actually_used = pixels - spreadout_leftover_px( visual_partitions[ extended ] .logicals, pixels ) ; 
		visual_partitions[ extended ] .length += actually_used ;
		pixels -= actually_used ;
	}
	
	return pixels ;
}


DrawingAreaVisualDisk::~DrawingAreaVisualDisk()
{
	clear() ;
}


}  // namespace GParted
