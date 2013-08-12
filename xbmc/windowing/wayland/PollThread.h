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

#if defined(HAVE_WAYLAND)

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include "threads/Thread.h"

namespace xbmc
{
namespace wayland
{
namespace events
{
class PollThread :
  public CThread
{
public:

  typedef boost::function<void()> Dispatch;

  PollThread(const Dispatch &dispatch,
             const Dispatch &beforePoll,
             int fd);
  ~PollThread();

private:

  void Process();

  Dispatch m_dispatch;
  Dispatch m_beforePoll;
  int m_fd;
  int m_wakeupPipe[2];
};
}
}
}

#endif
