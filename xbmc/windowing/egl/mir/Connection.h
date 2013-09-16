#pragma once

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

#if defined (HAVE_MIR)

#include <boost/noncopyable.hpp>

typedef struct MirConnection MirConnection;
typedef struct MirSurface MirSurface;

enum MirPixelFormat;
enum MirBufferUsage;

class IDllMirClient;

typedef void * MirEGLNativeDisplayType;

namespace xbmc
{
namespace mir
{
class Connection :
  boost::noncopyable
{
public:

  Connection(IDllMirClient &clientLibrary,
             const char *server,
             const char *applicationName);
  ~Connection();

  MirConnection * GetMirConnection();
  
  void FetchDisplayInfo(MirDisplayInfo &);
  MirSurface * CreateSurface(const char *surfaceName,
                             int width,
                             int height,
                             MirPixelFormat pixelFormat,
                             MirBufferUsage bufferUsage);
  MirEGLNativeDisplayType * GetEGLDisplay();

private:

  IDllMirClient &m_clientLibrary;

  MirConnection *m_connection;
  MirEGLNativeDisplayType m_nativeDisplay;
};
}
}

#endif
