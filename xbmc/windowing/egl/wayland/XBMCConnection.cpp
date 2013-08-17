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

#include <algorithm>

#include <sstream>
#include <stdexcept>
#include <queue>

#include <boost/array.hpp>
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
class RemoteGlobalInterface
{
public:

  virtual ~RemoteGlobalInterface() {}
  
  struct Constructor
  {
    const char *interfaceName;
    RemoteGlobalInterface *interface;
  };
  
  virtual void OnObjectAvailable(uint32_t name, uint32_t version) = 0;
};

class GlobalInterface :
  public RemoteGlobalInterface
{
public:

  typedef boost::function<void(uint32_t version)> AvailabilityHook;

protected:

  GlobalInterface(const AvailabilityHook &hook) :
    m_hook(hook)
  {
  }
  
  GlobalInterface()
  {
  }

  std::queue<uint32_t> & ObjectsAvailable(uint32_t minimum);

private:

  virtual void OnObjectAvailable(uint32_t name, uint32_t version);

  std::queue<uint32_t> m_availableNames;
  uint32_t m_version;
  AvailabilityHook m_hook;
};

template <typename Implementation>
class WaylandGlobalObject :
  public GlobalInterface
{
public:

  WaylandGlobalObject(uint32_t minimum,
                      struct wl_interface **interface) :
    GlobalInterface(),
    m_minimum(minimum),
    m_interface(interface)
  {
  }
  
  WaylandGlobalObject(uint32_t minimum,
                      struct wl_interface **interface,
                      const AvailabilityHook &hook) :
    GlobalInterface(hook),
    m_minimum(minimum),
    m_interface(interface)
  {
  }
  
  Implementation * FetchPending(Registry &registry);

private:

  uint32_t m_minimum;
  struct wl_interface **m_interface;
};

template <typename Implementation, typename WaylandImplementation>
class StoredGlobalInterface :
  public RemoteGlobalInterface
{
public:

  typedef boost::function<Implementation * (WaylandImplementation *)> Factory;
  typedef std::vector<boost::shared_ptr<Implementation> > Implementations;
  StoredGlobalInterface(const Factory &factory,
                        uint32_t minimum,
                        struct wl_interface **interface) :
    m_waylandObject(minimum, interface),
    m_factory(factory)
  {
  }

  StoredGlobalInterface(const Factory &factory,
                        uint32_t minimum,
                        struct wl_interface **interface,
                        const GlobalInterface::AvailabilityHook &hook) :
    m_waylandObject(minimum, interface, hook),
    m_factory(factory)
  {
  }
  
  ~StoredGlobalInterface()
  {
  }

  Implementation & GetFirst(Registry &registry);
  Implementations & Get(Registry &registry);

private:

  void OnObjectAvailable(uint32_t name,
                         uint32_t version);

  WaylandGlobalObject<WaylandImplementation> m_waylandObject;
  Factory m_factory;
  Implementations m_implementations;
};

class XBMCConnection::Private :
  public IWaylandRegistration
{
public:

  Private(IDllWaylandClient &clientLibrary,
          IDllXKBCommon &xkbCommonLibrary,
          EventInjector &eventInjector);
  ~Private();

  /* Do not call this from a non-main thread. The main thread may be
   * waiting for a wl_display.sync event to be coming through and this
   * function will merely spin until synchronized == true, for which
   * a non-main thread may be responsible for setting as true */
  void WaitForSynchronize();
  
  wayland::Display & Display();
  wayland::Compositor & Compositor();
  wayland::Shell & Shell();
  wayland::Output & Output();
  
private:

  IDllWaylandClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;
  
  EventInjector m_eventInjector;

  void Synchronize();

  bool synchronized;
  boost::scoped_ptr<Callback> synchronizeCallback;
  
  bool OnGlobalInterfaceAvailable(uint32_t name,
                                  const char *interface,
                                  uint32_t version);

  void InjectSeat();

  boost::scoped_ptr<wayland::Display> m_display;
  boost::scoped_ptr<wayland::Registry> m_registry;
  
  StoredGlobalInterface<wayland::Compositor, struct wl_compositor> m_compositor;
  StoredGlobalInterface<wayland::Shell, struct wl_shell> m_shell;
  WaylandGlobalObject<struct wl_seat> m_seat;
  StoredGlobalInterface<wayland::Output, struct wl_output> m_outputs;
  
  boost::scoped_ptr<events::IEventQueueStrategy> m_eventQueue;
};
}
}

namespace xw = xbmc::wayland;
namespace xwe = xbmc::wayland::events;
namespace xwe = xbmc::wayland::events;

void
xw::GlobalInterface::OnObjectAvailable(uint32_t name,
                                       uint32_t version)
{
  m_availableNames.push(name);
  m_version = version;
  
  if (!m_hook.empty())
    m_hook(m_version);
}

std::queue<uint32_t> &
xw::GlobalInterface::ObjectsAvailable(uint32_t minimum)
{
  if (m_version < minimum)
  {
    std::stringstream ss;
    ss << "Interface version at least "
       << minimum
       << " is not available"
       << " (less than version: "
       << m_version
       << ")";
    throw std::runtime_error(ss.str());
  }
  
  return m_availableNames;
}

template<typename Implementation>
Implementation *
xw::WaylandGlobalObject<Implementation>::FetchPending(Registry &registry)
{
  std::queue<uint32_t> &availableObjects(ObjectsAvailable(m_minimum));
  if (!availableObjects.empty())
  {
    uint32_t name = availableObjects.front();
    Implementation *proxy =
      registry.Bind<Implementation *>(name,
                                      m_interface,
                                      m_minimum);
    availableObjects.pop();
    return proxy;
  }
  
  return NULL;
}

template<typename Implementation, typename WaylandImplementation>
void
xw::StoredGlobalInterface<Implementation, WaylandImplementation>::OnObjectAvailable(uint32_t name, uint32_t version)
{
  RemoteGlobalInterface &rgi =
    static_cast<RemoteGlobalInterface &>(m_waylandObject);
  rgi.OnObjectAvailable(name, version);
}

template <typename Implementation, typename WaylandImplementation>
typename xw::StoredGlobalInterface<Implementation, WaylandImplementation>::Implementations &
xw::StoredGlobalInterface<Implementation, WaylandImplementation>::Get(Registry &registry)
{
  /* Instantiate any pending objects with this interface and then
   * return the available implementations */
  WaylandImplementation *proxy =
    m_waylandObject.FetchPending(registry);
  
  while (proxy)
  {
    boost::shared_ptr<Implementation> instance(m_factory(proxy));
    m_implementations.push_back(instance);
    proxy = m_waylandObject.FetchPending(registry);
  }

  if (m_implementations.empty())
    throw std::runtime_error("Remote interface not available");
  
  return m_implementations;
}

template <typename Implementation, typename WaylandImplementation>
Implementation &
xw::StoredGlobalInterface<Implementation, WaylandImplementation>::GetFirst(xw::Registry &registry)
{
  return *(Get(registry)[0]);
}

namespace
{
const std::string CompositorName("wl_compositor");
const std::string ShellName("wl_shell");
const std::string SeatName("wl_seat");
const std::string OutputName("wl_output");
  
xw::Compositor * CreateCompositor(struct wl_compositor *compositor,
                                  IDllWaylandClient *clientLibrary)
{
  return new xw::Compositor(*clientLibrary, compositor);
}

xw::Output * CreateOutput(struct wl_output *output,
                          IDllWaylandClient *clientLibrary)
{
  return new xw::Output(*clientLibrary, output);
}

xw::Shell * CreateShell(struct wl_shell *shell,
                        IDllWaylandClient *clientLibrary)
{
  return new xw::Shell(*clientLibrary, shell);
}

bool ConstructorMatchesInterface(const xw::RemoteGlobalInterface::Constructor &constructor,
                                 const char *interface)
{
  return std::strcmp(constructor.interfaceName,
                     interface) < 0;
}

const unsigned int RequestedCompositorVersion = 1;
const unsigned int RequestedShellVersion = 1;
const unsigned int RequestedOutputVersion = 1;
const unsigned int RequestedSeatVersion = 1;

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
  m_compositor(boost::bind(CreateCompositor, _1, &m_clientLibrary),
               RequestedCompositorVersion,
               clientLibrary.Get_wl_compositor_interface()),
  m_shell(boost::bind(CreateShell, _1, &m_clientLibrary),
          RequestedShellVersion,
          clientLibrary.Get_wl_shell_interface()),
  m_seat(RequestedSeatVersion,
         clientLibrary.Get_wl_seat_interface(),
         boost::bind(&Private::InjectSeat, this)),
  m_outputs(boost::bind(CreateOutput, _1, &m_clientLibrary),
            RequestedOutputVersion,
            clientLibrary.Get_wl_output_interface()),
  m_eventQueue(EventQueueForClientVersion(m_clientLibrary,
                                          m_display->GetWlDisplay()))
{
  (*m_eventInjector.setEventQueue)(*(m_eventQueue.get()));
	
  /* Wait only for the globals to appear, we will wait for
   * initialization upon binding them */
  WaitForSynchronize();
}

void
xw::XBMCConnection::Private::InjectSeat()
{
  struct wl_seat *seat = m_seat.FetchPending(*m_registry);
  (*m_eventInjector.setWaylandSeat)(m_clientLibrary,
                                    m_xkbCommonLibrary,
                                    seat);
}

xw::XBMCConnection::Private::~Private()
{
  (*m_eventInjector.destroyWaylandSeat)();
  (*m_eventInjector.destroyEventQueue)();
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

xw::Display &
xw::XBMCConnection::Private::Display()
{
  return *m_display;
}

xw::Compositor &
xw::XBMCConnection::Private::Compositor()
{
  return m_compositor.GetFirst(*m_registry);
}

xw::Shell &
xw::XBMCConnection::Private::Shell()
{
  return m_shell.GetFirst(*m_registry);
}

xw::Output &
xw::XBMCConnection::Private::Output()
{
  xw::Output &output(m_outputs.GetFirst(*m_registry));
  
  /* Wait for synchronize upon lazy-binding the first output
   * and then check if we got any modes */
  WaitForSynchronize();
  if (output.AllModes().empty())
  {
    std::stringstream ss;
    ss << "No modes detected on first output";
    throw std::runtime_error(ss.str());
  }
  return output;
}

bool
xw::XBMCConnection::Private::OnGlobalInterfaceAvailable(uint32_t name,
                                                        const char *interface,
                                                        uint32_t version)
{
  /* A boost::array is effectively immutable so we can leave out
   * const here */
  typedef boost::array<RemoteGlobalInterface::Constructor, 4> ConstructorArray;

  
  /* Not static, as the pointers here may change in cases where
   * Private is re-constructed */
  ConstructorArray constructors =
  {
    {
      { CompositorName.c_str(), &m_compositor },
      { OutputName.c_str(), &m_outputs },
      { SeatName.c_str(), &m_seat },
      { ShellName.c_str(), &m_shell }
    }
  };

  ConstructorArray::iterator it(std::lower_bound(constructors.begin(),
                                                 constructors.end(),
                                                 interface,
                                                 ConstructorMatchesInterface));
  if (it != constructors.end() &&
      strcmp(it->interfaceName, interface) == 0)
  {
    it->interface->OnObjectAvailable(name, version);
    return true;
  }
  
  return false;
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
  const xw::Output::ModeGeometry &current(priv->Output().CurrentMode());
  
  ResolutionInfoForMode(current, res);
}

void
xw::XBMCConnection::PreferredResolution(RESOLUTION_INFO &res) const
{
  /* Supporting only the first output device at the moment */
  const xw::Output::ModeGeometry &preferred(priv->Output().PreferredMode());
  ResolutionInfoForMode(preferred, res);
}

void
xw::XBMCConnection::AvailableResolutions(std::vector<RESOLUTION_INFO> &resolutions) const
{
  /* Supporting only the first output device at the moment */
  xw::Output &output(priv->Output());
  const std::vector<xw::Output::ModeGeometry> &m_modes(output.AllModes());

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
  return priv->Display().GetEGLNativeDisplay();
}

xw::Compositor &
xw::XBMCConnection::GetCompositor()
{
  return priv->Compositor();
}

xw::Shell &
xw::XBMCConnection::GetShell()
{
  return priv->Shell();
}

xw::Output &
xw::XBMCConnection::GetFirstOutput()
{
  return priv->Output();
}

#endif
