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
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <cstdlib>

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

#include "system.h"
#include "guilib/gui3d.h"
#include "EGLNativeTypeWayland.h"
#include "../WinEvents.h"

#if defined(HAVE_WAYLAND)
namespace
{
class WaylandRegistration
{
public:

  virtual ~WaylandRegistration() {};

  virtual bool OnCompositorAvailable(struct wl_compositor *) = 0;
  virtual bool OnShellAvailable(struct wl_shell *) = 0;
  virtual bool OnSeatAvailable(struct wl_seat *) = 0;
  virtual bool OnShmAvailable(struct wl_shm *) = 0;
  virtual bool OnOutputAvailable(struct wl_output *) = 0;
};
}

namespace xbmc
{
namespace wayland
{
class Callback :
  boost::noncopyable
{
public:

  typedef boost::function<void(uint32_t)> Func;

  Callback(struct wl_callback *callback,
           const Func &func);
  ~Callback();

  struct wl_callback * GetWlCallback();

  static const struct wl_callback_listener listener;

  static void OnCallback(void *,
                         struct wl_callback *,
                         uint32_t);

private:

  struct wl_callback *callback;
  Func func;
};

const wl_callback_listener Callback::listener =
{
  Callback::OnCallback
};

class Region :
  boost::noncopyable
{
public:

  Region(struct wl_region *);
  ~Region();
  
  struct wl_region * GetWlRegion();

  void AddRectangle(int32_t x,
                    int32_t y,
                    int32_t width,
                    int32_t height);

private:

  struct wl_region *region;
};

class Display :
  boost::noncopyable
{
  public:

    Display();
    ~Display();

    struct wl_display * GetWlDisplay();
    struct wl_callback * Sync();

  private:

    struct wl_display *display;
};

class Registry :
  boost::noncopyable
{
public:

  Registry(struct wl_display   *,
           WaylandRegistration &);
  ~Registry();

  struct wl_registry * GetWlRegistry();

  static void HandleGlobalCallback(void *, struct wl_registry *,
                                   uint32_t, const char *, uint32_t);
  static void HandleRemoveGlobalCallback(void *, struct wl_registry *,
                                         uint32_t name);

  static const std::string CompositorName;
  static const std::string ShellName;
  static const std::string SeatName;
  static const std::string ShmName;
  static const std::string OutputName;

private:

  static const struct wl_registry_listener listener;

  struct wl_registry *registry;
  WaylandRegistration &registration;

  void HandleGlobal(uint32_t, const char *, uint32_t);
  void HandleRemoveGlobal(uint32_t);
};

const std::string Registry::CompositorName("wl_compositor");
const std::string Registry::ShellName("wl_shell");
const std::string Registry::SeatName("wl_seat");
const std::string Registry::ShmName("wl_shm");
const std::string Registry::OutputName("wl_output");

const struct wl_registry_listener Registry::listener =
{
  Registry::HandleGlobalCallback,
  Registry::HandleRemoveGlobalCallback
};

class Compositor :
  boost::noncopyable
{
public:

  Compositor(struct wl_compositor *);
  ~Compositor();

  struct wl_compositor * GetWlCompositor();
  struct wl_surface * CreateSurface();
  struct wl_region * CreateRegion();

private:

  struct wl_compositor *compositor;
};

class Shell :
  boost::noncopyable
{
public:

  Shell(struct wl_shell *);
  ~Shell();

  struct wl_shell * GetWlShell();
  struct wl_shell_surface * CreateShellSurface(struct wl_surface *);

private:

  struct wl_shell *shell;
};

struct Output :
  boost::noncopyable
{
public:

  Output(struct wl_output *);
  ~Output();

  struct ModeGeometry
  {
    int32_t width;
    int32_t height;
    int32_t refresh;
  };

  struct PhysicalGeometry
  {
    int32_t x;
    int32_t y;
    int32_t physicalWidth;
    int32_t physicalHeight;
    enum wl_output_subpixel subpixelArrangement;
    enum wl_output_transform outputTransformation;
  };

  struct wl_output * GetWlOutput();

  const ModeGeometry & CurrentMode();
  const ModeGeometry & PreferredMode();

  const std::vector <ModeGeometry> & AllModes();

  const PhysicalGeometry & Geometry();
  uint32_t ScaleFactor();

  static void GeometryCallback(void *,
                               struct wl_output *,
                               int32_t,
                               int32_t,
                               int32_t,
                               int32_t,
                               int32_t,
                               const char *,
                               const char *,
                               int32_t);
  static void ModeCallback(void *,
                           struct wl_output *,
                           uint32_t,
                           int32_t,
                           int32_t,
                           int32_t);
  static void ScaleCallback(void *,
                            struct wl_output *,
                            int32_t);
  static void DoneCallback(void *,
                           struct wl_output *);

private:

  static const wl_output_listener listener;

  void Geometry(int32_t x,
                int32_t y,
                int32_t physicalWidth,
                int32_t physicalHeight,
                int32_t subpixel,
                const char *make,
                const char *model,
                int32_t transform);
  void Mode(uint32_t flags,
            int32_t width,
            int32_t height,
            int32_t refresh);
  void Scale(int32_t);
  void Done();

  struct wl_output          *output;

  PhysicalGeometry          geometry;
  std::vector<ModeGeometry> modes;

  uint32_t                  scaleFactor;

  ModeGeometry              *current;
  ModeGeometry              *preferred;
};

const wl_output_listener Output::listener = 
{
  Output::GeometryCallback,
  Output::ModeCallback,
  Output::DoneCallback,
  Output::ScaleCallback
};

class Surface :
  boost::noncopyable
{
public:

  Surface(struct wl_surface *);
  ~Surface();

  struct wl_surface * GetWlSurface();
  struct wl_callback * CreateFrameCallback();
  void SetOpaqueRegion(struct wl_region *region);
  void Commit();

private:

  struct wl_surface *surface;
};

class ShellSurface :
  boost::noncopyable
{
public:

  ShellSurface(struct wl_shell_surface *);
  ~ShellSurface();

  struct wl_shell_surface * GetWlShellSurface();
  void SetFullscreen(enum wl_shell_surface_fullscreen_method method,
                     uint32_t framerate,
                     struct wl_output *output);

  static const wl_shell_surface_listener listener;

  static void HandlePingCallback(void *,
                                 struct wl_shell_surface *,
                                 uint32_t);
  static void HandleConfigureCallback(void *,
                                      struct wl_shell_surface *,
                                      uint32_t,
                                      int32_t,
                                      int32_t);
  static void HandlePopupDoneCallback(void *,
                                      struct wl_shell_surface *);

private:

  void HandlePing(uint32_t serial);
  void HandleConfigure(uint32_t edges,
                       int32_t width,
                       int32_t height);
  void HandlePopupDone();

  struct wl_shell_surface *shell_surface;
};

const wl_shell_surface_listener ShellSurface::listener =
{
  ShellSurface::HandlePingCallback,
  ShellSurface::HandleConfigureCallback,
  ShellSurface::HandlePopupDoneCallback
};

class OpenGLSurface :
  boost::noncopyable
{
public:

  OpenGLSurface(struct wl_surface *surface,
                int32_t width,
                int32_t height);
  ~OpenGLSurface();

  struct wl_egl_window * GetWlEglWindow();
  void Resize(int width, int height);

private:

  struct wl_egl_window *egl_window;
  EGLSurface eglSurface;
  EGLDisplay eglDisplay;
};
}
}

namespace xw = xbmc::wayland;

xw::Region::Region(struct wl_region *region) :
  region(region)
{
}

xw::Region::~Region()
{
  wl_region_destroy(region);
}

struct wl_region *
xw::Region::GetWlRegion()
{
  return region;
}

void
xw::Region::AddRectangle(int32_t x,
                         int32_t y,
                         int32_t width,
                         int32_t height)
{
  wl_region_add(region, x, y, width, height);
}

xw::Callback::Callback(struct wl_callback *callback,
                       const Func &func) :
  callback(callback),
  func(func)
{
  wl_callback_add_listener(callback,
                           &listener,
                           this);
}

xw::Callback::~Callback()
{
  wl_callback_destroy(callback);
}

struct wl_callback *
xw::Callback::GetWlCallback()
{
  return callback;
}

void
xw::Callback::OnCallback(void *data,
                         struct wl_callback *callback,
                         uint32_t time)
{
  static_cast<Callback *>(data)->func(time);
}

xw::Display::Display() :
  display(wl_display_connect(NULL))
{
  if (!display)
  {
    std::stringstream ss;
    ss << "Failed to connect to "
       << getenv("WAYLAND_DISPLAY");
    throw std::runtime_error(ss.str());
  }
}

xw::Display::~Display()
{
  wl_display_flush(display);
  wl_display_disconnect(display);
}

struct wl_display *
xw::Display::GetWlDisplay()
{
  return display;
}

struct wl_callback *
xw::Display::Sync()
{
  return wl_display_sync(display);
}

xw::Registry::Registry(struct wl_display   *display,
                       WaylandRegistration &registration) :
  registry(wl_display_get_registry(display)),
  registration(registration)
{
  wl_registry_add_listener(registry, &listener, this);
}

xw::Registry::~Registry()
{
  wl_registry_destroy(registry);
}

void
xw::Registry::HandleGlobal(uint32_t name,
                           const char *interface,
                           uint32_t version)
{
  if (interface == CompositorName)
  {
    struct wl_compositor *compositor =
      static_cast<struct wl_compositor *>(wl_registry_bind(registry,
                                                           name,
                                                           &wl_compositor_interface,
                                                           1));
    registration.OnCompositorAvailable(compositor);
  }
  else if (interface == ShellName)
  {
    struct wl_shell *shell =
      static_cast<struct wl_shell *>(wl_registry_bind(registry,
                                                      name,
                                                      &wl_shell_interface,
                                                      1));
    registration.OnShellAvailable(shell);
  }
  else if (interface == SeatName)
  {
    struct wl_seat *seat =
      static_cast<struct wl_seat *>(wl_registry_bind(registry,
                                                     name,
                                                     &wl_seat_interface,
                                                     1));
    registration.OnSeatAvailable(seat);
  }
  else if (interface == ShmName)
  {
    struct wl_shm *shm =
      static_cast<struct wl_shm *>(wl_registry_bind(registry,
                                                    name,
                                                    &wl_shm_interface,
                                                    1));
    registration.OnShmAvailable(shm);
  }
  else if (interface == OutputName)
  {
    struct wl_output *output =
      static_cast<struct wl_output *>(wl_registry_bind(registry,
                                                       name,
                                                       &wl_output_interface,
                                                       1));
    registration.OnOutputAvailable(output);
  }
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

xw::Compositor::Compositor(struct wl_compositor *compositor) :
  compositor(compositor)
{
}

xw::Compositor::~Compositor()
{
  wl_compositor_destroy(compositor);
}

struct wl_compositor *
xw::Compositor::GetWlCompositor()
{
  return compositor;
}

struct wl_surface *
xw::Compositor::CreateSurface()
{
  return wl_compositor_create_surface(compositor);
}

struct wl_region *
xw::Compositor::CreateRegion()
{
  return wl_compositor_create_region(compositor);
}

xw::Shell::Shell(struct wl_shell *shell) :
  shell(shell)
{
}

xw::Shell::~Shell()
{
  wl_shell_destroy(shell);
}

struct wl_shell *
xw::Shell::GetWlShell()
{
  return shell;
}

struct wl_shell_surface *
xw::Shell::CreateShellSurface(struct wl_surface *surface)
{
  return wl_shell_get_shell_surface(shell, surface);
}

xw::Output::Output(struct wl_output *output) :
  output(output),
  scaleFactor(1.0),
  current(NULL),
  preferred(NULL)
{
  wl_output_add_listener(output,
                         &listener,
                         this);
}

xw::Output::~Output()
{
  wl_output_destroy(output);
}

struct wl_output *
xw::Output::GetWlOutput()
{
  return output;
}

const xw::Output::ModeGeometry &
xw::Output::CurrentMode()
{
  if (!current)
    throw std::logic_error("No current mode has been set by the server"
                           " yet");
  
  return *current;
}

const xw::Output::ModeGeometry &
xw::Output::PreferredMode()
{
  if (!preferred)
    throw std::logic_error("No preferred mode has been set by the "
                           " server yet");

  return *preferred;
}

const std::vector <xw::Output::ModeGeometry> &
xw::Output::AllModes()
{
  return modes;
}

const xw::Output::PhysicalGeometry &
xw::Output::Geometry()
{
  return geometry;
}

uint32_t
xw::Output::ScaleFactor()
{
  return scaleFactor;
}

void
xw::Output::GeometryCallback(void *data,
                             struct wl_output *output,
                             int32_t x,
                             int32_t y,
                             int32_t physicalWidth,
                             int32_t physicalHeight,
                             int32_t subpixelArrangement,
                             const char *make,
                             const char *model,
                             int32_t transform)
{
  return static_cast<xw::Output *>(data)->Geometry(x,
                                                   y,
                                                   physicalWidth,
                                                   physicalHeight,
                                                   subpixelArrangement,
                                                   make,
                                                   model,
                                                   transform);
}

void
xw::Output::ModeCallback(void *data,
                         struct wl_output *output,
                         uint32_t flags,
                         int32_t width,
                         int32_t height,
                         int32_t refresh)
{
  return static_cast<xw::Output *>(data)->Mode(flags,
                                               width,
                                               height,
                                               refresh);
}

void
xw::Output::DoneCallback(void *data,
                         struct wl_output *output)
{
  return static_cast<xw::Output *>(data)->Done();
}

void
xw::Output::ScaleCallback(void *data,
                          struct wl_output *output,
                          int32_t factor)
{
  return static_cast<xw::Output *>(data)->Scale(factor);
}

void
xw::Output::Geometry(int32_t x,
                     int32_t y,
                     int32_t physicalWidth,
                     int32_t physicalHeight,
                     int32_t subpixelArrangement,
                     const char *make,
                     const char *model,
                     int32_t transform)
{
  geometry.x = x;
  geometry.y = y;
  geometry.physicalWidth = physicalWidth;
  geometry.physicalHeight = physicalHeight;
  geometry.subpixelArrangement =
    static_cast<enum wl_output_subpixel>(subpixelArrangement);
  geometry.outputTransformation =
    static_cast<enum wl_output_transform>(transform);
}

void
xw::Output::Mode(uint32_t flags,
                 int32_t width,
                 int32_t height,
                 int32_t refresh)
{
  xw::Output::ModeGeometry *update = NULL;
  
  for (std::vector<ModeGeometry>::iterator it = modes.begin();
       it != modes.end();
       ++it)
  { 
    if (it->width == width &&
        it->height == height &&
        it->refresh == refresh)
    {
      update = &(*it);
      break;
    }
  }
  
  enum wl_output_mode outputFlags =
    static_cast<enum wl_output_mode>(flags);
  
  if (!update)
  {
    /* New output created */
    modes.push_back(ModeGeometry());
    ModeGeometry &next(modes.back());
    
    next.width = width;
    next.height = height;
    next.refresh = refresh;
    
    update = &next;
  }
  
  if (outputFlags & WL_OUTPUT_MODE_CURRENT)
    current = update;
  if (outputFlags & WL_OUTPUT_MODE_PREFERRED)
    preferred = update;
}

void
xw::Output::Done()
{
}

void
xw::Output::Scale(int32_t factor)
{
  scaleFactor = factor;
}

xw::Surface::Surface(struct wl_surface *surface) :
  surface(surface)
{
}

xw::Surface::~Surface()
{
  wl_surface_destroy(surface);
}

struct wl_surface *
xw::Surface::GetWlSurface()
{
  return surface;
}

struct wl_callback *
xw::Surface::CreateFrameCallback()
{
  return wl_surface_frame(surface);
}

void
xw::Surface::SetOpaqueRegion(struct wl_region *region)
{
  wl_surface_set_opaque_region(surface, region);
}

void
xw::Surface::Commit()
{
  wl_surface_commit(surface);
}

xw::ShellSurface::ShellSurface(struct wl_shell_surface *shell_surface) :
  shell_surface(shell_surface)
{
  wl_shell_surface_add_listener(shell_surface,
                                &listener,
                                this);
}

xw::ShellSurface::~ShellSurface()
{
  wl_shell_surface_destroy(shell_surface);
}

struct wl_shell_surface *
xw::ShellSurface::GetWlShellSurface()
{
  return shell_surface;
}

void
xw::ShellSurface::SetFullscreen(enum wl_shell_surface_fullscreen_method method,
                                uint32_t framerate,
                                struct wl_output *output)
{
  wl_shell_surface_set_fullscreen(shell_surface,
                                  method,
                                  framerate,
                                  output);
}

void
xw::ShellSurface::HandlePingCallback(void *data,
                                     struct wl_shell_surface *shell_surface,
                                     uint32_t serial)
{
  return static_cast<ShellSurface *>(data)->HandlePing(serial);
}

void
xw::ShellSurface::HandleConfigureCallback(void *data,
                                          struct wl_shell_surface *shell_surface,
                                          uint32_t edges,
                                          int32_t width,
                                          int32_t height)
{
  return static_cast<ShellSurface *>(data)->HandleConfigure(edges,
                                                            width,
                                                            height);
}

void
xw::ShellSurface::HandlePopupDoneCallback(void *data,
                                          struct wl_shell_surface *shell_surface)
{
  return static_cast<ShellSurface *>(data)->HandlePopupDone();
}

void
xw::ShellSurface::HandlePing(uint32_t serial)
{
  wl_shell_surface_pong(shell_surface, serial);
}

void
xw::ShellSurface::HandleConfigure(uint32_t edges,
                                  int32_t width,
                                  int32_t height)
{
}

void
xw::ShellSurface::HandlePopupDone()
{
}

xw::OpenGLSurface::OpenGLSurface(struct wl_surface *surface,
                                 int width,
                                 int height) :
  egl_window(wl_egl_window_create(surface,
                                  width,
                                  height))
{
}

xw::OpenGLSurface::~OpenGLSurface()
{
  wl_egl_window_destroy(egl_window);
}

struct wl_egl_window *
xw::OpenGLSurface::GetWlEglWindow()
{
  return egl_window;
}

void
xw::OpenGLSurface::Resize(int width, int height)
{
  wl_egl_window_resize(egl_window,
                       width,
                       height,
                       0,
                       0);
}

class CEGLNativeTypeWayland::Private :
  public WaylandRegistration
{
public:

  std::auto_ptr<xw::Display> display;
  std::auto_ptr<xw::Registry> registry;
  std::auto_ptr<xw::Compositor> compositor;
  std::auto_ptr<xw::Shell> shell;

  std::auto_ptr<xw::Surface> surface;
  std::auto_ptr<xw::ShellSurface> shellSurface;
  std::auto_ptr<xw::OpenGLSurface> glSurface;
  std::auto_ptr<xw::Callback> frameCallback;
  
  std::vector<boost::shared_ptr <xw::Output> > outputs;

  void AddFrameCallback();
  void WaitForSynchronize();

private:

  bool synchronized;
  std::auto_ptr<xw::Callback> synchronizeCallback;
  
  void Synchronize();

  bool OnCompositorAvailable(struct wl_compositor *);
  bool OnShellAvailable(struct wl_shell *);
  bool OnSeatAvailable(struct wl_seat *);
  bool OnShmAvailable(struct wl_shm *);
  bool OnOutputAvailable(struct wl_output *);

  void OnFrameCallback(uint32_t);
};
#else
class CEGLNativeTypeWayland::Private
{
};
#endif

CEGLNativeTypeWayland::CEGLNativeTypeWayland() :
  priv(new Private())
{
}

CEGLNativeTypeWayland::~CEGLNativeTypeWayland()
{
} 

#if defined(HAVE_WAYLAND)
bool CEGLNativeTypeWayland::Private::OnCompositorAvailable(struct wl_compositor *c)
{
  compositor.reset(new xw::Compositor(c));
  return true;
}

bool CEGLNativeTypeWayland::Private::OnShellAvailable(struct wl_shell *s)
{
  shell.reset(new xw::Shell(s));
  return true;
}

bool CEGLNativeTypeWayland::Private::OnSeatAvailable(struct wl_seat *s)
{
  CWinEventsWayland::SetWaylandSeat(s);
  return true;
}

bool CEGLNativeTypeWayland::Private::OnShmAvailable(struct wl_shm *s)
{
  return true;
}

bool CEGLNativeTypeWayland::Private::OnOutputAvailable(struct wl_output *o)
{
  outputs.push_back(boost::shared_ptr<xw::Output>(new xw::Output(o)));
  WaitForSynchronize();
  return true;
}

void CEGLNativeTypeWayland::Private::WaitForSynchronize()
{
  boost::function<void(uint32_t)> func(boost::bind(&Private::Synchronize,
                                                   this));
  
  synchronized = false;
  synchronizeCallback.reset(new xw::Callback(display->Sync(),
                                             func));
  while (!synchronized)
    CWinEvents::MessagePump();
}

void CEGLNativeTypeWayland::Private::Synchronize()
{
  synchronized = true;
  synchronizeCallback.reset();
}
#endif

bool CEGLNativeTypeWayland::CheckCompatibility()
{
  if (!getenv("WAYLAND_DISPLAY"))
    return false;

  return true;
}

void CEGLNativeTypeWayland::Initialize()
{
}

void CEGLNativeTypeWayland::Destroy()
{
#if defined(HAVE_WAYLAND)
  priv->registry.reset();
  priv->display.reset();
#endif
}

bool CEGLNativeTypeWayland::CreateNativeDisplay()
{
#if defined(HAVE_WAYLAND)
  try
  {
    priv->display.reset(new xw::Display());
  }
  catch (const std::runtime_error &err)
  {
    /* TODO: Use CLog */
    std::cout << err.what();
  }

  priv->registry.reset(new xw::Registry(priv->display->GetWlDisplay(),
                                        *priv));

  CWinEventsWayland::SetWaylandDisplay(priv->display->GetWlDisplay());
  priv->WaitForSynchronize();

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::CreateNativeWindow()
{
#if defined(HAVE_WAYLAND)
  struct wl_surface *wls = priv->compositor->CreateSurface();
  xw::Surface *s = new xw::Surface(wls);
  priv->surface.reset(s);

  xw::ShellSurface *ss =
    new xw::ShellSurface(priv->shell->CreateShellSurface(wls));
  priv->shellSurface.reset(ss);

  /* Supporting nly the first output device at the moment */
  const xw::Output::ModeGeometry &current(priv->outputs[0]->CurrentMode());

  xw::OpenGLSurface *os = new xw::OpenGLSurface(wls,
                                                current.width,
                                                current.height);
  priv->glSurface.reset(os);
  
  xw::Region region(priv->compositor->CreateRegion());
  
  region.AddRectangle(0, 0, current.width, current.height);
  
  priv->surface->SetOpaqueRegion(region.GetWlRegion());
  priv->surface->Commit();

  priv->AddFrameCallback();
  priv->WaitForSynchronize();

  CWinEventsWayland::SetXBMCSurface(wls);

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
#if defined(HAVE_WAYLAND)
  *nativeDisplay =
      reinterpret_cast <XBNativeDisplayType *>(priv->display.get());
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::GetNativeWindow(XBNativeDisplayType **nativeWindow) const
{
#if defined(HAVE_WAYLAND)
  *nativeWindow =
      reinterpret_cast <XBNativeWindowType *>(priv->glSurface.get());
  return true;
#else
  return false;
#endif
}  

bool CEGLNativeTypeWayland::DestroyNativeDisplay()
{
#if defined(HAVE_WAYLAND)
  CWinEventsWayland::DestroyWaylandSeat();
  CWinEventsWayland::DestroyWaylandDisplay();

  priv->shell.reset();
  priv->outputs.clear();
  priv->compositor.reset();
  
  priv->WaitForSynchronize();
  
  priv->display.reset();
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::DestroyNativeWindow()
{
#if defined(HAVE_WAYLAND)
  priv->glSurface.reset();
  priv->shellSurface.reset();
  priv->surface.reset();
  priv->frameCallback.reset();
  return true;  
#else
  return false;
#endif
}

#if defined(HAVE_WAYLAND)
namespace
{
void ResolutionInfoForMode(const xw::Output::ModeGeometry &mode,
                           RESOLUTION_INFO *res)
{
  res->iWidth = mode.width;
  res->iHeight = mode.height;
  res->fRefreshRate = mode.refresh;
  res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  res->iScreen = 0;
  res->bFullScreen = true;
  res->iSubtitles = static_cast<int>(0.965 * res->iHeight);
  res->fPixelRatio = 1.0f;
  res->iScreenWidth = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode.Format("%dx%d @ %.2fp",
                      res->iScreenWidth,
                      res->iScreenHeight,
                      res->fRefreshRate);
}
}
#endif

bool CEGLNativeTypeWayland::GetNativeResolution(RESOLUTION_INFO *res) const
{
#if defined(HAVE_WAYLAND)
  /* Supporting only the first output device at the moment */
  const xw::Output::ModeGeometry &current(priv->outputs[0]->CurrentMode());
  
  ResolutionInfoForMode(current, res);

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::SetNativeResolution(const RESOLUTION_INFO &res)
{
#if defined(HAVE_WAYLAND)
  priv->glSurface->Resize(res.iScreenWidth, res.iScreenHeight);
  
  xw::Region region(priv->compositor->CreateRegion());
  
  region.AddRectangle(0, 0, res.iScreenWidth, res.iScreenHeight);
  
  priv->surface->SetOpaqueRegion(region.GetWlRegion());
  priv->surface->Commit();
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
#if defined(HAVE_WAYLAND)
  /* Supporting only the first output device at the moment */
  const boost::shared_ptr <xw::Output> &output(priv->outputs[0]);
  const std::vector<xw::Output::ModeGeometry> &modes(output->AllModes());

  for (std::vector<xw::Output::ModeGeometry>::const_iterator it = modes.begin();
       it != modes.end();
       ++it)
  {
    resolutions.push_back(RESOLUTION_INFO());
    RESOLUTION_INFO &back(resolutions.back());
    
    ResolutionInfoForMode(*it, &back);
  }

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::GetPreferredResolution(RESOLUTION_INFO *res) const
{
#if defined(HAVE_WAYLAND)
  /* Supporting only the first output device at the moment */
  const xw::Output::ModeGeometry &preferred(priv->outputs[0]->PreferredMode());
  ResolutionInfoForMode(preferred, res);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::ShowWindow(bool show)
{
#if defined(HAVE_WAYLAND)
  if (show)
    priv->shellSurface->SetFullscreen(WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
                                      0,
                                      priv->outputs[0]->GetWlOutput());
  else
    return false;

  return true;
#else
  return false;
#endif
}

#if defined(HAVE_WAYLAND)
void CEGLNativeTypeWayland::Private::OnFrameCallback(uint32_t time)
{
  AddFrameCallback();
}

void CEGLNativeTypeWayland::Private::AddFrameCallback()
{
  frameCallback.reset(new xw::Callback(surface->CreateFrameCallback(),
                                       boost::bind(&CEGLNativeTypeWayland::Private::OnFrameCallback,
                                                   this,
                                                   _1)));
}
#endif
