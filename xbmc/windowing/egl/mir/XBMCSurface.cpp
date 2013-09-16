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

#if defined(HAVE_MIR)

#include <sstream>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <mir_toolkit/mir_client_library.h>

#include "windowing/DllMirClient.h"

#include "Connection.h"
#include "Surface.h"
#include "XBMCSurface.h"

namespace xbmc
{
namespace mir
{
class XBMCSurface::Private :
  public ISurfaceEventProcessor
{
public:

  Private(IDllMirClient &clientLibrary,
          IDllXKBCommon &xkbCommonLibrary,
          const EventInjector &eventInjector,
          const boost::scoped_ptr<Connection> &connection);
  ~Private();

  IDllMirClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;
  EventInjector m_eventInjector;

  boost::scoped_ptr<Surface> m_surface;

private:

  void KeyboardEvent(const MirKeyEvent &);
  void MotionEvent(const MirMotionEvent &);
  void SurfaceEvent(const MirSurfaceEvent &);
};
}
}

namespace xm = xbmc::mir;

xm::XBMCSurface::Private::Private(IDllMirClient &clientLibrary,
                                  IDllXKBCommon &xkbCommonLibrary,
                                  const EventInjector &eventInjector,
                                  const boost::scoped_ptr<Connection> &connection) :
  m_clientLibrary(clientLibrary),
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_eventInjector(eventInjector)
{
  MirDisplayInfo info;
  connection->FetchDisplayInfo(info);
  
  MirSurface *surface =
    connection->CreateSurface("xbmc.mainwindow",
                              info.width,
                              info.height,
                              mir_pixel_format_argb_8888,
                              mir_buffer_usage_hardware);
  m_surface.reset(new xm::Surface(m_clientLibrary,
                                  surface,
                                  *this));
  
  m_surface->SetState(mir_surface_state_fullscreen);
  
  (*m_eventInjector.registerEventPipe)(m_clientLibrary,
                                       m_xkbCommonLibrary,
                                       *m_surface);
}

xm::XBMCSurface::Private::~Private()
{
  (*m_eventInjector.destroyEventPipe)();
}

xm::XBMCSurface::XBMCSurface(IDllMirClient &clientLibrary,
                             IDllXKBCommon &xkbCommonLibrary,
                             const EventInjector &eventInjector,
                             const boost::scoped_ptr<Connection> &connection) :
  priv(new Private(clientLibrary,
                   xkbCommonLibrary,
                   eventInjector,
                   connection))
{
}

/* A defined destructor is required such that
 * boost::scoped_ptr<Private>::~scoped_ptr is generated here */
xm::XBMCSurface::~XBMCSurface()
{
}

EGLNativeWindowType *
xm::XBMCSurface::NativeWindow() const
{
  return static_cast<EGLNativeWindowType *>(priv->m_surface->GetEGLWindow());
}

void
xm::XBMCSurface::Private::KeyboardEvent(const MirKeyEvent &event)
{
  (*m_eventInjector.handleKeyboardEvent)(event);
}

void
xm::XBMCSurface::Private::MotionEvent(const MirMotionEvent &event)
{
  (*m_eventInjector.handleMotionEvent)(event);
}

void
xm::XBMCSurface::Private::SurfaceEvent(const MirSurfaceEvent &event)
{
  (*m_eventInjector.handleSurfaceEvent)(event);
}

#endif
