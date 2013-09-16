#pragma once

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

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

class IDllMirClient;
class IDllXKBCommon;

struct RESOLUTION_INFO;

typedef struct MirMesaEGLNativeDisplay MirMesaEGLNativeDisplay;
typedef struct MirMesaEGLNativeSurface MirMesaEGLNativeSurface;
typedef MirEGLNativeDisplayType EGLNativeDisplayType;
typedef MirEGLNativeWindowType  EGLNativeWindowType;

namespace xbmc
{
namespace mir
{
class Connection;

class XBMCConnection
{
public:

  XBMCConnection(IDllMirClient &clientLibrary,
                 IDllXKBCommon &xkbCommonLibrary);
  ~XBMCConnection();
  
  void PreferredResolution(RESOLUTION_INFO &res) const;
  void CurrentResolution(RESOLUTION_INFO &res) const;
  void AvailableResolutions(std::vector<RESOLUTION_INFO> &res) const;
  
  EGLNativeDisplayType * NativeDisplay() const;
  
  const boost::scoped_ptr<Connection> & GetConnection() const;
  
private:

  class Private;
  boost::scoped_ptr<Private> priv;
};
}
}

#endif
