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

#include "ProgressBar.h"
#include "Utils.h"

#include <glibmm/ustring.h>

namespace GParted
{

ProgressBar::ProgressBar() : m_running( false ), m_target( 1.0 ), m_progress( 0.0 ), m_fraction( 0.0 ),
                             m_text_mode( PROGRESSBAR_TEXT_NONE )
{
}

ProgressBar::~ProgressBar()
{
}

void ProgressBar::start( double target, ProgressBar_Text text_mode )
{
	m_running = true;
	m_target = target;
	m_progress = 0.0;
	m_text_mode = text_mode;
	m_timer.start();
	do_update();
}

void ProgressBar::update( double progress )
{
	if ( m_running )
	{
		m_progress = progress;
		do_update();
	}
}

void ProgressBar::stop()
{
	m_running = false;
	m_timer.stop();
	do_update();
}

bool ProgressBar::running() const
{
	return m_running;
}

double ProgressBar::get_fraction() const
{
	return m_fraction;
}

Glib::ustring ProgressBar::get_text() const
{
	return m_text;
}

// Private methods

// Update m_fraction and m_text as required.
void ProgressBar::do_update()
{
	m_fraction = m_progress / m_target;
	if ( m_fraction < 0.0 )
		m_fraction = 0.0;
	else if ( m_fraction > 1.0 )
		m_fraction = 1.0;

	if ( m_text_mode == PROGRESSBAR_TEXT_COPY_BYTES )
	{
		double elapsed = m_timer.elapsed();
		if ( m_running && elapsed >= 5.0 )
		{
			/* Only show "(00:01:59 remaining)" when a progress bar has been
			 * running for at least 5 seconds to allow the data copying rate
			 * to settle down a little before estimating the remaining time.
			 */
			std::time_t remaining = Utils::round( (m_target - m_progress) /
			                                      (m_progress / elapsed) );
			m_text = String::ucompose( /* TO TRANSLATORS: looks like   1.00 MiB of 16.00 MiB copied (00:01:59 remaining) */
			                         _("%1 of %2 copied (%3 remaining)"),
			                         Utils::format_size( (long long)m_progress, 1 ),
			                         Utils::format_size( (long long)m_target, 1 ),
			                         Utils::format_time( remaining ) );
		}
		else
		{
			m_text = String::ucompose( /* TO TRANSLATORS: looks like   1.00 MiB of 16.00 MiB copied */
			                         _("%1 of %2 copied"),
			                         Utils::format_size( m_progress, 1 ),
			                         Utils::format_size( m_target, 1 ) );
		}
	}
}

}//GParted
