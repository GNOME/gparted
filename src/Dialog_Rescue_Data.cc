/* Copyright (C) 2010 Joan Lledó
 * Copyright (C) 2011 Curtis Gedak
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

#include "Utils.h"
#include "Dialog_Rescue_Data.h"
#include "Partition.h"

#include <gtkmm/messagedialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/checkbutton.h>
#include <sstream>
#include <cerrno>

namespace GParted
{

#define tmp_prefix P_tmpdir "/gparted-roview-XXXXXX"

//The constructor creates a empty dialog
Dialog_Rescue_Data::Dialog_Rescue_Data()
{
	this ->set_title( _("Search disk for file systems") );

	this ->add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE );
}

//getters
bool Dialog_Rescue_Data::found_partitions()
{
	return this->partitions.size() > 0;
}

// Draws the dialog
void Dialog_Rescue_Data::draw_dialog()
{
	Glib::ustring *message;

	/*TO TRANSLATORS: looks like    File systems found on /dev/sdb */
	this ->set_title( String::ucompose( _("File systems found on %1"), this->device_path ) );

	message=new Glib::ustring("<big><b>");
	if(!this->inconsistencies)
	{
		message->append(_("Data found"));
	}
	else
	{
		message->append(_("Data found with inconsistencies"));
		
		Glib::ustring msg_label=_("WARNING!: The file systems marked with (!) are inconsistent.");
		msg_label.append("\n");
		msg_label.append(_("You might encounter errors trying to view these file systems."));
		
		Gtk::Label *inconsis_label=manage(Utils::mk_label(msg_label));
		Gdk::Color c( "red" );
		inconsis_label->modify_fg(Gtk::STATE_NORMAL, c );
		this->get_vbox()->pack_end(*inconsis_label, Gtk::PACK_SHRINK, 5);
	}
	message->append("</b></big>");

	this->get_vbox()->set_spacing(5);

	Gtk::Label *msgLbl=Utils::mk_label(*message);
	this->get_vbox()->pack_start(*msgLbl, Gtk::PACK_SHRINK);

	this->create_list_of_fs();

	Glib::ustring info_txt=_("The 'View' buttons create read-only views of each file system.");
	info_txt+="\n";
	info_txt+=_("All mounted views will be unmounted when you close this dialog.");

	Gtk::HBox *infoBox=manage(new Gtk::HBox());
	Gtk::Image *infoImg=manage(new Gtk::Image( Gtk::Stock::DIALOG_INFO, Gtk::ICON_SIZE_DIALOG));
	Gtk::Label *infoLabel= manage(new Gtk::Label (info_txt));

	infoBox->pack_start(*infoImg, Gtk::PACK_SHRINK, 5);
	infoBox->pack_start(*infoLabel, Gtk::PACK_SHRINK);

	this->get_vbox()->pack_start(*infoBox, Gtk::PACK_SHRINK);

	this->show_all_children();

	delete message;
}

/*
 * Create the list of the filesystems found */
void Dialog_Rescue_Data::create_list_of_fs()
{
	Gtk::VBox *vb=manage(new Gtk::VBox());
	vb->set_border_width(5);
	vb->set_spacing(5);
	this->frm=Gtk::manage(new Gtk::Frame(_("File systems")));
	this->frm->add(*vb);

	for(unsigned int i=0;i<this->partitions.size();i++)
	{
		if(this->partitions[i].filesystem==GParted::FS_UNALLOCATED
			|| this->partitions[i].filesystem==GParted::FS_UNKNOWN
			|| this->partitions[i].filesystem==GParted::FS_UNFORMATTED
			|| this->partitions[i].filesystem==GParted::FS_EXTENDED
			|| this->partitions[i].type==GParted::TYPE_EXTENDED
			|| this->partitions[i].type==GParted::TYPE_UNALLOCATED)
		{
			continue;
		}

		std::string fs_name=Utils::get_filesystem_string( this->partitions[i].filesystem );
		if(this->partitions[i].filesystem==FS_EXT2)
		{
			fs_name+="/3/4, ReiserFs or XFS";
		}

		/*TO TRANSLATORS: looks like    1: ntfs (10240 MiB)*/
		Gtk::Label *fsLbl= manage(new Gtk::Label( String::ucompose(_("#%1: %2 (%3 MiB)"), i+1, fs_name, (this->partitions[i].get_byte_length()/1024/1024))));
		if(this->is_inconsistent(this->partitions[i]))
		{
			fsLbl->set_label(fsLbl->get_label().append(" (!)"));
		}

		Gtk::HBox *hb=manage(new Gtk::HBox());

		Gtk::Button *btn=manage(new Gtk::Button(_("View")));

		btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &Dialog_Rescue_Data::on_view_clicked), i));

        hb->pack_start(*fsLbl, Gtk::PACK_SHRINK);
        hb->pack_end(*btn, Gtk::PACK_SHRINK);

        vb->pack_start(*hb, Gtk::PACK_SHRINK);
	}

	this->get_vbox()->pack_start(*this->frm, Gtk::PACK_SHRINK);
}

/*
 * Callback function for "View" button */
void Dialog_Rescue_Data::on_view_clicked(int nPart)
{
	const Partition & part = this->partitions[nPart];

	Byte_Value initOffset=this->sector_size*part.sector_start;
	Sector numSectors=part.sector_end-part.sector_start+1;
	Byte_Value totalSize=this->sector_size*numSectors;

	this->check_overlaps(nPart);

	char tmpDir[32]=tmp_prefix;

	char * tmpDirResult = mkdtemp(tmpDir);
	if ( tmpDirResult == NULL )
	{
		Glib::ustring error_txt = _("An error occurred while creating a temporary directory for use as a mount point.");
		error_txt += "\n";
		error_txt += _("Error");
		error_txt += ":\n";
		error_txt += Glib::strerror( errno );

		//Dialog information
		Gtk::MessageDialog errorDialog(*this, "", true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		errorDialog.set_message(_("Failed creating temporary directory"));
		errorDialog.set_secondary_text(error_txt);

		errorDialog.run();

		return;
	}

	Glib::ustring mountPoint=tmpDir;

	Glib::ustring commandLine = "mount -o ro,loop,offset=" + Utils::num_to_str(initOffset) +
	                            ",sizelimit=" + Utils::num_to_str(totalSize) +
	                            " " + Glib::shell_quote(this->device_path) +
	                            " " + Glib::shell_quote(mountPoint);
	int mountResult=Utils::execute_command(commandLine);

	if(mountResult!=0)
	{
		Glib::ustring error_txt=_("An error occurred while creating the read-only view.");
		error_txt+="\n";
		error_txt+=_("Either the file system can not be mounted (like swap), or there are inconsistencies or errors in the file system.");

		//Dialog information
		Gtk::MessageDialog errorDialog(*this, "", true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		errorDialog.set_message(_("Failed creating read-only view"));
		errorDialog.set_secondary_text(error_txt);

		errorDialog.run();

		return;
	}

	this->open_ro_view(mountPoint);
}

/* Opens the default browser in a directory */
void Dialog_Rescue_Data::open_ro_view(Glib::ustring mountPoint)
{
	GError *error = NULL ;
	GdkScreen *gscreen = NULL ;

	Glib::ustring uri = "file:" + mountPoint ;

	gscreen = gdk_screen_get_default() ;

#ifdef HAVE_GTK_SHOW_URI
	gtk_show_uri( gscreen, uri .c_str(), gtk_get_current_event_time(), &error ) ;
#else
	Glib::ustring command = "gnome-open " + uri ;
	gdk_spawn_command_line_on_screen( gscreen, command .c_str(), &error ) ;
#endif

	if ( error != NULL )
	{
		Glib::ustring sec_text(_("Error:"));
		sec_text.append("\n");
		sec_text.append(error ->message);
		sec_text.append("\n");
		sec_text.append("\n");
		/*TO TRANSLATORS: looks like
		 * The file system is mounted on:
		 * /tmp/gparted-roview-Nlhb3R. */
		sec_text.append(_("The file system is mounted on:"));
		sec_text.append("\n");
		sec_text.append("<b>"+mountPoint+"</b>");

		Gtk::MessageDialog dialog( *this
		                         , _( "Unable to open the default file manager" )
		                         , false
		                         , Gtk::MESSAGE_ERROR
		                         , Gtk::BUTTONS_OK
		                         , true
		                         ) ;
		dialog .set_secondary_text( sec_text, true ) ;
		dialog .run() ;
	}
}

/*
 * Checks if a guessed filesystem is overlapping other one and
 * shows the dialog to umount it */
void Dialog_Rescue_Data::check_overlaps(int nPart)
{
	if(is_overlaping(nPart))
	{
		Gtk::MessageDialog overlapDialog(*this, "", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		overlapDialog.set_message(_("Warning: The detected file system area overlaps with at least one existing partition"));

		Glib::ustring sec_text=_("It is recommended that you do not use any overlapping file systems to avoid disturbing existing data.");
		sec_text+="\n";
		sec_text+=_("Do you want to try to deactivate the following mount points?");

		for(unsigned int i=0;i<this->overlappedPartitions.size(); i++)
		{
			Glib::ustring ovrDevPath=this->device->partitions[i].get_path();
			Glib::ustring ovrDevMountPoint=this->device->partitions[i].get_mountpoint();

			sec_text+="\n"+ovrDevPath+" mounted on "+ovrDevMountPoint;
		}

		overlapDialog.set_secondary_text(sec_text);

		if(overlapDialog.run()==Gtk::RESPONSE_YES)
		{
			for(unsigned int i=0;i<this->overlappedPartitions.size(); i++)
			{
				Glib::ustring mountP=this->device->partitions[i].get_mountpoint();

				Glib::ustring commandUmount = "umount " + Glib::shell_quote(mountP);
				Utils::execute_command(commandUmount);
			}
		}
	}
}

/*
 * Reads the output of gpart and sets some private variables */
void Dialog_Rescue_Data::init_partitions(Device *parentDevice, const Glib::ustring & buff)
{
	this->device=parentDevice;
	this->device_path=parentDevice->get_path();
	this->device_length=parentDevice->length;
	this->sector_size=parentDevice->sector_size;
	this->buffer=new std::istringstream(buff);
	this->inconsistencies=false;

	std::string line;

	while(getline(*this->buffer, line))
	{
		//If gpart finds inconsistencies, might not be able to mount some partitions
		if(line.find("Number of inconsistencies found")!=Glib::ustring::npos)
		{
			this->inconsistencies=true;
		}

		//Read the primary partition table
		// (gpart only finds primary partitions)
		if(line.find("Guessed primary partition table:")!=Glib::ustring::npos)
		{
			this->read_partitions_from_buffer();
		}
	}

	this->draw_dialog();
}

/* Reads the output of gpart and builds the vector of guessed partitions */
void Dialog_Rescue_Data::read_partitions_from_buffer()
{
	this->partitions.clear();

	std::string line;

	while(getline(*this->buffer, line))
	{
		if(line.find("Primary partition")!=line.npos)
		{
			// Parameters of Partition::Set
			Glib::ustring dev_path=this->device_path;
			Glib::ustring part_path;
			int part_num;
			PartitionType type=GParted::TYPE_PRIMARY;
			FSType fs = FS_UNALLOCATED;
			Sector sec_start=0;
			Sector sec_end=0;
			Byte_Value sec_size=this->sector_size;

			// Get the part_num
			Glib::ustring num=Utils::regexp_label(line, "^Primary partition\\(+([0-9])+\\)$");
			part_num=Utils::convert_to_int(num);

			//Get the part_path
			part_path=String::ucompose ( "%1%2", this->device_path, num );

			while(getline(*this->buffer, line))
			{
				if(line=="")
				{
					break;
				}

				if(line.find("type:")!=Glib::ustring::npos)
				{
					//Get the filesystem (needed for set the color of the visual partition)
					int code=Utils::convert_to_int(Utils::regexp_label(line, "^[\t ]+type: +([0-9]+)+\\(+[a-zA-Z0-9]+\\)\\(+.+\\)$"));

					switch (code)
					{
						case 0x83: //FS code for ext2, reiserfs and xfs
						{
							fs=GParted::FS_EXT2;
							break;
						}
						case 0x18:
						case 0x82:
						case 0xB8: //SWAP partition
						{
							fs=GParted::FS_LINUX_SWAP;
							break;
						}
						case 0x04:
						case 0x14:
						case 0x86:
						case 0xE4: //FAT16
						{
							fs=GParted::FS_FAT16;
							break;
						}
						case 0x0B:
						case 0x0C: //FAT32
						{
							fs=GParted::FS_FAT32;
							break;
						}
						case 0x07: //NTFS, HPFS, exFAT and UDF
						{
							fs=GParted::FS_NTFS;
							break;
						}
						case 0x05:
						case 0x0F:
						case 0x85: //Extended
						{
							fs=GParted::FS_EXTENDED;
							type=GParted::TYPE_EXTENDED;
							break;
						}
						default:
						{
							fs=GParted::FS_UNKNOWN;
						}
					}
				}

				if(line.find("size:")!=Glib::ustring::npos)
				{
					// Get the start sector
					sec_start=Utils::convert_to_int(Utils::regexp_label(line, "^[\t ]+size: [0-9]+mb #s\\(+[0-9]+\\) s\\(+([0-9]+)+\\-+[0-9]+\\)$"));

					// Get the end sector
					sec_end=Utils::convert_to_int(Utils::regexp_label(line, "^[\t ]+size: [0-9]+mb #s\\(+[0-9]+\\) s\\(+[0-9]+\\-+([0-9]+)+\\)$"));
				}
			}

			//No part found
			if(sec_start==0 && sec_end==0)
			{
				continue;
			}

			//Swap partitions don't contain data
			if(fs==GParted::FS_LINUX_SWAP)
			{
				continue;
			}

			Partition * part = new Partition();
			part->Set( dev_path, part_path, part_num,
			           type, fs, sec_start, sec_end, sec_size, false, false );
			this->partitions.push_back_adopt( part );
		}
	}
}


/*
 * Checks if the guessed partition is overlaping some active partition
 */
bool Dialog_Rescue_Data::is_overlaping(int nPart)
{
	bool result=false;

	Sector start_sector=this->partitions[nPart].sector_start;
	Sector end_sector=this->partitions[nPart].sector_end;

	for(unsigned int j=0;j<this->device->partitions.size();j++)
	{
		//only check if the partition if mounted
		if(this->device->partitions[j].get_mountpoint()=="")
		{
			continue;
		}

		//If the start sector is inside other partition
		if(start_sector>this->device->partitions[j].sector_start &&
			start_sector<this->device->partitions[j].sector_end)
		{
			this->overlappedPartitions.push_back(j);
			result=true;
			continue;
		}

		//If the end sector is inside other partition
		if(end_sector>this->device->partitions[j].sector_start &&
			end_sector<this->device->partitions[j].sector_end)
		{
			this->overlappedPartitions.push_back(j);
			result=true;
			continue;
		}

		//There is a partition between the sectors start and end
		//If the end sector is inside other partition
		if(this->device->partitions[j].sector_start>start_sector &&
			this->device->partitions[j].sector_end<end_sector)
		{
			this->overlappedPartitions.push_back(j);
			result=true;
			continue;
		}
	}

	return result;
}

/* Check if a partition is inconsistent */
bool Dialog_Rescue_Data::is_inconsistent(const Partition &part)
{
	for(unsigned int i=0;i<this->inconsistencies_list.size();i++)
	{
		if (part.partition_number==this->inconsistencies_list[i])
		{
			return true;
		}
	}

	return false;
}

}//GParted
