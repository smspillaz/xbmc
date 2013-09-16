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

#ifndef WINDOW_EVENTS_MIR_H
#define WINDOW_EVENTS_MIR_H

#pragma once
#include "windowing/WinEvents.h"

/* Having to include mir_client_library.h here is unfortunate, but
 * this is because Mir*Event are typedef anonymous structs */
#include <mir_toolkit/mir_client_library.h>

class IDllMirClient;
class IDllXKBCommon;

namespace xbmc
{
namespace mir
{
class IEventPipe;
}
}

class CWinEventsMir : public CWinEventsBase
{
public:
  CWinEventsMir();
  static bool MessagePump();
  static void RefreshDevices();
  static bool IsRemoteLowBattery();
  
  static void SetEventPipe(IDllMirClient &,
                           IDllXKBCommon &,
                           xbmc::mir::IEventPipe &);
  static void RemoveEventPipe();
  
  static void HandleKeyEvent(const MirKeyEvent &);
  static void HandleMotionEvent(const MirMotionEvent &);
  static void HandleSurfaceEvent(const MirSurfaceEvent &);
};

#endif
