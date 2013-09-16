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

/* MIR_EGL_PLATFORM pulls in mir_toolkit/mir_client_library.h which
 * will not be available if we are configured without mir support */
#if defined(HAVE_MIR)
#define MIR_EGL_PLATFORM
#endif

#include <deque>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <cstdlib>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>

#include "windowing/DllMirClient.h"
#include "windowing/DllXKBCommon.h"

#include "guilib/gui3d.h"
#include "utils/log.h"
#include "EGLNativeTypeMir.h"
#include "../WinEvents.h"
#include "Application.h"

#if defined(HAVE_MIR)
#include "mir/MirLibraries.h"
#include "mir/XBMCSurface.h"
#include "mir/XBMCConnection.h"

#include <mir_toolkit/mir_client_library.h>

namespace xm = xbmc::mir;

class CEGLNativeTypeMir::Private
{
public:

  DllMirClient m_clientLibrary;
  DllXKBCommon m_xkbCommonLibrary;
  
  boost::scoped_ptr<xm::Libraries> m_libraries;
  boost::scoped_ptr<xm::XBMCConnection> m_connection;
  boost::scoped_ptr<xm::XBMCSurface> m_surface;

private:
};
#else
class CEGLNativeTypeMir::Private
{
};
#endif

CEGLNativeTypeMir::CEGLNativeTypeMir() :
  priv(new Private())
{
}

CEGLNativeTypeMir::~CEGLNativeTypeMir()
{
} 


bool CEGLNativeTypeMir::CheckCompatibility()
{
#if defined(HAVE_MIR)
  struct stat socketStat;
  if (stat("/tmp/mir_socket", &socketStat) == -1)
  {
    CLog::Log(LOGWARNING, "%s:, stat (/tmp/mir_socket): %s",
              __FUNCTION__, strerror(errno));
    return false;
  }
  
  try
  {
	  priv->m_libraries.reset(new xm::Libraries());
  }
  catch (const std::runtime_error &err)
  {
	  CLog::Log(LOGWARNING, "%s: %s\n",
              __FUNCTION__, err.what());
    return false;
  }

  return true;
#else
  return false;
#endif
}

void CEGLNativeTypeMir::Initialize()
{
}

void CEGLNativeTypeMir::Destroy()
{
#if defined(HAVE_MIR)
  priv->m_clientLibrary.Unload();
  priv->m_xkbCommonLibrary.Unload();
#endif
}

bool CEGLNativeTypeMir::CreateNativeDisplay()
{
#if defined(HAVE_MIR)
  try
  {
    priv->m_connection.reset(new xm::XBMCConnection(priv->m_libraries->ClientLibrary(),
                                                    priv->m_libraries->XKBCommonLibrary()));
  }
  catch (const std::runtime_error &err)
  {
    CLog::Log(LOGERROR, "%s: Failed to get Mir connection: %s",
              __FUNCTION__, err.what());
    return false;
  }

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::CreateNativeWindow()
{
#if defined(HAVE_MIR)

  try
  {
    xm::XBMCSurface::EventInjector injector =
    {
      CWinEventsMir::SetEventPipe,
      CWinEventsMir::RemoveEventPipe,
      CWinEventsMir::HandleKeyEvent,
      CWinEventsMir::HandleMotionEvent,
      CWinEventsMir::HandleSurfaceEvent
    };
    
    priv->m_surface.reset(new xm::XBMCSurface(priv->m_libraries->ClientLibrary(),
                                              priv->m_libraries->XKBCommonLibrary(),
                                              injector,
                                              priv->m_connection->GetConnection()));
                                              
  }
  catch (const std::runtime_error &err)
  {
    CLog::Log(LOGERROR, "%s: Failed to create surface: %s", 
              __FUNCTION__, err.what());
    return false;
  }

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
#if defined(HAVE_MIR)
  *nativeDisplay =
    reinterpret_cast<XBNativeDisplayType *>(priv->m_connection->NativeDisplay());

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::GetNativeWindow(XBNativeDisplayType **nativeWindow) const
{
#if defined(HAVE_MIR)
  *nativeWindow =
    reinterpret_cast<XBNativeWindowType *>(priv->m_surface->NativeWindow());

  return true;
#else
  return false;
#endif
}  

bool CEGLNativeTypeMir::DestroyNativeDisplay()
{
#if defined(HAVE_MIR)
  priv->m_connection.reset();
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::DestroyNativeWindow()
{
#if defined(HAVE_MIR)
  priv->m_surface.reset();
  return true;  
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::GetNativeResolution(RESOLUTION_INFO *res) const
{
#if defined(HAVE_MIR)
  priv->m_connection->CurrentResolution(*res);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::SetNativeResolution(const RESOLUTION_INFO &res)
{
#if defined(HAVE_MIR)
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
#if defined(HAVE_MIR)
  priv->m_connection->AvailableResolutions(resolutions);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::GetPreferredResolution(RESOLUTION_INFO *res) const
{
#if defined(HAVE_MIR)
  priv->m_connection->PreferredResolution(*res);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::ShowWindow(bool show)
{
#if defined(HAVE_MIR)
  return true;
#else
  return false;
#endif
}
