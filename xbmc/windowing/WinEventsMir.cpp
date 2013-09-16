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

#include <algorithm>
#include <sstream>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scope_exit.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <sys/poll.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "Application.h"
#include "WindowingFactory.h"
#include "WinEvents.h"
#include "WinEventsMir.h"
#include "utils/Stopwatch.h"
#include "utils/log.h"

#include "DllMirClient.h"
#include "DllXKBCommon.h"

#include "input/linux/XKBCommonKeymap.h"

#include "mir/EventPipe.h"

namespace xbmc
{
namespace mir
{
class IEventListener
{
public:

  virtual ~IEventListener() {}
  virtual bool OnEvent(XBMC_Event &) = 0;
  virtual bool OnFocused() = 0;
  virtual bool OnUnfocused() = 0;
};
}

class EventDispatch :
  public mir::IEventListener
{
public:

  bool OnEvent(XBMC_Event &);
  bool OnFocused();
  bool OnUnfocused();
};
}

namespace xm = xbmc::mir;

namespace
{
class MirInput
{
public:

  MirInput(IDllMirClient &clientLibrary,
           IDllXKBCommon &xkbCommonLibrary,
           xm::IEventPipe &eventPipe,
           xbmc::EventDispatch &eventDispatch);

  void PumpEvents();
  void KeyboardEvent(const MirKeyEvent &keyEvent);
  void MotionEvent(const MirMotionEvent &motionEvent);

private:

  void MouseMotion(const float &x, const float &y);
  void MouseButton(const float &x,
                   const float &y,
                   const MirMotionAction &action,
                   const MirMotionButton &button);
  void MouseScroll();

  IDllMirClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;
  xm::IEventPipe &m_eventPipe;
  xbmc::EventDispatch &m_eventDispatch;
  
  MirMotionButton m_lastDownButtons;
};

xbmc::EventDispatch g_dispatch;
boost::scoped_ptr<MirInput> g_mirInput;
}

bool xbmc::EventDispatch::OnEvent(XBMC_Event &e)
{
  return g_application.OnEvent(e);
}

bool xbmc::EventDispatch::OnFocused()
{
  g_application.m_AppFocused = true;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
  return true;
}

bool xbmc::EventDispatch::OnUnfocused()
{
  g_application.m_AppFocused = false;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
  return true;
}

MirInput::MirInput(IDllMirClient &clientLibrary,
                   IDllXKBCommon &xkbCommonLibrary,
                   xm::IEventPipe &eventPipe,
                   xbmc::EventDispatch &eventDispatch) :
  m_clientLibrary(clientLibrary),
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_eventPipe(eventPipe),
  m_eventDispatch(eventDispatch),
  m_lastDownButtons(static_cast<MirMotionButton>(0))
{
}

void MirInput::PumpEvents()
{
  m_eventPipe.Pump();
}

void MirInput::KeyboardEvent(const MirKeyEvent &keyEvent)
{
  static const struct ModifiersTable
  {
    MirKeyModifier mirMod;
    XBMCMod xbmcMod;
  } modTable[] =
  {
    { mir_key_modifier_none, XBMCKMOD_NONE },
    { mir_key_modifier_alt, XBMCKMOD_LALT },
    { mir_key_modifier_alt_left, XBMCKMOD_LALT },
    { mir_key_modifier_alt_right, XBMCKMOD_RALT },
    { mir_key_modifier_shift, XBMCKMOD_LSHIFT },
    { mir_key_modifier_shift_left, XBMCKMOD_LSHIFT },
    { mir_key_modifier_shift_right, XBMCKMOD_RSHIFT },
    { mir_key_modifier_ctrl, XBMCKMOD_LCTRL },
    { mir_key_modifier_ctrl_left, XBMCKMOD_LCTRL },
    { mir_key_modifier_ctrl_right, XBMCKMOD_RCTRL },
    { mir_key_modifier_meta, XBMCKMOD_LMETA },
    { mir_key_modifier_meta_left, XBMCKMOD_LMETA },
    { mir_key_modifier_meta_right, XBMCKMOD_RMETA },
    { mir_key_modifier_caps_lock, XBMCKMOD_CAPS },
    { mir_key_modifier_num_lock, XBMCKMOD_NUM }
  };
  
  XBMCMod xbmcModifiers = XBMCKMOD_NONE;
  
  size_t modTableSize = sizeof(modTable) / sizeof(modTable[0]);

  for (size_t i = 0; i < modTableSize; ++i)
  {
    if (keyEvent.modifiers & (1 << modTable[i].mirMod))
      xbmcModifiers = static_cast<XBMCMod>(xbmcModifiers |
                                           modTable[i].xbmcMod);
  }

  uint32_t sym = CXKBKeymap::XBMCKeysymForKeysym(keyEvent.key_code);
  uint32_t keyEventType = 0;

  switch (keyEvent.action)
  {
    case mir_key_action_down:
      keyEventType = XBMC_KEYDOWN;
      break;
    case mir_key_action_up:
      keyEventType = XBMC_KEYUP;
      break;
    default:
      return;
  }
  
  XBMC_Event event;
  event.type = keyEventType;
  event.key.keysym.scancode = keyEvent.scan_code;
  event.key.keysym.sym = static_cast<XBMCKey>(sym);
  event.key.keysym.unicode = static_cast<XBMCKey>(sym);
  event.key.keysym.mod = xbmcModifiers;
  event.key.state = 0;
  event.key.type = event.type;
  event.key.which = '0';
  
  m_eventDispatch.OnEvent(event);
}

void MirInput::MotionEvent(const MirMotionEvent &motionEvent)
{
  switch (motionEvent.action)
  {
    case mir_motion_action_hover_move:
      MouseMotion(motionEvent.pointer_coordinates[0].x,
                  motionEvent.pointer_coordinates[0].y);
      break;
    case mir_motion_action_down:
    case mir_motion_action_up:
      MouseButton(motionEvent.pointer_coordinates[0].x,
                  motionEvent.pointer_coordinates[0].y,
                  static_cast<MirMotionAction>(motionEvent.action),
                  motionEvent.button_state);
      break;
    case mir_motion_action_scroll:
      MouseScroll();
      break;
    default:
      break;
  }
}

void MirInput::MouseMotion(const float &x,
                           const float &y)
{
  XBMC_Event event;
  
  event.type = XBMC_MOUSEMOTION;
  event.motion.xrel = ::round(x);
  event.motion.yrel = ::round(y);
  event.motion.state = 0;
  event.motion.type = XBMC_MOUSEMOTION;
  event.motion.which = 0;
  event.motion.x = event.motion.xrel;
  event.motion.y = event.motion.yrel;
  
  m_eventDispatch.OnEvent(event);
}

void MirInput::MouseButton(const float &x,
                           const float &y,
                           const MirMotionAction &action,
                           const MirMotionButton &button)
{
  if (action != mir_motion_action_down &&
      action != mir_motion_action_up)
    throw std::logic_error("Only mir_motion_action_down and mir_motion"
                           "_action_up are supported");
  
  MirMotionButton lostButtons =
    static_cast<MirMotionButton>(m_lastDownButtons & ~(button));
  m_lastDownButtons = button;
  
  MirMotionButton checkButton =
    action == mir_motion_action_down ? button : lostButtons;
  
  static const struct ButtonTable
  {
    MirMotionButton MirButton;
    unsigned int XBMCButton;
  } buttonTable[] =
  {
    { mir_motion_button_primary, 1 },
    { mir_motion_button_secondary, 2 },
    { mir_motion_button_tertiary, 3 }
  };

  size_t buttonTableSize = sizeof(buttonTable) / sizeof(buttonTable[0]);

  unsigned int xbmcButton = 0;

  /* Multiple button presses get interpreted as the
   * most significant button for now */
  for (size_t i = 0; i < buttonTableSize; ++i)
    if (buttonTable[i].MirButton & checkButton)
      xbmcButton = buttonTable[i].XBMCButton;

  if (!xbmcButton)
    return;

  XBMC_Event event;

  event.type = action == mir_motion_action_down ?
               XBMC_MOUSEBUTTONDOWN : XBMC_MOUSEBUTTONUP;
  event.button.button = xbmcButton;
  event.button.state = 0;
  event.button.type = event.type;
  event.button.which = 0;
  event.button.x = ::round(x);
  event.button.y = ::round(y);

  m_eventDispatch.OnEvent(event);
}

void MirInput::MouseScroll()
{
}

CWinEventsMir::CWinEventsMir()
{
}

void CWinEventsMir::RefreshDevices()
{
}

bool CWinEventsMir::IsRemoteLowBattery()
{
  return false;
}

bool CWinEventsMir::MessagePump()
{
  if (g_mirInput.get())
  {
    g_mirInput->PumpEvents();
    return true;
  }
  
  return false;
}

void CWinEventsMir::SetEventPipe(IDllMirClient &clientLibrary,
                                 IDllXKBCommon &xkbCommonLibrary,
                                 xm::IEventPipe &eventPipe)
{
  g_mirInput.reset(new MirInput(clientLibrary,
                                xkbCommonLibrary,
                                eventPipe,
                                g_dispatch));
}

void CWinEventsMir::RemoveEventPipe()
{
  g_mirInput.reset();
}

void CWinEventsMir::HandleKeyEvent(const MirKeyEvent &ev)
{
  if (g_mirInput)
    g_mirInput->KeyboardEvent(ev);
}

void CWinEventsMir::HandleMotionEvent(const MirMotionEvent &ev)
{
  if (g_mirInput)
    g_mirInput->MotionEvent(ev);
}

void CWinEventsMir::HandleSurfaceEvent(const MirSurfaceEvent &ev)
{
}

#endif
