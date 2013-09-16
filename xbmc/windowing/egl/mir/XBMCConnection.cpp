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

#include "guilib/Resolution.h"
#include "guilib/gui3d.h"

#include "Connection.h"
#include "XBMCConnection.h"

namespace xbmc
{
namespace mir
{
class XBMCConnection::Private
{
public:

  Private(IDllMirClient &clientLibrary,
          IDllXKBCommon &xkbCommonLibrary);
  ~Private();

  IDllMirClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;

  boost::scoped_ptr<Connection> m_connection;
};
}
}

namespace xm = xbmc::mir;

xm::XBMCConnection::Private::Private(IDllMirClient &clientLibrary,
                                     IDllXKBCommon &xkbCommonLibrary) :
  m_clientLibrary(clientLibrary),
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_connection(new xm::Connection(m_clientLibrary,
                                  NULL,
                                  "xbmc"))
{
}

xm::XBMCConnection::Private::~Private()
{
}

xm::XBMCConnection::XBMCConnection(IDllMirClient &clientLibrary,
                                   IDllXKBCommon &xkbCommonLibrary) :
  priv(new Private (clientLibrary, xkbCommonLibrary))
{
}

/* A defined destructor is required such that
 * boost::scoped_ptr<Private>::~scoped_ptr is generated here */
xm::XBMCConnection::~XBMCConnection()
{
}

void
xm::XBMCConnection::CurrentResolution(RESOLUTION_INFO &res) const
{
  MirDisplayInfo info;
    priv->m_connection->FetchDisplayInfo(info);
    
  res.iWidth = info.width;
  res.iHeight = info.height;
  
  /* The refresh rate is given as an integer in the second exponent
   * so we need to divide by 100.0f to get a floating point value */
  res.fRefreshRate = 60.0f;
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

void
xm::XBMCConnection::PreferredResolution(RESOLUTION_INFO &res) const
{
  CurrentResolution(res);
}

void
xm::XBMCConnection::AvailableResolutions(std::vector<RESOLUTION_INFO> &resolutions) const
{
  RESOLUTION_INFO info;
  CurrentResolution(info);
  resolutions.push_back(info);
}

EGLNativeDisplayType *
xm::XBMCConnection::NativeDisplay() const
{
  return static_cast<EGLNativeDisplayType *>(priv->m_connection->GetEGLDisplay());
}

const boost::scoped_ptr<xm::Connection> &
xm::XBMCConnection::GetConnection() const
{
  return priv->m_connection;
}

#endif
