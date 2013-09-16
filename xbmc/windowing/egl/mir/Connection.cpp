/*
*      Copyright (C) 2005-2013 Team XBMC
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

#include <mir_toolkit/mir_client_library.h>

#include "windowing/DllMirClient.h"
#include "Connection.h"

namespace xm = xbmc::mir;

xm::Connection::Connection(IDllMirClient &clientLibrary,
                           const char *server,
                           const char *applicationName) :
  m_clientLibrary(clientLibrary),
  m_connection(m_clientLibrary.mir_connect_sync(server, applicationName)),
  m_nativeDisplay(NULL)
{
  if (!m_clientLibrary.mir_connection_is_valid(m_connection))
  {
    std::stringstream ss;
    ss << "Failed to connect to server :"
       << std::string(server ? server : " default ")
       << " with application name : "
       << std::string(applicationName);
    throw std::runtime_error(ss.str());
  }
}

xm::Connection::~Connection()
{
  m_clientLibrary.mir_connection_release(m_connection);
}

void
xm::Connection::FetchDisplayInfo(MirDisplayInfo &info)
{ 
  m_clientLibrary.mir_connection_get_display_info(m_connection, &info);
}

MirSurface *
xm::Connection::CreateSurface(const char *surfaceName,
                              int width,
                              int height,
                              MirPixelFormat format,
                              MirBufferUsage usage)
{
  MirSurfaceParameters param =
  {
    surfaceName,
    width,
    height,
    format,
    usage
  };
  
  return m_clientLibrary.mir_connection_create_surface_sync(m_connection,
                                                            &param);
}

MirEGLNativeDisplayType *
xm::Connection::GetEGLDisplay()
{
  if (!m_nativeDisplay)
  {
    m_nativeDisplay =
      m_clientLibrary.mir_connection_get_egl_native_display(m_connection);
  }
  
  return &m_nativeDisplay;
}

#endif
