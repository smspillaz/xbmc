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

#include "guilib/Resolution.h"
#include "guilib/gui3d.h"

#include "windowing/DllWaylandClient.h"
#include "windowing/DllXKBCommon.h"

#include "Callback.h"
#include "Compositor.h"
#include "Display.h"
#include "Output.h"
#include "Registry.h"
#include "Region.h"
#include "Shell.h"

#include "windowing/WaylandProtocol.h"
#include "XBMCConnection.h"

#include "windowing/wayland/Wayland11EventQueueStrategy.h"
#include "windowing/wayland/Wayland12EventQueueStrategy.h"

namespace xbmc
{
namespace wayland
{
class XBMCConnection::Private :
  public IWaylandRegistration
{
public:

  Private(IDllWaylandClient &clientLibrary,
          IDllXKBCommon &xkbCommonLibrary,
          EventInjector &eventInjector);
  ~Private();

  IDllWaylandClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;
  
  EventInjector m_eventInjector;

  boost::scoped_ptr<Display> m_display;
  boost::scoped_ptr<Registry> m_registry;
  boost::scoped_ptr<Compositor> m_compositor;
  boost::scoped_ptr<Shell> m_shell;
  
  std::vector<boost::shared_ptr<Output> > m_outputs;

  boost::scoped_ptr<events::IEventQueueStrategy> m_eventQueue;

  bool synchronized;
  boost::scoped_ptr<Callback> synchronizeCallback;

  /* Do not call this from a non-main thread. The main thread may be
   * waiting for a wl_display.sync event to be coming through and this
   * function will merely spin until synchronized == true, for which
   * a non-main thread may be responsible for setting as true */
  void WaitForSynchronize();
  void Synchronize();

  bool OnCompositorAvailable(struct wl_compositor *);
  bool OnShellAvailable(struct wl_shell *);
  bool OnSeatAvailable(struct wl_seat *);
  bool OnOutputAvailable(struct wl_output *);
};
}
}

namespace xw = xbmc::wayland;
namespace xwe = xbmc::wayland::events;

namespace
{
xwe::IEventQueueStrategy *
EventQueueForClientVersion(IDllWaylandClient &clientLibrary,
                           struct wl_display *display)
{
  /* TODO: Test for wl_display_read_events / wl_display_prepare_read */
  const bool version12 =
    clientLibrary.wl_display_read_events_proc() &&
    clientLibrary.wl_display_prepare_read_proc();
  if (version12)
    return new xw::version_12::EventQueueStrategy(clientLibrary,
                                                  display);
  else
    return new xw::version_11::EventQueueStrategy(clientLibrary,
                                                  display);
}
}

xw::XBMCConnection::Private::Private(IDllWaylandClient &clientLibrary,
                                     IDllXKBCommon &xkbCommonLibrary,
                                     EventInjector &eventInjector) :
  m_clientLibrary(clientLibrary),
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_eventInjector(eventInjector),
  m_display(new xw::Display(clientLibrary)),
  m_registry(new xw::Registry(clientLibrary,
                              m_display->GetWlDisplay(),
                              *this)),
  m_eventQueue(EventQueueForClientVersion(m_clientLibrary,
                                          m_display->GetWlDisplay()))
{
  (*m_eventInjector.setDisplay)(clientLibrary,
                                *(m_eventQueue.get()),
                                m_display->GetWlDisplay());
	
  /* Wait once for the globals to appear and then once
   * for the globals to be initialized */
  unsigned int waitCount = 2;
  while (waitCount--)
    WaitForSynchronize();

  if (m_outputs.empty())
  {
    std::stringstream ss;
    throw std::runtime_error(ss.str());
  }
}

xw::XBMCConnection::Private::~Private()
{
  (*m_eventInjector.destroyWaylandSeat)();
  (*m_eventInjector.destroyDisplay)();
}

xw::XBMCConnection::XBMCConnection(IDllWaylandClient &clientLibrary,
                                   IDllXKBCommon &xkbCommonLibrary,
                                   EventInjector &eventInjector) :
  priv(new Private (clientLibrary, xkbCommonLibrary, eventInjector))
{
}

/* A defined destructor is required such that
 * boost::scoped_ptr<Private>::~scoped_ptr is generated here */
xw::XBMCConnection::~XBMCConnection()
{
}

bool xw::XBMCConnection::Private::OnCompositorAvailable(struct wl_compositor *c)
{
  m_compositor.reset(new xw::Compositor(m_clientLibrary, c));
  return true;
}

bool xw::XBMCConnection::Private::OnShellAvailable(struct wl_shell *s)
{
  m_shell.reset(new xw::Shell(m_clientLibrary, s));
  return true;
}

bool xw::XBMCConnection::Private::OnSeatAvailable(struct wl_seat *s)
{
  (*m_eventInjector.setWaylandSeat)(m_clientLibrary,
                                    m_xkbCommonLibrary,
                                    s);
  return true;
}

bool xw::XBMCConnection::Private::OnOutputAvailable(struct wl_output *o)
{
  m_outputs.push_back(boost::shared_ptr<xw::Output>(new xw::Output(m_clientLibrary,
                                                                   o)));
  /* It is the responsibility of the caller to ensure that we have
   * recieved all the events telling us the output modes by
   * calling WaitForSynchronize() in the main thread */
  return true;
}

void xw::XBMCConnection::Private::WaitForSynchronize()
{
  boost::function<void(uint32_t)> func(boost::bind(&Private::Synchronize,
                                                   this));
  
  synchronized = false;
  synchronizeCallback.reset(new xw::Callback(m_clientLibrary,
                                             m_display->Sync(),
                                             func));

  /* For version 1.1 event queues the effect of this is going to be
   * a spin-wait. That's not exactly ideal, but we do need to
   * continuously flush the event queue */
  while (!synchronized)
    (*m_eventInjector.messagePump)();
}

void xw::XBMCConnection::Private::Synchronize()
{
  synchronized = true;
  synchronizeCallback.reset();
}

namespace
{
void ResolutionInfoForMode(const xw::Output::ModeGeometry &mode,
                           RESOLUTION_INFO &res)
{
  res.iWidth = mode.width;
  res.iHeight = mode.height;
  
  /* The refresh rate is given as an integer in the second exponent
   * so we need to divide by 100.0f to get a floating point value */
  res.fRefreshRate = mode.refresh / 100.0f;
  res.dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  res.iScreen = 0;
  res.bFullScreen = true;
  res.iSubtitles = static_cast<int>(0.965 * res.iHeight);
  res.fPixelRatio = 1.0f;
  res.iScreenWidth = res.iWidth;
  res.iScreenHeight = res.iHeight;
  res.strMode.Format("%dx%d @ %.2fp",
                     res.iScreenWidth,
                     res.iScreenHeight,
                     res.fRefreshRate);
}
}

void
xw::XBMCConnection::CurrentResolution(RESOLUTION_INFO &res) const
{
  /* Supporting only the first output device at the moment */
  const xw::Output::ModeGeometry &current(priv->m_outputs[0]->CurrentMode());
  
  ResolutionInfoForMode(current, res);
}

void
xw::XBMCConnection::PreferredResolution(RESOLUTION_INFO &res) const
{
  /* Supporting only the first output device at the moment */
  const xw::Output::ModeGeometry &preferred(priv->m_outputs[0]->PreferredMode());
  ResolutionInfoForMode(preferred, res);
}

void
xw::XBMCConnection::AvailableResolutions(std::vector<RESOLUTION_INFO> &resolutions) const
{
  /* Supporting only the first output device at the moment */
  const boost::shared_ptr <xw::Output> &output(priv->m_outputs[0]);
  const std::vector<xw::Output::ModeGeometry> &m_modes(output->AllModes());

  for (std::vector<xw::Output::ModeGeometry>::const_iterator it = m_modes.begin();
       it != m_modes.end();
       ++it)
  {
    resolutions.push_back(RESOLUTION_INFO());
    RESOLUTION_INFO &back(resolutions.back());
    
    ResolutionInfoForMode(*it, back);
  }
}

EGLNativeDisplayType *
xw::XBMCConnection::NativeDisplay() const
{
  return priv->m_display->GetEGLNativeDisplay();
}

const boost::scoped_ptr<xw::Compositor> &
xw::XBMCConnection::GetCompositor() const
{
  return priv->m_compositor;
}

const boost::scoped_ptr<xw::Shell> &
xw::XBMCConnection::GetShell() const
{
  return priv->m_shell;
}

const boost::shared_ptr<xw::Output> &
xw::XBMCConnection::GetFirstOutput() const
{
  return priv->m_outputs[0];
}

#endif
