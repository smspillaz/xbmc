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

#include <wayland-client.h>

#include "windowing/DllWaylandClient.h"
#include "windowing/WaylandProtocol.h"
#include "Registry.h"

namespace xw = xbmc::wayland;

const struct wl_registry_listener xw::Registry::m_listener =
{
  Registry::HandleGlobalCallback,
  Registry::HandleRemoveGlobalCallback
};

xw::Registry::Registry(IDllWaylandClient &clientLibrary,
                       struct wl_display *display,
                       IWaylandRegistration &registration) :
  m_clientLibrary(clientLibrary),
  m_registry(protocol::CreateWaylandObject<struct wl_registry *,
                                           struct wl_display *> (m_clientLibrary,
                                                                 display,
                                                                 m_clientLibrary.Get_wl_registry_interface())),
  m_registration(registration)
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      display,
                                      WL_DISPLAY_GET_REGISTRY,
                                      m_registry);
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_registry,
                                       &m_listener,
                                       reinterpret_cast<void *>(this));
}

xw::Registry::~Registry()
{
  protocol::DestroyWaylandObject(m_clientLibrary, m_registry);
}

void
xw::Registry::BindInternal(uint32_t name,
                           const char *interface,
                           uint32_t version,
                           void *proxy)
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_registry,
                                      WL_REGISTRY_BIND,
                                      name,
                                      interface,
                                      version,
                                      proxy);
}

void
xw::Registry::HandleGlobal(uint32_t name,
                           const char *interface,
                           uint32_t version)
{
  m_registration.OnGlobalInterfaceAvailable(name,
                                            interface,
                                            version);
}

void
xw::Registry::HandleRemoveGlobal(uint32_t name)
{
}

void
xw::Registry::HandleGlobalCallback(void *data,
                                   struct wl_registry *registry,
                                   uint32_t name,
                                   const char *interface,
                                   uint32_t version)
{
  static_cast<Registry *>(data)->HandleGlobal(name, interface, version);
}

void
xw::Registry::HandleRemoveGlobalCallback(void *data,
                                         struct wl_registry *registry,
                                         uint32_t name)
{
  static_cast<Registry *>(data)->HandleRemoveGlobal(name);
}

#endif
