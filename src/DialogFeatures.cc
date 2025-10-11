/* Copyright (C) 2004-2006 Bart 'plors' Hakvoort
 * Copyright (C) 2008, 2009, 2010, 2011 Curtis Gedak
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


#include "DialogFeatures.h"
#include "FileSystem.h"
#include "GParted_Core.h"
#include "Utils.h"

#include <gtkmm/stock.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/box.h>
#include <glibmm/ustring.h>
#include <gtkmm/label.h>
#include <gtkmm/image.h>
#include <vector>


namespace GParted
{


DialogFeatures::DialogFeatures() 
{
	set_title( _("File System Support") ) ;
	//Set minimum dialog height so it fits on an 800x600 screen
	set_size_request( -1, 500 ) ;

	//initialize icons
	m_icon_yes = Utils::mk_pixbuf(*this, Gtk::Stock::APPLY, Gtk::ICON_SIZE_LARGE_TOOLBAR);
	m_icon_no = Utils::mk_pixbuf(*this, Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR);
	int width = 0;
	int height = 0;
	if (Gtk::IconSize::lookup(Gtk::ICON_SIZE_LARGE_TOOLBAR, width, height) && width > 0 && height > 0)
	{
		m_icon_blank = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
		m_icon_blank->fill(0xFFFFFF00);
	}

	//treeview
	Gtk::TreeView::Column *col;
	m_liststore_filesystems = Gtk::ListStore::create(m_treeview_filesystems_columns);
	m_treeview_filesystems.set_model(m_liststore_filesystems);
	m_treeview_filesystems.append_column(_("File System"), m_treeview_filesystems_columns.fsname);
	m_treeview_filesystems.append_column(_("Create"), m_treeview_filesystems_columns .create);
	col = Gtk::manage(new Gtk::TreeView::Column(_("Grow")));
	col->pack_start(m_treeview_filesystems_columns.grow, false);
	col->pack_start(m_treeview_filesystems_columns.online_grow, false);
	m_treeview_filesystems.append_column(*col);
	col = Gtk::manage(new Gtk::TreeView::Column(_("Shrink")));
	col->pack_start(m_treeview_filesystems_columns.shrink, false);
	col->pack_start(m_treeview_filesystems_columns.online_shrink, false);
	m_treeview_filesystems.append_column(*col);
	m_treeview_filesystems.append_column(_("Move"), m_treeview_filesystems_columns.move);
	m_treeview_filesystems.append_column(_("Copy"), m_treeview_filesystems_columns.copy);
	m_treeview_filesystems.append_column(_("Check"), m_treeview_filesystems_columns.check);
	col = Gtk::manage(new Gtk::TreeView::Column(_("Label")));
	col->pack_start(m_treeview_filesystems_columns.label, false);
	col->pack_start(m_treeview_filesystems_columns.online_label, false);
	m_treeview_filesystems.append_column(*col);
	m_treeview_filesystems.append_column(_("UUID"), m_treeview_filesystems_columns.uuid);
	m_treeview_filesystems.append_column(_("Required Software"), m_treeview_filesystems_columns.software);
	m_treeview_filesystems.get_selection()->set_mode(Gtk::SELECTION_NONE);
	m_treeview_filesystems.set_rules_hint(true);

	//scrollable file system list
	m_filesystems_scrolled.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	m_filesystems_scrolled.add(m_treeview_filesystems);

	Gtk::Box* filesystems_hbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)));
	filesystems_hbox->set_border_width(5);
	filesystems_hbox->pack_start(m_filesystems_scrolled);
	this->get_content_area()->pack_start(*filesystems_hbox);

	// File system support legend
	Gtk::Box* legend_hbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 6)));
	legend_hbox->set_border_width(5);

	Gtk::Box* legend_narrative_hbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)));
	Glib::ustring str_temp( _("This chart shows the actions supported on file systems.") ) ;
	str_temp += "\n" ;
	str_temp += _("Not all actions are available on all file systems, in part due to the nature of file systems and limitations in the required software.") ;
	Gtk::Label *label_narrative = Utils::mk_label(str_temp, true, true);
	label_narrative->set_max_width_chars(45);
	legend_narrative_hbox->pack_start(*label_narrative, Gtk::PACK_EXPAND_WIDGET);
	legend_hbox ->pack_start( *legend_narrative_hbox, Gtk::PACK_EXPAND_WIDGET, 6 ) ;

	// Icon legend
	Gtk::Box* icon_legend_vbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL)));

	Gtk::Box* available_both_hbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)));
	Gtk::Image* image_yes(Gtk::manage(new Gtk::Image(m_icon_yes)));
	available_both_hbox ->pack_start( *image_yes, Gtk::PACK_SHRINK ) ;
	image_yes = Gtk::manage(new Gtk::Image(m_icon_yes));
	available_both_hbox ->pack_start( *image_yes, Gtk::PACK_SHRINK ) ;
	available_both_hbox ->pack_start( *Utils::mk_label(
			/* TO TRANSLATORS:  Available offline and online
			* means that this action is valid for this file system when
			* it is both unmounted and mounted.
			*/
			_("Available offline and online")), Gtk::PACK_EXPAND_WIDGET ) ;
	icon_legend_vbox ->pack_start( *available_both_hbox ) ;

	Gtk::Box* available_online_hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
	Gtk::Image* image_no(Gtk::manage(new Gtk::Image(m_icon_no)));
	available_online_hbox->pack_start( *image_no, Gtk::PACK_SHRINK );
	image_yes = Gtk::manage(new Gtk::Image(m_icon_yes));
	available_online_hbox->pack_start( *image_yes, Gtk::PACK_SHRINK );
	available_online_hbox->pack_start( *Utils::mk_label(
			/* TO TRANSLATORS:  Available online only
			 * means that this action is valid for this file system only
			 * when it is mounted.
			 */
			_("Available online only")), Gtk::PACK_EXPAND_WIDGET );
	icon_legend_vbox->pack_start( *available_online_hbox );

	Gtk::Box* available_offline_hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
	image_yes = Gtk::manage(new Gtk::Image(m_icon_yes));
	available_offline_hbox ->pack_start( *image_yes, Gtk::PACK_SHRINK ) ;
	Gtk::Image* image_blank(Gtk::manage(new Gtk::Image(m_icon_blank)));
	available_offline_hbox ->pack_start( *image_blank, Gtk::PACK_SHRINK ) ;
	available_offline_hbox ->pack_start( *Utils::mk_label(
			/* TO TRANSLATORS:  Available offline only
			* means that this action is valid for this file system only
			* when it is unmounted.
			*/
			_("Available offline only")), Gtk::PACK_EXPAND_WIDGET ) ;
	icon_legend_vbox ->pack_start( *available_offline_hbox ) ;

	Gtk::Box* not_available_hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
	image_no = Gtk::manage(new Gtk::Image(m_icon_no));
	not_available_hbox ->pack_start( *image_no, Gtk::PACK_SHRINK ) ;
	image_blank = Gtk::manage(new Gtk::Image(m_icon_blank));
	not_available_hbox ->pack_start( *image_blank, Gtk::PACK_SHRINK ) ;
	not_available_hbox->pack_start(*Utils::mk_label(
			/* TO TRANSLATORS:  Not Available
			* means that this action is not valid for this file system.
			*/
			_("Not Available") ), Gtk::PACK_EXPAND_WIDGET ) ;
	icon_legend_vbox ->pack_start( *not_available_hbox ) ;
	legend_hbox ->pack_start( *icon_legend_vbox ) ;

	str_temp = "<b>" ;
	str_temp += _("Legend") ;
	str_temp += "</b>" ;
	m_legend_frame.set_label_widget(*Utils::mk_label(str_temp));
	m_legend_frame.set_shadow_type(Gtk::SHADOW_NONE);
	m_legend_frame.add(*legend_hbox);
	this->get_content_area()->pack_start(m_legend_frame, Gtk::PACK_SHRINK);

	/*TO TRANSLATORS: This is a button that will search for the software tools installed and then refresh the screen with the file system actions supported. */
	add_button( _("Rescan For Supported Actions"), Gtk::RESPONSE_OK );
	add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE ) ->grab_focus() ;
	show_all_children() ;
}


void DialogFeatures::load_filesystems(const std::vector<FS>& fss)
{
	m_liststore_filesystems->clear();

	// Fill the features chart with fully supported file systems.
	for (unsigned i = 0; i < fss.size(); i++)
	{
		if (GParted_Core::supported_filesystem(fss[i].fstype))
			load_one_filesystem(fss[i]);
	}

	// Find and add "other" at the end, for all the basic supported file systems.
	for (unsigned i = 0; i < fss.size(); i++)
	{
		if (fss[i].fstype == FS_OTHER)
		{
			load_one_filesystem(fss[i]);
			break;
		}
	}
}


void DialogFeatures::load_one_filesystem(const FS& fs)
{
	m_treerow = *(m_liststore_filesystems->append());
	m_treerow[m_treeview_filesystems_columns.fsname] = Utils::get_filesystem_string(fs.fstype);

	m_treerow[m_treeview_filesystems_columns.create       ] = fs.create             ? m_icon_yes : m_icon_no;
	m_treerow[m_treeview_filesystems_columns.grow         ] = fs.grow               ? m_icon_yes : m_icon_no;
	m_treerow[m_treeview_filesystems_columns.online_grow  ] = fs.online_grow        ? m_icon_yes : m_icon_blank;
	m_treerow[m_treeview_filesystems_columns.shrink       ] = fs.shrink             ? m_icon_yes : m_icon_no;
	m_treerow[m_treeview_filesystems_columns.online_shrink] = fs.online_shrink      ? m_icon_yes : m_icon_blank;
	m_treerow[m_treeview_filesystems_columns.move         ] = fs.move               ? m_icon_yes : m_icon_no;
	m_treerow[m_treeview_filesystems_columns.copy         ] = fs.copy               ? m_icon_yes : m_icon_no;
	m_treerow[m_treeview_filesystems_columns.check        ] = fs.check              ? m_icon_yes : m_icon_no;
	m_treerow[m_treeview_filesystems_columns.label        ] = fs.write_label        ? m_icon_yes : m_icon_no;
	m_treerow[m_treeview_filesystems_columns.online_label ] = fs.online_write_label ? m_icon_yes : m_icon_blank;
	m_treerow[m_treeview_filesystems_columns.uuid         ] = fs.write_uuid         ? m_icon_yes : m_icon_no;

	m_treerow[m_treeview_filesystems_columns.software] = Utils::get_filesystem_software(fs.fstype);
}


}  // namespace GParted
