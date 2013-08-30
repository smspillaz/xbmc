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

#include <boost/scoped_ptr.hpp>

#include "input/linux/Keymap.h"
#include "Keyboard.h"
#include "TimeoutManager.h"

class IDllXKBCommon;

struct wl_array;
struct wl_surface;
struct xkb_context;
enum wl_keyboard_key_state;

namespace xbmc
{
class IEventListener;

class KeyboardProcessor :
  public wayland::IKeyboardReceiver
{
public:

  KeyboardProcessor(IDllXKBCommon &m_xkbCommonLibrary,
                    IEventListener &listener,
                    ITimeoutManager &timeouts);
  ~KeyboardProcessor();
  
  void SetXBMCSurface(struct wl_surface *xbmcWindow);

private:

  void UpdateKeymap(uint32_t format,
                    int fd,
                    uint32_t size);
  void Enter(uint32_t serial,
             struct wl_surface *surface,
             struct wl_array *keys);
  void Leave(uint32_t serial,
             struct wl_surface *surface);
  void Key(uint32_t serial,
           uint32_t time,
           uint32_t key,
           enum wl_keyboard_key_state state);
  void Modifier(uint32_t serial,
                uint32_t depressed,
                uint32_t latched,
                uint32_t locked,
                uint32_t group);
                
  void SendKeyToXBMC(uint32_t key,
                     uint32_t sym,
                     uint32_t type);
  void RepeatCallback(uint32_t key,
                      uint32_t sym);

  IDllXKBCommon &m_xkbCommonLibrary;

  boost::scoped_ptr<ILinuxKeymap> m_keymap;
  IEventListener &m_listener;
  ITimeoutManager &m_timeouts;
  struct wl_surface *m_xbmcWindow;
  
  ITimeoutManager::CallbackPtr m_repeatCallback;
  uint32_t m_repeatSym;
  
  struct xkb_context *m_context;
};
}

#endif
