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

#include <algorithm>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "windowing/DllWaylandClient.h"
#include "utils/log.h"

#include "Wayland11EventQueueStrategy.h"

namespace xwe = xbmc::wayland::events;
namespace xw11 = xbmc::wayland::version_11;

namespace
{
void DispatchPendingEvents(IDllWaylandClient &clientLibrary,
                           struct wl_display *display)
{
  clientLibrary.wl_display_dispatch_pending(display);
  /* We flush the output queue in the main thread as that needs to
   * happen after eglSwapBuffers */
}

void ReadAndDispatch(IDllWaylandClient &clientLibrary,
                     struct wl_display *display)
{
  clientLibrary.wl_display_dispatch(display);
}
}

xw11::EventQueueStrategy::EventQueueStrategy(IDllWaylandClient &clientLibrary,
                                             struct wl_display *display) :
  m_clientLibrary(clientLibrary),
  m_display(display),
  m_thread(boost::bind(ReadAndDispatch,
                       boost::ref(m_clientLibrary),
                       m_display),
           boost::bind(DispatchPendingEvents,
                       boost::ref(m_clientLibrary),
                       m_display),
           m_clientLibrary.wl_display_get_fd(m_display))
{
}

namespace
{
void ExecuteAction(const xwe::IEventQueueStrategy::Action &action)
{
  action();
}
}

void
xw11::EventQueueStrategy::DispatchEventsFromMain()
{
  unsigned int numActions = 0;
  std::vector<Action> pendingActions;
  
  /* We only need to hold the lock while we copy out actions from the
   * queue */
  {
    CSingleLock lock(m_actionsMutex);
    numActions = m_actions.size();
    pendingActions.reserve(numActions);
    
    /* Only pump the initial queued event count otherwise if the UI
     * keeps pushing events then the loop won't finish */
    for (unsigned int index = 0; index < numActions; ++index)
    {      
      pendingActions.push_back(m_actions.front());
      m_actions.pop();
    }
  }
  
  /* Execute each of the queued up actions */
  std::for_each(pendingActions.begin(),
                pendingActions.end(),
                ExecuteAction);
  
  /* After we've done dispatching flush the event queue */
  m_clientLibrary.wl_display_flush(m_display);
}

void
xw11::EventQueueStrategy::PushAction(const Action &action)
{
  CSingleLock lock(m_actionsMutex);
  m_actions.push(action);
}

#endif
