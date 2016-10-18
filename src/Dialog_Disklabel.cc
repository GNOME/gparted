/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009 Curtis Gedak
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

#include "Dialog_Disklabel.h"
#include "GParted_Core.h"

namespace GParted
{

Dialog_Disklabel::Dialog_Disklabel( const Device & device )
: image(Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_DIALOG)
{
	const Glib::ustring device_path = device .get_path() ;

	/*TO TRANSLATORS: dialogtitle, looks like Create partition table on /dev/hda */
	this ->set_title( String::ucompose( _("Create partition table on %1"), device_path ) );
	this ->set_has_separator( false ) ;
	this ->set_resizable( false );

	{
		Gtk::HBox* hbox(manage(new Gtk::HBox()));

		get_vbox()->pack_start(*hbox, Gtk::PACK_SHRINK);

		Gtk::VBox* vbox(manage(new Gtk::VBox()));

		vbox->set_border_width(10);
		hbox->pack_start(*vbox, Gtk::PACK_SHRINK);

		vbox->pack_start(image, Gtk::PACK_SHRINK);

		vbox = manage(new Gtk::VBox());
		vbox->set_border_width(10);
		hbox->pack_start(*vbox, Gtk::PACK_SHRINK);

		{
			Glib::ustring str_temp("<span weight=\"bold\" size=\"larger\">");

			/*TO TRANSLATORS: looks like WARNING:  This will ERASE ALL DATA on the ENTIRE DISK /dev/hda */
			str_temp += String::ucompose(_("WARNING:  This will ERASE ALL DATA on the ENTIRE DISK %1"), device_path);
			str_temp += "</span>\n";
			vbox->pack_start(*Utils::mk_label(str_temp), Gtk::PACK_SHRINK);

			hbox = manage(new Gtk::HBox(false, 5));
			hbox->set_border_width(5);
			str_temp = _("Select new partition table type:");
			str_temp += "\t";
			hbox->pack_start(*Utils::mk_label(str_temp), Gtk::PACK_SHRINK);
			vbox->pack_start(*hbox, Gtk::PACK_SHRINK);
		}

		//Default to MSDOS partition table type for disks < 2^32 sectors
		//  (2 TiB with 512 byte sectors) and GPT for larger disks.
		Glib::ustring default_label ;
		if ( device .length < 4LL * GIBIBYTE )
			default_label = "msdos" ;
		else
			default_label = "gpt" ;

		//create and add combo with partition table types (label types)
		this ->labeltypes = GParted_Core::get_disklabeltypes() ;

		bool set_active = false ;
		for ( unsigned int t = 0 ; t < labeltypes .size() ; t ++ )
		{
			combo_labeltypes .append_text( labeltypes[ t ] ) ;
			if ( default_label == labeltypes[ t ] )
			{
				combo_labeltypes .set_active( t ) ;
				set_active = true ;
			}
		}
		//Should be impossible for libparted to not known about MSDOS and GPT
		//  partition table types, but just in case fallback to activating the
		//  first item in the combobox.
		if ( ! set_active && labeltypes .size() )
			combo_labeltypes .set_active( 0 ) ;

		hbox->pack_start(combo_labeltypes, Gtk::PACK_SHRINK);
	}

	this ->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	this ->add_button( Gtk::Stock::APPLY, Gtk::RESPONSE_APPLY );
	this ->show_all_children() ;
}

Glib::ustring Dialog_Disklabel::Get_Disklabel() 
{
	return labeltypes[ combo_labeltypes .get_active_row_number() ] ;
}

}//GParted
