/*
*      Copyright (C) 2005-2013 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/
#include "system.h"

#if defined (HAVE_WAYLAND)

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "windowing/DllWaylandClient.h"
#include "utils/log.h"

#include "Wayland12EventQueueStrategy.h"

namespace xw12 = xbmc::wayland::version_12;

namespace
{
void Nothing()
{
}

void Read(IDllWaylandClient &clientLibrary,
          struct wl_display *display)
{
  /* If wl_display_prepare_read() returns a nonzero value it means
   * that we still have more events to dispatch. So let the main thread
   * dispatch all the pending events first before trying to read
   * more events from the pipe */
  if ((*(clientLibrary.wl_display_prepare_read_proc()))(display) == 0)
    (*(clientLibrary.wl_display_read_events_proc()))(display);
}
}

xw12::EventQueueStrategy::EventQueueStrategy(IDllWaylandClient &clientLibrary,
                                             struct wl_display *display) :
  m_clientLibrary(clientLibrary),
  m_display(display),
  m_thread(boost::bind(Read, boost::ref(m_clientLibrary), display),
           boost::bind(Nothing),
           m_clientLibrary.wl_display_get_fd(m_display))
{
}

void
xw12::EventQueueStrategy::DispatchEventsFromMain()
{
  m_clientLibrary.wl_display_dispatch_pending(m_display);
  /* We perform the flush here and not in the reader thread
   * as it needs to come after eglSwapBuffers */
  m_clientLibrary.wl_display_flush(m_display);
}

void
xw12::EventQueueStrategy::PushAction(const Action &action)
{
  action();
}

#endif
