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

#include "MirLibraries.h"

namespace xm = xbmc::mir;

void
xm::LoadLibrary(DllDynamic &dll)
{
  if (!dll.Load())
  {
    std::stringstream ss;
    ss << "Failed to load library "
       << dll.GetFile().c_str();

    throw std::runtime_error(ss.str());
  }
}

IDllMirClient &
xm::Libraries::ClientLibrary()
{
  return m_clientLibrary.Get();
}

IDllXKBCommon &
xm::Libraries::XKBCommonLibrary()
{
  return m_xkbCommonLibrary.Get();
}


#endif
