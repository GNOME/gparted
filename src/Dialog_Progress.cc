/* Copyright (C) 2004 Bart
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

#include "Dialog_Progress.h"
#include "Device.h"
#include "GParted_Core.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "ProgressBar.h"
#include "Utils.h"

#include <glibmm/miscutils.h>
#include <glibmm/main.h>
#include <glibmm/ustring.h>
#include <gtkmm/stock.h>
#include <gtkmm/main.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/filechooserdialog.h>
#include <vector>


namespace GParted
{

Dialog_Progress::Dialog_Progress(const std::vector<Device>& devices, const std::vector<Operation *>& operations)
 : m_devices(devices), m_curr_op(0)
{
	this ->set_title( _("Applying pending operations") ) ;
	this ->operations = operations ;
	succes = true ;
	cancel = false ;
	warnings = 0 ;

	fraction = 1.00 / operations .size() ;
	this->property_default_width() = 700;

	{
		Gtk::Box *vbox(manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL)));

		vbox->set_border_width(10);
		this->get_content_area()->pack_start(*vbox, Gtk::PACK_EXPAND_WIDGET);

		Glib::ustring str_temp(_("Depending on the number and type of operations this might take a long time."));
		str_temp += "\n";
		vbox->pack_start(*Utils::mk_label(str_temp), Gtk::PACK_SHRINK);

		label_current.set_alignment(Gtk::ALIGN_START);
		vbox->pack_start(label_current, Gtk::PACK_SHRINK);

		progressbar_current.set_pulse_step(0.01);
		progressbar_current.set_show_text();
		vbox->pack_start(progressbar_current, Gtk::PACK_SHRINK);

		label_current_sub.set_alignment(Gtk::ALIGN_START);
		vbox->pack_start(label_current_sub, Gtk::PACK_SHRINK);

		vbox->pack_start(*Utils::mk_label("<b>" + Glib::ustring(_("Completed Operations:")) + "</b>"),
					Gtk::PACK_SHRINK);

		progressbar_all.set_show_text();
		vbox->pack_start(progressbar_all, Gtk::PACK_SHRINK);

		//create some icons here, instead of recreating them every time
		icon_execute = Utils::mk_pixbuf(*this, Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		icon_success = Utils::mk_pixbuf(*this, Gtk::Stock::APPLY, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		icon_error = Utils::mk_pixbuf(*this, Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		icon_info = Utils::mk_pixbuf(*this, Gtk::Stock::INFO, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		icon_warning = Utils::mk_pixbuf(*this, Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_LARGE_TOOLBAR);

		treestore_operations = Gtk::TreeStore::create( treeview_operations_columns);
		treeview_operations.set_model(treestore_operations);
		treeview_operations.set_headers_visible(false);
		treeview_operations.set_rules_hint(true);
		treeview_operations.set_size_request(700, 250);
		treeview_operations.append_column("", treeview_operations_columns.operation_description);
		treeview_operations.append_column("", treeview_operations_columns.elapsed_time);
		treeview_operations.append_column("", treeview_operations_columns.status_icon);

		treeview_operations.get_column(0)->set_expand(true);
		treeview_operations.get_column(0)->set_cell_data_func(
			*(treeview_operations.get_column(0)->get_first_cell()),
			sigc::mem_fun(*this, &Dialog_Progress::on_cell_data_description) );

		//fill 'er up
		for (unsigned int i = 0; i < operations.size(); ++i)
		{
			this->operations[i]->operation_detail.set_description(operations[i]->description, FONT_BOLD);
			this->operations[i]->operation_detail.set_treepath(Utils::num_to_str(i));

			treerow = *(treestore_operations->append());
			treerow[treeview_operations_columns.operation_description] =
				this->operations[i]->operation_detail.get_description();
		}

		scrolledwindow.set_shadow_type(Gtk::SHADOW_ETCHED_IN);
		scrolledwindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		scrolledwindow.set_size_request (700, 250);
		scrolledwindow.add(treeview_operations);

		expander_details.set_label("<b>" + Glib::ustring(_("Details")) + "</b>");
		expander_details.set_use_markup(true);
		expander_details.add(scrolledwindow);

		vbox ->pack_start(expander_details, Gtk::PACK_EXPAND_WIDGET);
		vbox ->set_spacing(5);
	}

	cancelbutton = this ->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	
	this ->signal_show() .connect( sigc::mem_fun(*this, &Dialog_Progress::on_signal_show) );
	this ->show_all_children() ;
}

void Dialog_Progress::on_signal_update( const OperationDetail & operationdetail ) 
{
	Gtk::TreeModel::iterator iter = treestore_operations ->get_iter( operationdetail .get_treepath() ) ;

	//i added the second check after get_iter() in gtk+-2.10 seems to behave differently from gtk+-2.8 
	if ( iter && treestore_operations ->get_string( iter ) == operationdetail .get_treepath() )
	{
		Gtk::TreeRow treerow = *iter ;

		treerow[ treeview_operations_columns .operation_description ] = operationdetail .get_description() ;
		treerow[ treeview_operations_columns .elapsed_time ] = operationdetail .get_elapsed_time() ;

		switch ( operationdetail .get_status() )
		{
			case STATUS_EXECUTE:
				treerow[ treeview_operations_columns .status_icon ] = icon_execute ;
				break ;
			case STATUS_SUCCESS:
				treerow[treeview_operations_columns.status_icon] = icon_success;
				break ;
			case STATUS_ERROR:
				treerow[ treeview_operations_columns .status_icon ] = icon_error ;
				break ;
			case STATUS_INFO:
				treerow[ treeview_operations_columns .status_icon ] = icon_info ;
				break ;
			case STATUS_WARNING:
				treerow[treeview_operations_columns.status_icon] = icon_warning;
				warnings++ ;
				break ;
			case STATUS_NONE:
				static_cast< Glib::RefPtr<Gdk::Pixbuf> >(
					treerow[ treeview_operations_columns .status_icon ] ) .clear() ;
				break ;
		}

		//update the gui elements..
		if ( operationdetail .get_status() == STATUS_EXECUTE )
			label_current_sub_text = operationdetail .get_description() ;

		ProgressBar & progressbar_src = operationdetail.get_progressbar();
		if ( progressbar_src.running() )
		{
			if ( pulsetimer.connected() )
				pulsetimer.disconnect();
			progressbar_current.set_fraction( progressbar_src.get_fraction() );
			progress_text = progressbar_src.get_text();
		}
		else
		{
			if ( ! pulsetimer.connected() )
			{
				pulsetimer = Glib::signal_timeout().connect(
				                sigc::mem_fun( *this, &Dialog_Progress::pulsebar_pulse ), 100 );
				progress_text.clear();
			}
		}
		update_gui_elements();
	}
	else//it's an new od which needs to be added to the model.
	{
		unsigned int pos = operationdetail .get_treepath() .rfind( ":" ) ;
		if ( pos < operationdetail .get_treepath() .length() )
			iter = treestore_operations ->get_iter( operationdetail .get_treepath() 
							.substr( 0, operationdetail .get_treepath() .rfind( ":" ) ) ) ;
		else
			iter = treestore_operations ->get_iter( operationdetail .get_treepath() ) ;

		if ( iter)
		{
			treestore_operations ->append( static_cast<Gtk::TreeRow>( *iter) .children() ) ;
			on_signal_update( operationdetail ) ;
		}
	}
}

void Dialog_Progress::update_gui_elements()
{
	label_current_sub .set_markup( "<i>" + label_current_sub_text + "</i>\n" ) ;
	
	//To ensure progress bar height remains the same, add a space in case message is empty
	progressbar_current.set_text( progress_text + " " );
}

bool Dialog_Progress::pulsebar_pulse()
{
	progressbar_current.pulse();
	return true;
}

void Dialog_Progress::on_signal_show()
{
	for (m_curr_op = 0; m_curr_op < operations.size() && succes && ! cancel; m_curr_op++)
	{
		operations[m_curr_op]->operation_detail.signal_update.connect(
			sigc::mem_fun( this, &Dialog_Progress::on_signal_update ) ) ;

		label_current.set_markup("<b>" + operations[m_curr_op]->description + "</b>");

		progressbar_all.set_text(Glib::ustring::compose(_("%1 of %2 operations completed"),
		                                                m_curr_op, operations.size()));
		progressbar_all.set_fraction(fraction * m_curr_op > 1.0 ? 1.0 : fraction * m_curr_op);

		treerow = treestore_operations ->children()[m_curr_op];

		//set status to 'execute'
		operations[m_curr_op]->operation_detail.set_status(STATUS_EXECUTE);

		//set focus...
		treeview_operations .set_cursor( static_cast<Gtk::TreePath>( treerow ) ) ;

		succes = signal_apply_operation.emit(operations[m_curr_op]);

		//set status (succes/error) for this operation
		operations[m_curr_op]->operation_detail.set_success_and_capture_errors(succes);
	}
	
	//add save button
	this ->add_button( _("_Save Details"), Gtk::RESPONSE_OK ) ; //there's no enum for SAVE
	
	//replace 'cancel' with 'close'
	canceltimer.disconnect();
	delete cancelbutton;
	cancelbutton = 0;
	this ->add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE );

	pulsetimer.disconnect();

	if ( cancel )
	{
		progressbar_current .set_text( _("Operation cancelled") ) ;
		progressbar_current .set_fraction( 0.0 ) ;
	}
	else
	{
		//hide 'current operation' stuff
		label_current .hide() ;
		progressbar_current .hide() ;
		label_current_sub .hide() ;
	}

	//deal with succes/error...
	if ( succes )
	{
		Glib::ustring str_temp(_("All operations successfully completed"));

		if ( warnings > 0 )
			str_temp += " ("
			         +  Glib::ustring::compose( ngettext("%1 warning", "%1 warnings", warnings), warnings )
			         +  ")" ;

		progressbar_all .set_text( str_temp ) ;
		progressbar_all .set_fraction( 1.0 ) ;
	}
	else 
	{
		expander_details .set_expanded( true ) ;

		if ( ! cancel )
		{
			Gtk::MessageDialog dialog( *this,
						   _("An error occurred while applying the operations"),
						   false,
						   Gtk::MESSAGE_ERROR,
						   Gtk::BUTTONS_OK,
						   true ) ;
			Glib::ustring str_temp(_("See the details for more information."));

			str_temp += "\n\n<i><b>" + Glib::ustring( _("IMPORTANT") ) + "</b>\n" ;
			str_temp += _("If you want support, you need to provide the saved details!") ;
			str_temp += "\n";
			str_temp += Glib::ustring::compose(
			                    /* TO TRANSLATORS: looks like
			                     * See https://gparted.org/save-details.htm for more information.
			                     */
			                    _("See %1 for more information."),
			                    "https://gparted.org/save-details.htm" );
			str_temp += "</i>";
		
			dialog .set_secondary_text( str_temp, true ) ;
			dialog .run() ;
		}
	} 
}

void Dialog_Progress::on_cell_data_description( Gtk::CellRenderer * renderer, const Gtk::TreeModel::iterator & iter )
{
	dynamic_cast<Gtk::CellRendererText *>( renderer ) ->property_markup() = 
		static_cast<Gtk::TreeRow>( *iter )[ treeview_operations_columns .operation_description ] ;
}

bool Dialog_Progress::cancel_timeout()
{
	if (--cancel_countdown) {
		/*TO TRANSLATORS: looks like  Force Cancel (5)
		 *  where the number represents a count down in seconds until the button is enabled */
		cancelbutton->set_label( Glib::ustring::compose( _("Force Cancel (%1)"), cancel_countdown ) );
	} else {
		cancelbutton->set_label( _("Force Cancel") );
		canceltimer.disconnect();
		cancelbutton->set_sensitive();
		return false;
	}
	return true;
}

void Dialog_Progress::on_cancel()
{
	Gtk::MessageDialog dialog( *this,
				   _("Are you sure you want to cancel the current operation?"),
				   false,
				   Gtk::MESSAGE_QUESTION,
				   Gtk::BUTTONS_NONE,
				   true ) ;
		
	dialog .set_secondary_text( _("Canceling an operation might cause SEVERE file system damage.") ) ;

	dialog .add_button( _("Continue Operation"), Gtk::RESPONSE_NONE ) ;
	dialog .add_button( _("Cancel Operation"), Gtk::RESPONSE_CANCEL ) ;
	
	if ( !cancel || dialog .run() == Gtk::RESPONSE_CANCEL )
	{
		cancelbutton->set_sensitive( false );
		if (!cancel) {
			cancel_countdown = 5;
			/*TO TRANSLATORS: looks like  Force Cancel (5)
			 *  where the number represents a count down in seconds until the button is enabled */
			cancelbutton->set_label( Glib::ustring::compose( _("Force Cancel (%1)"), cancel_countdown ) );
			canceltimer = Glib::signal_timeout().connect(
				sigc::mem_fun(*this, &Dialog_Progress::cancel_timeout), 1000 );
		}
		else cancelbutton->set_label( _("Force Cancel") );
		operations[m_curr_op]->operation_detail.signal_cancel.emit(cancel);
		cancel = true;
	}
}

void Dialog_Progress::on_save()
{
	Gtk::FileChooserDialog dialog( _("Save Details"), Gtk::FILE_CHOOSER_ACTION_SAVE ) ;
	dialog .set_transient_for( *this ) ;
	dialog .set_current_folder( Glib::get_home_dir() ) ;
	dialog .set_current_name( "gparted_details.htm" ) ;
	dialog .set_do_overwrite_confirmation( true ) ; 
	dialog .add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL ) ; 
	dialog .add_button( Gtk::Stock::SAVE, Gtk::RESPONSE_OK ) ; //there's no enum for SAVE

	if ( dialog .run() == Gtk::RESPONSE_OK )
	{
		std::ofstream out( dialog .get_filename() .c_str() ) ;
		if ( out )
		{
			//Write out proper HTML start
			out << "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>" << std::endl
			<< "<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='" << Utils::get_lang() << "'"
			<< " lang='" << Utils::get_lang() << "'>" << std::endl
			<< "<head>" << std::endl
			<< "<meta http-equiv='Content-Type' content='text/html;charset=utf-8' />" << std::endl
			<< "<title>" << _("GParted Details") << "</title>" << std::endl
			<< "<style type='text/css'>" << std::endl
			<< "table {border:0}" << std::endl  // Disable borders for all tables
			<< "th {text-align:left}" << std::endl  // Left align all table headers
			<< ".number_col {text-align:right}" << std::endl  // Class for right alignment
			<< "</style>" << std::endl
			<< "</head>" << std::endl
			<< "<body>" << std::endl;

			std::vector<Glib::ustring> lines;
			Utils::split(GParted_Core::get_version_and_config_string(), lines, "\n");
			for (unsigned int i = 0; i < lines.size(); i++)
				out << "<p>" << lines[i] << "</p>" << std::endl;

			// Write out starting device layout
			for (unsigned int i = 0; i < m_devices.size(); i++)
			{
				out << "<p>========================================</p>" << std::endl;
				write_device_details(m_devices[i], out);
			}

			//Write out each operation
			for (unsigned int i = 0; i < operations.size(); i++)
			{
				out << "<p>========================================</p>" << std::endl;
				write_operation_details(operations[i]->operation_detail, out);
			}

			//Write out proper HTML finish
			out << "</body>" << std::endl << "</html>" ;
			out .close() ;
		}
	}
}


void Dialog_Progress::write_device_details(const Device& device, std::ofstream& out)
{
	out << "<table>" << std::endl;

	// Device overview information
	out << "<tr><th>" << _("Device:") << "</th><td>" << device.get_path() << "</td></tr>" << std::endl;
	out << "<tr><th>" << _("Model:") << "</th><td>" << device.model << "</td></tr>" << std::endl;
	out << "<tr><th>" << _("Serial:") << "</th><td>" << device.serial_number << "</td></tr>" << std::endl;
	out << "<tr><th>" << _("Sector size:") << "</th><td>" << device.sector_size << "</td></tr>" << std::endl;
	out << "<tr><th>" << _("Total sectors:") << "</th><td>" << device.length << "</td></tr>" << std::endl;

	out << "<tr><td colspan='2'>&nbsp;</td></tr>" << std::endl;

	out << "<tr><th>" << _("Heads:") << "</th><td>" << device.heads << "</td></tr>" << std::endl;
	out << "<tr><th>" << _("Sectors/track:") << "</th><td>" << device.sectors << "</td></tr>" << std::endl;
	out << "<tr><th>" << _("Cylinders:") << "</th><td>" << device.cylinders << "</td></tr>" << std::endl;

	out << "<tr><td colspan='2'>&nbsp;</td></tr>" << std::endl;

	// Partition table type
	out << "<tr><th>" << _("Partition table:") << "</th><td>" << device.disktype << "</td></tr>" << std::endl;

	out << "<tr><td colspan='2'>&nbsp;</td></tr>" << std::endl;
	out << "</table>" << std::endl;

	out << "<table>" << std::endl;

	// Partition headings
	out << "<tr>"
	    << "<th>" << _("Partition") << "</th>"
	    << "<th>" << _("Type") << "</th>"
	    << "<th class='number_col'>" << _("Start") << "</th>"
	    << "<th class='number_col'>" << _("End") << "</th>"
	    << "<th>" << _("Flags") << "</th>"
	    << "<th>" << _("Partition Name") << "</th>"
	    << "<th>" << _("File System") << "</th>"
	    << "<th>" << _("Label") << "</th>"
	    << "<th>" << _("Mount Point") << "</th>"
	    << "</tr>" << std::endl;

	// Partition details
	for (unsigned int i = 0; i < device.partitions.size(); i++)
	{
		if (device.partitions[i].type != TYPE_UNALLOCATED)
			write_partition_details(device.partitions[i], out);

		if (device.partitions[i].type == TYPE_EXTENDED)
		{
			for (unsigned int j = 0; j < device.partitions[i].logicals.size(); j++)
			{
				if (device.partitions[i].logicals[j].type != TYPE_UNALLOCATED)
					write_partition_details(device.partitions[i].logicals[j], out);
			}
		}
	}

	out << "</table>" << std::endl;
}


void Dialog_Progress::write_partition_details(const Partition& partition, std::ofstream& out)
{
	out << "<tr>"
	    << "<td>" << (partition.inside_extended ? "&nbsp;&nbsp;&nbsp; " : "") << partition.get_path() << "</td>"
	    << "<td>" << Partition::get_partition_type_string(partition.type) << "</td>"
	    << "<td class='number_col'>" << partition.sector_start << "</td>"
	    << "<td class='number_col'>" << partition.sector_end << "</td>"
	    << "<td>" << Glib::build_path(", ", partition.flags) << "</td>"
	    << "<td>" << partition.name << "</td>"
	    << "<td>" << Utils::get_filesystem_string(partition.fstype) << "</td>"
	    << "<td>" << partition.get_filesystem_label() << "</td>"
	    << "<td>" << Glib::build_path(", ", partition.get_mountpoints()) << "</td>"
	    << "</tr>" << std::endl;
}


void Dialog_Progress::write_operation_details(const OperationDetail& operationdetail, std::ofstream& out)
{
	// Replace '\n' with '<br />'
	Glib::ustring temp = operationdetail .get_description() ;
	for ( unsigned int index = temp .find( "\n" ) ; index < temp .length() ; index = temp .find( "\n" ) )
		temp .replace( index, 1, "<br />" ) ;
	
	//and export everything to some kind of html...
	out << "<table>" << std::endl
	<< "<tr>" << std::endl
	<< "<td colspan='2'>" << std::endl
	<< temp ;
	if ( ! operationdetail .get_elapsed_time() .empty() )
		out << "&nbsp;&nbsp;" << operationdetail .get_elapsed_time() ;
	
	//show status...
	if ( operationdetail .get_status() != STATUS_NONE )
	{
		out << "&nbsp;&nbsp;&nbsp;&nbsp;" ;
		switch ( operationdetail .get_status() )
		{
			case STATUS_EXECUTE:
				out << "( " <<
						/* TO TRANSLATORS:  EXECUTING
						 * means that the status for this operation is
						 * executing or currently in progress.
						 */
						_("EXECUTING") << " )" ;
				break ;
			case STATUS_SUCCESS:
				out << "( " <<
						/* TO" TRANSLATORS:  SUCCESS
						 * means that the status for this operation is
						 * completed successfully.
						 */
						_("SUCCESS") << " )" ;
				break ;
			case STATUS_ERROR:
				out << "( "
						/* TO TRANSLATORS:  ERROR
						 * means that the status for this operation is
						 * completed with errors.
						 */
						<< _("ERROR") << " )" ;
				break ;
			case STATUS_INFO:
				out << "( " <<
						/* TO TRANSLATORS:  INFO
						 * means that the status for this operation is
						 * for your information , or messages from the
						 * libparted library.
						 */
						_("INFO") << " )" ;
				break ;
			case STATUS_WARNING:
				out << "( " <<
						/* TO TRANSLATORS:  WARNING
						 * means that the status for this operation is
						 * completed with warnings.  Either the operation
						 * is not supported on the file system in the
						 * partition, or the operation failed but it does
						 * not matter that it failed.
						 */
						_("WARNING") << " )" ;
				break ;

			default:
				break ;
		}
	}
	
	out << std::endl << "</td>" << std::endl << "</tr>" << std::endl ;
	
	if ( operationdetail .get_childs(). size() )
	{
		out << "<tr>" << std::endl
		<< "<td>&nbsp;&nbsp;&nbsp;&nbsp;</td>" << std::endl
		<< "<td>" << std::endl ;

		for (unsigned int i = 0; i < operationdetail.get_childs().size(); i++)
			write_operation_details(*(operationdetail.get_childs()[i]), out);

		out << "</td>" << std::endl << "</tr>" << std::endl ;
	}
	out << "</table>" << std::endl ;
	
}

void Dialog_Progress::on_response( int response_id ) 
{
	switch ( response_id )
	{
		case Gtk::RESPONSE_OK:
			on_save() ;
			break ;
		case Gtk::RESPONSE_CANCEL:
			on_cancel() ;
			break ;
	}
}

bool Dialog_Progress::on_delete_event( GdkEventAny * event ) 
{
	//it seems this get only called at runtime
	on_cancel() ;
	return true ;
}


Dialog_Progress::~Dialog_Progress()
{
	delete cancelbutton;
}


}//GParted
