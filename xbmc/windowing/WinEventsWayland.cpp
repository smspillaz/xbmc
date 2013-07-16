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

#include <memory>
#include <sstream>

#include <boost/noncopyable.hpp>
#include <boost/scope_exit.hpp>
#include <boost/scoped_ptr.hpp>

#include <sys/mman.h>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "Application.h"
#include "WindowingFactory.h"
#include "WinEvents.h"
#include "WinEventsWayland.h"

#include "DllWaylandClient.h"
#include "DllXKBCommon.h"
#include "WaylandProtocol.h"

#include "wayland/Seat.h"

namespace
{
IDllWaylandClient *g_clientLibrary = NULL;
struct wl_display *g_display = NULL;
}

namespace xbmc
{
class IEventListener
{
public:

  virtual ~IEventListener() {}
  virtual bool OnEvent(XBMC_Event &) = 0;
  virtual bool OnFocused() = 0;
  virtual bool OnUnfocused() = 0;
};

class EventDispatch :
  public IEventListener
{
public:

  bool OnEvent(XBMC_Event &);
  bool OnFocused();
  bool OnUnfocused();
};
}

namespace xw = xbmc::wayland;

namespace
{
class WaylandInput :
  public xw::IInputReceiver
{
public:

  WaylandInput(IDllWaylandClient &clientLibrary,
               IDllXKBCommon &xkbCommonLibrary,
               struct wl_seat *seat,
               xbmc::EventDispatch &dispatch);

  void SetXBMCSurface(struct wl_surface *s);

private:

  bool OnEvent(XBMC_Event &);

  IDllWaylandClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;
  boost::scoped_ptr<xw::Seat> m_seat;
};

xbmc::EventDispatch g_dispatch;
boost::scoped_ptr <WaylandInput> g_inputInstance;
}

bool xbmc::EventDispatch::OnEvent(XBMC_Event &e)
{
  return g_application.OnEvent(e);
}

bool xbmc::EventDispatch::OnFocused()
{
  g_application.m_AppFocused = true;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
  return true;
}

bool xbmc::EventDispatch::OnUnfocused()
{
  g_application.m_AppFocused = false;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
  return true;
}

WaylandInput::WaylandInput(IDllWaylandClient &clientLibrary,
                           IDllXKBCommon &xkbCommonLibrary,
                           struct wl_seat *seat,
                           xbmc::EventDispatch &dispatch) :
  m_clientLibrary(clientLibrary),
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_seat(new xw::Seat(clientLibrary, seat, *this))
{
}

void
WaylandInput::SetXBMCSurface(struct wl_surface *s)
{
}

CWinEventsWayland::CWinEventsWayland()
{
}

void CWinEventsWayland::RefreshDevices()
{
}

bool CWinEventsWayland::IsRemoteLowBattery()
{
  return false;
}

bool CWinEventsWayland::MessagePump()
{
  if (!g_display)
    return false;

  g_clientLibrary->wl_display_dispatch_pending(g_display);
  g_clientLibrary->wl_display_flush(g_display);
  g_clientLibrary->wl_display_dispatch(g_display);

  return true;
}

void CWinEventsWayland::SetWaylandDisplay(IDllWaylandClient *clientLibrary,
                                          struct wl_display *d)
{
  g_clientLibrary = clientLibrary;
  g_display = d;
}

void CWinEventsWayland::DestroyWaylandDisplay()
{
  MessagePump();
  g_display = NULL;
}

void CWinEventsWayland::SetWaylandSeat(IDllWaylandClient &clientLibrary,
                                       IDllXKBCommon &xkbCommonLibrary,
                                       struct wl_seat *s)
{
  g_inputInstance.reset(new WaylandInput(clientLibrary,
                                         xkbCommonLibrary,
                                         s,
                                         g_dispatch));
}

void CWinEventsWayland::DestroyWaylandSeat()
{
  g_inputInstance.reset();
}

void CWinEventsWayland::SetXBMCSurface(struct wl_surface *s)
{
  if (!g_inputInstance.get())
    throw std::logic_error("Must have a wl_seat set before setting "
                           "the wl_surface in CWinEventsWayland");
  
  g_inputInstance->SetXBMCSurface(s);
}

#endif
