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

namespace GParted
{

Dialog_Progress::Dialog_Progress( const std::vector<Operation> & operations )
{
	this ->set_has_separator( false ) ;
	this ->set_title( _("Applying pending operations") ) ;
	this ->operations = operations ;
	succes = true ;

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
	progressbar_all .set_size_request( 500, -1 ) ;
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
	treeview_operations .set_size_request( -1, 250 ) ;
	treeview_operations .set_rules_hint( true ) ;

	//fill 'er up
	for ( unsigned int t = 0 ; t < operations .size() ; t++ )
	{
		treerow = *( treestore_operations ->append() );
		treerow[ treeview_operations_columns .operation_icon ] = operations[ t ] .operation_icon ;
		treerow[ treeview_operations_columns .operation_description ] = operations[ t ] .str_operation ; 

		this ->operations[ t ] .operation_details .description = operations[ t ] .str_operation ;
	}
	
	treeview_operations .get_column( 1 ) ->set_expand( true ) ;
	
	scrolledwindow .set_shadow_type( Gtk::SHADOW_ETCHED_IN ) ;
	scrolledwindow .set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC ) ;
	scrolledwindow .add( treeview_operations ) ;

	expander_details .set_label( "<b>" + static_cast<Glib::ustring>( _("Details") ) + "</b>" ) ;
	expander_details .set_use_markup( true ) ;
	expander_details .add( scrolledwindow ) ;
	
	this ->get_vbox() ->pack_start( expander_details, Gtk::PACK_SHRINK ) ; 
	this ->get_vbox() ->set_spacing( 5 ) ;
	
	this ->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_NONE ) ;
	
	this ->signal_show() .connect( sigc::mem_fun(*this, &Dialog_Progress::on_signal_show) );
	this ->show_all_children() ;
}

void Dialog_Progress::update_operation_details( const Gtk::TreeRow & treerow, 
					        const OperationDetails & operation_details ) 
{
	Gtk::TreeRow treerow_child ;
	
	//append new rows (if any)
	for ( unsigned int t = treerow .children() .size() ; t < operation_details .sub_details .size() ; t++ )
	{
		treerow_child = *( treestore_operations ->append( treerow .children() ) ) ;
		
		treerow_child[ treeview_operations_columns .operation_description ] =
			operation_details .sub_details[ t ] .description ;
		
		treerow_child[ treeview_operations_columns .hidden_status ] = OperationDetails::NONE ;
	}

	for ( unsigned int t = 0 ; t < operation_details .sub_details .size() ; t++ )
	{
		treerow_child = treerow .children()[ t ] ;

		if ( operation_details .sub_details[ t ] .status != treerow_child[ treeview_operations_columns .hidden_status ] )
		{
			treerow_child[ treeview_operations_columns .hidden_status ] =
				operation_details .sub_details[ t ] .status ;

			switch ( operation_details .sub_details[ t ] .status )
			{
				case OperationDetails::EXECUTE:
					treerow_child[ treeview_operations_columns .status_icon ] = icon_execute ;
					
					break ;
				case OperationDetails::SUCCES:
					treerow_child[ treeview_operations_columns .status_icon ] = icon_succes ;
					
					break ;
				case OperationDetails::ERROR:
					treerow_child[ treeview_operations_columns .status_icon ] = icon_error ;
					
					break ;
					
				default : 
					//plain, old-fashioned paranoia ;)
					treerow_child[ treeview_operations_columns .hidden_status ] =
						OperationDetails::NONE ;
					break ;
			}
		}
		
		update_operation_details( treerow_child, operation_details .sub_details[ t ] ) ;
	}
}

void Dialog_Progress::on_signal_show()
{
	for ( t = 0 ; t < operations .size() && succes ; t++ )
	{
		label_current .set_markup( "<i>" + operations[ t ] .str_operation + "</i>" ) ;
		
		progressbar_all .set_text( String::ucompose( _("%1 of %2 operations completed"), t, operations .size() ) ) ;
		progressbar_all .set_fraction( fraction * t ) ;
		
		static_cast<Gtk::TreeRow>( treestore_operations ->children()[ t ] )
			[ treeview_operations_columns .status_icon ] = icon_execute ;
		treeview_operations .set_cursor( static_cast<Gtk::TreePath>( treestore_operations ->children()[ t ] ) ) ;
		
		pulse = true ;
		thread = Glib::Thread::create( sigc::bind<Operation *>( 
				sigc::mem_fun( *this, &Dialog_Progress::thread_apply_operation ), &( operations[ t ] ) ), true );
		
		treerow = treestore_operations ->children()[ t ] ;
		while ( pulse )
		{
			update_operation_details( treerow, operations[ t ] .operation_details ) ;

			progressbar_current .pulse() ;
			
			while ( Gtk::Main::events_pending() )
				Gtk::Main::iteration();

			usleep( 10000 ) ;
		}

		thread ->join() ;

		//final updates for this operation
		update_operation_details( treerow, operations[ t ] .operation_details ) ;
		static_cast<Gtk::TreeRow>( treestore_operations ->children()[ t ] )
			[ treeview_operations_columns .status_icon ] = succes ? icon_succes : icon_error ;
	}

	//replace 'cancel' with 'close'
	std::vector<Gtk::Widget *> children  = this ->get_action_area() ->get_children() ;
	this ->get_action_area() ->remove( * children .back() ) ;
	this ->add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_OK );

	//hide 'current operation' stuff
	children = this ->get_vbox() ->get_children() ;
	children[ 1 ] ->hide() ;
	progressbar_current .hide() ;
	label_current .hide() ;

	//deal with succes/error...
	if ( succes )
	{
		progressbar_all .set_text( _("All operations succesfully completed") ) ;
		progressbar_all .set_fraction( 1.0 ) ;
	}
	else 
	{
		progressbar_all .set_text( _("Not all operations were succesfully completed") ) ;
		expander_details .set_expanded( true ) ;

		Gtk::MessageDialog dialog( *this,
					   _("An error occurred while applying the operations"),
					   false,
					   Gtk::MESSAGE_ERROR,
					   Gtk::BUTTONS_OK,
					   true ) ;
		str_temp = _("The following operation could not be applied to disk:") ;
		str_temp += "\n<i>" ;
		str_temp += label_current .get_text() ;
		str_temp += "</i>\n\n" ;
		str_temp += _("See the details for more information") ;
		
		dialog .set_secondary_text( str_temp, true ) ;
		dialog .run() ;
	}
}

void Dialog_Progress::thread_apply_operation( Operation * operation ) 
{
	succes = signal_apply_operation .emit( *operation ) ;

	pulse = false ;
}

void Dialog_Progress::on_response( int response_id ) 
{
	if ( pulse && ( response_id == Gtk::RESPONSE_DELETE_EVENT || response_id == Gtk::RESPONSE_CANCEL ) )
	{
		//FIXME i guess this is the best place to implement the cancel. there are 2 ways to trigger this:
		//press the 'cancel' button
		//close the dialog by pressing the cross
		std::cout << "CANCEL!! yet to be implemented... ;)" << std::endl ;
	}

	Gtk::Dialog::on_response( response_id ) ;
}

Dialog_Progress::~Dialog_Progress()
{
}


}//GParted
