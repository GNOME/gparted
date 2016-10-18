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


#ifndef GPARTED_FRAME_RESIZER_EXTENDED_H
#define GPARTED_FRAME_RESIZER_EXTENDED_H

#include "Frame_Resizer_Base.h"

class Frame_Resizer_Extended : public Frame_Resizer_Base
{
public:
	Frame_Resizer_Extended() ;
	
	void set_used_start( int used_start ) ;

private:
	int USED_START ;

	//overridden signal handler
	virtual bool drawingarea_on_mouse_motion( GdkEventMotion * ev ) ;

	virtual void Draw_Partition() ;
};

#endif /* GPARTED_FRAME_RESIZER_EXTENDED_H */
