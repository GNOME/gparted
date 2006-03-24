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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "../include/Dialog_Progress.h"

#include <gtkmm/stock.h>
#include <gtkmm/main.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/filechooserdialog.h>

namespace GParted
{

Dialog_Progress::Dialog_Progress( const std::vector<Operation *> & operations )
{
	this ->set_resizable( false ) ;
	this ->set_has_separator( false ) ;
	this ->set_title( _("Applying pending operations") ) ;
	this ->operations = operations ;
	succes = true ;
	cancel = false ;

	fraction = 1.00 / operations .size() ;
			
	Glib::ustring str_temp = "<span weight=\"bold\" size=\"larger\">" ;
	str_temp += _("Applying pending operations") ;
	str_temp += "</span>\n\n" ;
	str_temp += _("Applying all listed operations.") ;
	str_temp += "\n";
	str_temp += _("Depending on the amount and type of operations this might take a long time.") ;
	str_temp += "\n";
	this ->get_vbox() ->pack_start( * Utils::mk_label( str_temp ), Gtk::PACK_SHRINK );
	
	this ->get_vbox() ->pack_start( * Utils::mk_label( "<b>" + static_cast<Glib::ustring>( _("Current Operation:") ) + "</b>" ), Gtk::PACK_SHRINK );
	
	progressbar_current .set_pulse_step( 0.01 ) ;
	this->get_vbox() ->pack_start( progressbar_current, Gtk::PACK_SHRINK );
	
	label_current .set_alignment( Gtk::ALIGN_LEFT );
	this ->get_vbox() ->pack_start( label_current, Gtk::PACK_SHRINK );
	
	this ->get_vbox() ->pack_start( * Utils::mk_label( "<b>" + static_cast<Glib::ustring>( _("Completed Operations:") ) + "</b>" ), Gtk::PACK_SHRINK );
	this ->get_vbox() ->pack_start( progressbar_all, Gtk::PACK_SHRINK );
	
	//create some icons here, instead of recreating them every time
	icon_execute = render_icon( Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_LARGE_TOOLBAR ) ;
	icon_succes = render_icon( Gtk::Stock::APPLY, Gtk::ICON_SIZE_LARGE_TOOLBAR ) ;
	icon_error = render_icon( Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_LARGE_TOOLBAR ) ;
	
	treestore_operations = Gtk::TreeStore::create( treeview_operations_columns );
	treeview_operations .set_model( treestore_operations );
	treeview_operations .set_headers_visible( false );
	treeview_operations .append_column( "", treeview_operations_columns .operation_icon );
	treeview_operations .append_column( "", treeview_operations_columns .operation_description );
	treeview_operations .append_column( "", treeview_operations_columns .status_icon );
	treeview_operations .set_size_request( 500, 250 ) ;
	treeview_operations .set_rules_hint( true ) ;

	//fill 'er up
	for ( unsigned int t = 0 ; t < operations .size() ; t++ )
	{
		this ->operations[ t ] ->operation_details .description = "<b>" + operations[ t ] ->description + "</b>" ;
		
		treerow = *( treestore_operations ->append() );
		treerow[ treeview_operations_columns .operation_icon ] = operations[ t ] ->icon ;
		treerow[ treeview_operations_columns .operation_description ] =
			this ->operations[ t ] ->operation_details .description ; 
		treerow[ treeview_operations_columns .hidden_status ] = OperationDetails::NONE ; 
	}
	
	treeview_operations .get_column( 1 ) ->set_expand( true ) ;
	treeview_operations .get_column( 1 ) ->set_cell_data_func( 
		* ( treeview_operations .get_column( 1 ) ->get_first_cell_renderer() ),
		sigc::mem_fun(*this, &Dialog_Progress::on_cell_data_description) ) ;
	
	scrolledwindow .set_shadow_type( Gtk::SHADOW_ETCHED_IN ) ;
	scrolledwindow .set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC ) ;
	scrolledwindow .add( treeview_operations ) ;

	expander_details .set_label( "<b>" + static_cast<Glib::ustring>( _("Details") ) + "</b>" ) ;
	expander_details .set_use_markup( true ) ;
	expander_details .property_expanded() .signal_changed() .connect(
   		sigc::mem_fun(*this, &Dialog_Progress::on_expander_changed) );
	expander_details .add( scrolledwindow ) ;
	
	this ->get_vbox() ->pack_start( expander_details, Gtk::PACK_EXPAND_WIDGET ) ; 
	this ->get_vbox() ->set_spacing( 5 ) ;
	
	this ->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL ) ;
	
	this ->signal_show() .connect( sigc::mem_fun(*this, &Dialog_Progress::on_signal_show) );
	this ->show_all_children() ;
}

void Dialog_Progress::update_operation_details( const Gtk::TreeRow & treerow, 
					        const OperationDetails & operation_details ) 
{
	//append new rows (if any)
	for ( unsigned int t = treerow .children() .size() ; t < operation_details .sub_details .size() ; t++ )
	{
		Gtk::TreeRow treerow_child = *( treestore_operations ->append( treerow .children() ) ) ;
		
		treerow_child[ treeview_operations_columns .operation_description ] =
			operation_details .sub_details[ t ] .description ;
		
		treerow_child[ treeview_operations_columns .hidden_status ] = OperationDetails::NONE ;
	}
	
	//check status and update if necessary
	if ( operation_details .status != treerow[ treeview_operations_columns .hidden_status ] )
	{
		treerow[ treeview_operations_columns .hidden_status ] =	operation_details .status ;

		switch ( operation_details .status )
		{
			case OperationDetails::EXECUTE:
				treerow[ treeview_operations_columns .status_icon ] = icon_execute ;
					
				break ;
			case OperationDetails::SUCCES:
				treerow[ treeview_operations_columns .status_icon ] = icon_succes ;
				
				break ;
			case OperationDetails::ERROR:
				treerow[ treeview_operations_columns .status_icon ] = icon_error ;
				
				break ;
					
			default : 
				treerow[ treeview_operations_columns .hidden_status ] = OperationDetails::NONE ;
				break ;
		}
	}

	//and update the children..
	for ( unsigned int t = 0 ; t < operation_details .sub_details .size() ; t++ )
		update_operation_details( treerow .children()[ t ], operation_details .sub_details[ t ] ) ;
}

void Dialog_Progress::on_signal_show()
{
	for ( t = 0 ; t < operations .size() && succes && ! cancel ; t++ )
	{
		label_current .set_markup( "<i>" + operations[ t ] ->description + "</i>\n" ) ;
		
		progressbar_all .set_text( String::ucompose( _("%1 of %2 operations completed"), t, operations .size() ) ) ;
		progressbar_all .set_fraction( fraction * t ) ;
		
		treerow = treestore_operations ->children()[ t ] ;

		//set status to 'execute'
		operations[ t ] ->operation_details .status = OperationDetails::EXECUTE ;
		update_operation_details( treerow, operations[ t ] ->operation_details ) ;

		//set focus...
		treeview_operations .set_cursor( static_cast<Gtk::TreePath>( treerow ) ) ;
		
		//and start..
		pulse = true ;
		pthread_create( & pthread, NULL, Dialog_Progress::static_pthread_apply_operation, this );
	
		while ( pulse )
		{
			update_operation_details( treerow, operations[ t ] ->operation_details ) ;

			progressbar_current .pulse() ;
			
			while ( Gtk::Main::events_pending() )
				Gtk::Main::iteration();

			usleep( 10000 ) ;
		}

		//set status (succes/error) for this operation
		operations[ t ] ->operation_details .status =
			succes ? OperationDetails::SUCCES : OperationDetails::ERROR ;
		update_operation_details( treerow, operations[ t ] ->operation_details ) ;
	}
	
	//add save button
	this ->add_button( _("_Save Details"), Gtk::RESPONSE_OK ) ; //there's no enum for SAVE
	
	//replace 'cancel' with 'close'
	std::vector<Gtk::Widget *> children = this ->get_action_area() ->get_children() ;
	this ->get_action_area() ->remove( * children .back() ) ;
	this ->add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE );

	if ( cancel )
	{
		progressbar_current .set_text( _("Operation canceled") ) ;
		progressbar_current .set_fraction( 0.0 ) ;
	}
	else
	{
		//hide 'current operation' stuff
		children = this ->get_vbox() ->get_children() ;
		children[ 1 ] ->hide() ;
		progressbar_current .hide() ;
		label_current .hide() ;
	}

	//deal with succes/error...
	if ( succes )
	{
		progressbar_all .set_text( _("All operations succesfully completed") ) ;
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
			str_temp = _("The following operation could not be applied to disk:") ;
			str_temp += "\n\n<i>" ;
			str_temp += label_current .get_text() ;
			str_temp += "</i>\n" ;
			str_temp += _("See the details for more information") ;
		
			dialog .set_secondary_text( str_temp, true ) ;
			dialog .run() ;
		}
	} 
}

void Dialog_Progress::on_expander_changed() 
{
	this ->set_resizable( expander_details .get_expanded() ) ;
}

void Dialog_Progress::on_cell_data_description( Gtk::CellRenderer * renderer, const Gtk::TreeModel::iterator & iter )
{
	dynamic_cast<Gtk::CellRendererText *>( renderer ) ->property_markup() = 
		static_cast<Gtk::TreeRow>( * iter )[ treeview_operations_columns .operation_description ] ;
}

void * Dialog_Progress::static_pthread_apply_operation( void * p_dialog_progress ) 
{
	Dialog_Progress *dp = static_cast<Dialog_Progress *>( p_dialog_progress ) ;
	
	dp ->succes = dp ->signal_apply_operation .emit( dp ->operations[ dp ->t ] ) ;
	
	dp ->pulse = false ;

	return NULL ;
}

void Dialog_Progress::on_cancel()
{
	Gtk::MessageDialog dialog( *this,
				   _("Are you sure you want to cancel the current operation?"),
				   false,
				   Gtk::MESSAGE_QUESTION,
				   Gtk::BUTTONS_NONE,
				   true ) ;
		
	dialog .set_secondary_text( _("Canceling an operation may cause SEVERE filesystem damage.") ) ;

	dialog .add_button( _("Continue Operation"), Gtk::RESPONSE_NONE ) ;
	dialog .add_button( _("Cancel Operation"), Gtk::RESPONSE_CANCEL ) ;
	
	if ( dialog .run() == Gtk::RESPONSE_CANCEL )
	{
		pthread_cancel( pthread ) ;
		cancel = true ;
		pulse = false ;
		succes = false ;
	}
}

void Dialog_Progress::on_save()
{
	Gtk::FileChooserDialog dialog( _("Save Details"), Gtk::FILE_CHOOSER_ACTION_SAVE ) ;
	dialog .set_transient_for( *this ) ;
	dialog .set_current_folder( Glib::get_home_dir() ) ;
	dialog .set_current_name( "gparted_details.htm" ) ;
//	dialog .set_do_overwrite_confirmation( true ) ; FIXME: since gtkmm-2.8.. 
	dialog .add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL ) ; 
	dialog .add_button( Gtk::Stock::SAVE, Gtk::RESPONSE_OK ) ; //there's no enum for SAVE

	if ( dialog .run() == Gtk::RESPONSE_OK )
	{
		std::ofstream out( dialog .get_filename() .c_str() ) ;
		if ( out )
		{
			out << "GParted " << VERSION << "<BR><BR>" << std::endl ;
			for ( unsigned int t = 0 ; t < operations .size() ; t++ )
			{
				echo_operation_details( operations[ t ] ->operation_details, out ) ;
				out << "<BR>========================================<BR><BR>" << std::endl ;
			}
				
			out .close() ;
		}
	}
}

void Dialog_Progress::echo_operation_details( const OperationDetails & operation_details, std::ofstream & out ) 
{
	//replace '\n' with '<br>'
	Glib::ustring temp = operation_details .description ;
	for ( unsigned int index = temp .find( "\n" ) ; index < temp .length() ; index = temp .find( "\n" ) )
		temp .replace( index, 1, "<BR>" ) ;
	
	//and export everything to some kind of html...
	out << "<TABLE border=0>" << std::endl ;
	out << "<TR>" << std::endl ;
	out << "<TD colspan=2>" << std::endl ;
	out << temp ;
	
	//show status...
	if ( operation_details .status != OperationDetails::NONE )
	{
		out << "&nbsp;&nbsp;&nbsp;&nbsp;" ;
		switch ( operation_details .status )
		{
			case OperationDetails::EXECUTE:
				out << "( EXECUTING )" ;
				break ;
			case OperationDetails::SUCCES:
				out << "( SUCCES )" ;
				break ;
			case OperationDetails::ERROR:
				out << "( ERROR )" ;
				break ;

			default:
				break ;
		}
	}
	
	out << std::endl ;
	out << "</TD>" << std::endl ;
	out << "</TR>" << std::endl ;
	
	if ( operation_details .sub_details. size() )
	{
		out << "<TR>" << std::endl ;
		out << "<TD>&nbsp;&nbsp;&nbsp;&nbsp;</TD>" << std::endl ;
		out << "<TD>" << std::endl ;

		for ( unsigned int t = 0 ; t < operation_details .sub_details .size() ; t++ )
			echo_operation_details( operation_details .sub_details[ t ], out ) ;

		out << "</TD>" << std::endl ;
		out << "</TR>" << std::endl ;
	}
	
	out << "</TABLE>" << std::endl ;
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
}


}//GParted
