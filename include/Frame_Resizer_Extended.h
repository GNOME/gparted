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
 
 
#ifndef  FRAME_RESIZER_EXTENDED
#define FRAME_RESIZER_EXTENDED

#include "../include/Frame_Resizer_Base.h"
 
 class Frame_Resizer_Extended : public Frame_Resizer_Base
{
public:
	Frame_Resizer_Extended(  ) ;
	

	

	

private:
	int UNUSED_BEFORE ;
	//overridden signal handler
	virtual bool drawingarea_on_mouse_motion( GdkEventMotion* ) ;

	virtual void Draw_Partition() ;

		
	

	


};

#endif // FRAME_RESIZER_EXTENDED
