/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#if defined(HAVE_WAYLAND)

#include <sstream>
#include <iostream>
#include <stdexcept>

#include <cstdlib>

#include <wayland-client.h>

#include "windowing/DllWaylandClient.h"
#include "windowing/WaylandProtocol.h"
#include "Display.h"

namespace xw = xbmc::wayland;

xw::Display::Display(IDllWaylandClient &clientLibrary) :
  m_clientLibrary(clientLibrary),
  m_display(m_clientLibrary.wl_display_connect(NULL))
{
  if (!m_display)
  {
    std::stringstream ss;
    ss << "Failed to connect to "
       << getenv("WAYLAND_DISPLAY");
    throw std::runtime_error(ss.str());
  }
}

xw::Display::~Display()
{
  m_clientLibrary.wl_display_flush(m_display);
  m_clientLibrary.wl_display_disconnect(m_display);
}

struct wl_display *
xw::Display::GetWlDisplay()
{
  return m_display;
}

EGLNativeDisplayType *
xw::Display::GetEGLNativeDisplay()
{
  return &m_display;
}

struct wl_callback *
xw::Display::Sync()
{
  struct wl_callback *callback =
      protocol::CreateWaylandObject<struct wl_callback *,
                                    struct wl_display *> (m_clientLibrary,
                                                          m_display,
                                                          m_clientLibrary.Get_wl_callback_interface());
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_display,
                                      WL_DISPLAY_SYNC,
                                      callback);
  return callback;
}

#endif
