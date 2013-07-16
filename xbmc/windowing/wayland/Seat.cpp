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

#include <wayland-client.h>
#include <wayland-version.h>

#include "windowing/DllWaylandClient.h"
#include "windowing/WaylandProtocol.h"
#include "Seat.h"

#define WAYLAND_VERSION_GREATER_OR_EQUAL(maj, min, micro) \
  WAYLAND_MAJOR_VERSION >= maj || \
  (WAYLAND_MAJOR_VERSION == maj && WAYLAND_MINOR_VERSION >= min) || \
  (WAYLAND_MAJOR_VERSION == maj && WAYLAND_MINOR_VERSION == min && WAYLAND_MICRO_VERSION >= micro) 

namespace xw = xbmc::wayland;

const struct wl_seat_listener xw::Seat::m_listener =
{
  Seat::HandleCapabilitiesCallback
};

xw::Seat::Seat(IDllWaylandClient &clientLibrary,
               struct wl_seat *seat,
               IInputReceiver &reciever) :
  m_clientLibrary(clientLibrary),
  m_seat(seat),
  m_input(reciever),
  m_currentCapabilities(static_cast<enum wl_seat_capability>(0))
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_seat,
                                       &m_listener,
                                       reinterpret_cast<void *>(this));
}

xw::Seat::~Seat()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_seat);
}

void xw::Seat::HandleCapabilitiesCallback(void *data,
                                          struct wl_seat *seat,
                                          uint32_t cap)
{
  enum wl_seat_capability capabilities =
    static_cast<enum wl_seat_capability>(cap);
  static_cast<Seat *>(data)->HandleCapabilities(capabilities);
}

void xw::Seat::HandleCapabilities(enum wl_seat_capability cap)
{
  m_currentCapabilities = cap;
}

#endif
