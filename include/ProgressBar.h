/* Copyright (C) 2016 Mike Fleetwood
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


#ifndef GPARTED_PROGRESSBAR_H
#define GPARTED_PROGRESSBAR_H

#include <glibmm/ustring.h>
#include <glibmm/timer.h>

namespace GParted
{

enum ProgressBar_Text
{
	PROGRESSBAR_TEXT_NONE,
	PROGRESSBAR_TEXT_COPY_BYTES
};

class ProgressBar
{
public:
	ProgressBar();
	~ProgressBar();

	void start( double target, ProgressBar_Text text_mode = PROGRESSBAR_TEXT_NONE );
	void update( double progress );
	void stop();
	bool running() const;
	double get_fraction() const;
	Glib::ustring get_text() const;

private:
	ProgressBar( const ProgressBar & src );              // Not implemented copy constructor
	ProgressBar & operator=( const ProgressBar & rhs );  // Not implemented copy assignment operator

	void do_update();

	bool              m_running;    // Is this progress bar running?
	double            m_target;     // Progress bar target should be > 0.0
	double            m_progress;   // Should be 0.0 <= m_progress <= m_target
	double            m_fraction;   // Always between 0.0 and 1.0 for passing to Gtk::ProgressBar.set_fraction()
	ProgressBar_Text  m_text_mode;  // Whether to and style of text generation
	Glib::ustring     m_text;       // Optional text for passing to Gtk::ProgressBar.set_text()
	Glib::Timer       m_timer;      // Measures elapsed time to the microsecond for accurate estimation
};

}//GParted

#endif /* GPARTED_PROGRESS_H */
