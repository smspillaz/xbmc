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
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <wayland-client.h>

#include "windowing/DllWaylandClient.h"
#include "windowing/DllWaylandEgl.h"

#include "Callback.h"
#include "Compositor.h"
#include "OpenGLSurface.h"
#include "Output.h"
#include "Region.h"
#include "Shell.h"
#include "ShellSurface.h"
#include "Surface.h"

#include "windowing/WaylandProtocol.h"
#include "XBMCSurface.h"

namespace xbmc
{
namespace wayland
{
class XBMCSurface::Private
{
public:

  Private(IDllWaylandClient &clientLibrary,
          IDllWaylandEGL &eglLibrary,
          const EventInjector &eventInjector,
          const boost::scoped_ptr<Compositor> &compositor,
          const boost::scoped_ptr<Shell> &shell,
          uint32_t width,
          uint32_t height);

  typedef boost::function<struct wl_region * ()> RegionFactory;

  IDllWaylandClient &m_clientLibrary;
  IDllWaylandEGL &m_eglLibrary;
  
  EventInjector m_eventInjector;

  RegionFactory m_regionFactory;

  boost::scoped_ptr<Surface> m_surface;
  boost::scoped_ptr<ShellSurface> m_shellSurface;
  boost::scoped_ptr<OpenGLSurface> m_glSurface;
  boost::scoped_ptr<Callback> m_frameCallback;
  
  void OnFrameCallback(uint32_t);
  void AddFrameCallback();
};
}
}

namespace xw = xbmc::wayland;

xw::XBMCSurface::Private::Private(IDllWaylandClient &clientLibrary,
                                  IDllWaylandEGL &eglLibrary,
                                  const EventInjector &eventInjector,
                                  const boost::scoped_ptr<Compositor> &compositor,
                                  const boost::scoped_ptr<Shell> &shell,
                                  uint32_t width,
                                  uint32_t height) :
  m_clientLibrary(clientLibrary),
  m_eglLibrary(eglLibrary),
  m_eventInjector(eventInjector),
  m_regionFactory(boost::bind(&Compositor::CreateRegion,
                              compositor.get())),
  m_surface(new xw::Surface(m_clientLibrary,
                            compositor->CreateSurface())),
  m_shellSurface(new xw::ShellSurface(m_clientLibrary,
                                      shell->CreateShellSurface(
                                        m_surface->GetWlSurface()))),
  m_glSurface(new xw::OpenGLSurface(m_eglLibrary,
                                    m_surface->GetWlSurface(),
                                    width,
                                    height))
{
  xw::Region region(m_clientLibrary, m_regionFactory());
  
  region.AddRectangle(0, 0, 640, 480);
  
  m_surface->SetOpaqueRegion(region.GetWlRegion());
  m_surface->Commit();
  
  AddFrameCallback();
  
  (*m_eventInjector.setXBMCSurface)(m_surface->GetWlSurface());
}

xw::XBMCSurface::XBMCSurface(IDllWaylandClient &clientLibrary,
                             IDllWaylandEGL &eglLibrary,
                             const EventInjector &eventInjector,
                             const boost::scoped_ptr<Compositor> &compositor,
                             const boost::scoped_ptr<Shell> &shell,
                             uint32_t width,
                             uint32_t height) :
  priv(new Private(clientLibrary,
                   eglLibrary,
                   eventInjector,
                   compositor,
                   shell,
                   width,
                   height))
{
}

/* A defined destructor is required such that
 * boost::scoped_ptr<Private>::~scoped_ptr is generated here */
xw::XBMCSurface::~XBMCSurface()
{
}

void
xw::XBMCSurface::Show(const boost::shared_ptr<xw::Output> &output)
{
  xw::Output &mutableOutput (const_cast<xw::Output &>(*output));
  
  priv->m_shellSurface->SetFullscreen(WL_SHELL_SURFACE_FULLSCREEN_METHOD_DRIVER,
                                      0,
                                      mutableOutput.GetWlOutput());
}

void
xw::XBMCSurface::Resize(uint32_t width, uint32_t height)
{
  priv->m_glSurface->Resize(width, height);
  
  xw::Region region(priv->m_clientLibrary,
                    priv->m_regionFactory());
  
  region.AddRectangle(0, 0, width, height);
  
  priv->m_surface->SetOpaqueRegion(region.GetWlRegion());
  priv->m_surface->Commit();
}

EGLNativeWindowType *
xw::XBMCSurface::EGLNativeWindow() const
{
  return priv->m_glSurface->GetEGLNativeWindow();
}

void xw::XBMCSurface::Private::OnFrameCallback(uint32_t time)
{
  AddFrameCallback();
}

void xw::XBMCSurface::Private::AddFrameCallback()
{
  m_frameCallback.reset(new xw::Callback(m_clientLibrary,
                                         m_surface->CreateFrameCallback(),
                                         boost::bind(&Private::OnFrameCallback,
                                                     this,
                                                     _1)));
}

#endif
