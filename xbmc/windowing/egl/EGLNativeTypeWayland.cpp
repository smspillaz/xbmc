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
#define WL_EGL_PLATFORM
 
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <cstdlib>

#include <wayland-client.h>
#include <wayland-version.h>

#include "windowing/DllWaylandClient.h"
#include "windowing/DllWaylandEgl.h"
#include "windowing/DllXKBCommon.h"
#include "windowing/WaylandProtocol.h"

#include "system.h"
#include "guilib/gui3d.h"
#include "utils/log.h"
#include "EGLNativeTypeWayland.h"
#include "windowing/WinEvents.h"

#include "wayland/WaylandLibraries.h"
#include "wayland/XBMCConnection.h"
#include "wayland/XBMCSurface.h"

namespace xw = xbmc::wayland;

#if defined(HAVE_WAYLAND)
class CEGLNativeTypeWayland::Private
{
public:

  boost::scoped_ptr<xw::Libraries> m_libraries;
  boost::scoped_ptr<xw::XBMCConnection> m_connection;
  boost::scoped_ptr<xw::XBMCSurface> m_surface;

  bool LoadWaylandLibraries();
  void UnloadWaylandLibraries();
};

bool CEGLNativeTypeWayland::Private::LoadWaylandLibraries()
{
  try
  {
    m_libraries.reset(new xw::Libraries());
  }
  catch (const std::runtime_error &err)
  {
    CLog::Log(LOGWARNING, "%s: %s\n",
              __FUNCTION__, err.what());
    return false;
  }
  
  return true;
}

void CEGLNativeTypeWayland::Private::UnloadWaylandLibraries()
{
  m_libraries.reset();
}

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

bool CEGLNativeTypeWayland::CheckCompatibility()
{
#if defined(HAVE_WAYLAND)
  if (!getenv("WAYLAND_DISPLAY"))
  {
    CLog::Log(LOGWARNING, "%s:, WAYLAND_DISPLAY is not set",
              __FUNCTION__);
    return false;
  }
  
  /* FIXME:
   * There appears to be a bug in DllDynamic::CanLoad() which causes
   * it to always return false. We are just loading the library 
   * directly at CheckCompatibility time now */
  if (!priv->LoadWaylandLibraries())
    return false;

  return true;
#else
  return false;
#endif
}

void CEGLNativeTypeWayland::Initialize()
{
}

void CEGLNativeTypeWayland::Destroy()
{
#if defined(HAVE_WAYLAND)
  priv->UnloadWaylandLibraries();
#endif
}

bool CEGLNativeTypeWayland::CreateNativeDisplay()
{
#if defined(HAVE_WAYLAND)
  try
  {
    xw::XBMCConnection::EventInjector injector =
    {
      CWinEventsWayland::SetWaylandDisplay,
      CWinEventsWayland::DestroyWaylandDisplay,
      CWinEventsWayland::SetWaylandSeat,
      CWinEventsWayland::DestroyWaylandSeat,
      CWinEventsWayland::MessagePump
    };
      
    priv->m_connection.reset(new xw::XBMCConnection(priv->m_libraries->ClientLibrary(),
                                                    priv->m_libraries->XKBCommonLibrary(),
                                                    injector));
  }
  catch (const std::runtime_error &err)
  {
    CLog::Log(LOGERROR, "%s: %s", __FUNCTION__, err.what());
    return false;
  }

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::CreateNativeWindow()
{
#if defined(HAVE_WAYLAND)
  try
  {
    RESOLUTION_INFO info;
    priv->m_connection->CurrentResolution(info);

    xw::XBMCSurface::EventInjector injector =
    {
      CWinEventsWayland::SetXBMCSurface
    };

    priv->m_surface.reset(new xw::XBMCSurface(priv->m_libraries->ClientLibrary(),
                                              priv->m_libraries->EGLLibrary(),
                                              injector,
                                              priv->m_connection->GetCompositor(),
                                              priv->m_connection->GetShell(),
                                              info.iScreenWidth,
                                              info.iScreenHeight));
  }
  catch (const std::runtime_error &err)
  {
    CLog::Log(LOGERROR, "%s: %s", __FUNCTION__, err.what());
    return false;
  }

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
#if defined(HAVE_WAYLAND)
  /* We need to return a pointer to the wl_display * (eg wl_display **),
   * as EGLWrapper needs to dereference our return value to get the
   * actual display and not its first member */
  *nativeDisplay =
      reinterpret_cast <XBNativeDisplayType *>(priv->m_connection->NativeDisplay());
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::GetNativeWindow(XBNativeDisplayType **nativeWindow) const
{
#if defined(HAVE_WAYLAND)
  *nativeWindow =
      reinterpret_cast <XBNativeWindowType *>(priv->m_surface->EGLNativeWindow());
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::DestroyNativeDisplay()
{
#if defined(HAVE_WAYLAND)
  priv->m_connection.reset();
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::DestroyNativeWindow()
{
#if defined(HAVE_WAYLAND)
  priv->m_surface.reset();
  return true;  
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::GetNativeResolution(RESOLUTION_INFO *res) const
{
#if defined(HAVE_WAYLAND)
  priv->m_connection->CurrentResolution(*res);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::SetNativeResolution(const RESOLUTION_INFO &res)
{
#if defined(HAVE_WAYLAND)
  priv->m_surface->Resize(res.iScreenWidth, res.iScreenHeight);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
#if defined(HAVE_WAYLAND)
  priv->m_connection->AvailableResolutions(resolutions);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::GetPreferredResolution(RESOLUTION_INFO *res) const
{
#if defined(HAVE_WAYLAND)
  priv->m_connection->PreferredResolution(*res);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::ShowWindow(bool show)
{
#if defined(HAVE_WAYLAND)
  if (show)
    priv->m_surface->Show(priv->m_connection->GetFirstOutput());
  else
    return false;

  return true;
#else
  return false;
#endif
}
