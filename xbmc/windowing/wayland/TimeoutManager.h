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

#if defined (HAVE_WAYLAND)

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

namespace xbmc
{
class ITimeoutManager
{
public:
  
  typedef boost::function<void()> Callback;
  typedef boost::shared_ptr <Callback> CallbackPtr;
  
  virtual ~ITimeoutManager() {}
  virtual CallbackPtr RepeatAfterMs (const Callback &callback,
                                     uint32_t initial, 
                                     uint32_t timeout) = 0;
};
}
#endif
