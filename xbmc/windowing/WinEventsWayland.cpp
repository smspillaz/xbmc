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

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "Application.h"
#include "WindowingFactory.h"
#include "WinEvents.h"
#include "WinEventsWayland.h"
#include "utils/Stopwatch.h"
#include "utils/log.h"

#include "DllWaylandClient.h"
#include "DllXKBCommon.h"
#include "WaylandProtocol.h"

#include "wayland/Seat.h"
#include "wayland/Pointer.h"
#include "wayland/Keyboard.h"
#include "wayland/EventQueueStrategy.h"

#include "input/linux/XKBCommonKeymap.h"
#include "input/linux/Keymap.h"

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

class IEventListener
{
public:

  virtual ~IEventListener() {}
  virtual void OnEvent(XBMC_Event &) = 0;
  virtual void OnFocused() = 0;
  virtual void OnUnfocused() = 0;
};

class ICursorManager
{
public:

  virtual ~ICursorManager() {}
  virtual void SetCursor(uint32_t serial,
                         struct wl_surface *surface,
                         double surfaceX,
                         double surfaceY) = 0;
};

class PointerProcessor :
  public wayland::IPointerReceiver
{
public:

  PointerProcessor(IEventListener &,
                   ICursorManager &);

private:

  void Motion(uint32_t time,
              const float &x,
              const float &y);
  void Button(uint32_t serial,
              uint32_t time,
              uint32_t button,
              enum wl_pointer_button_state state);
  void Axis(uint32_t time,
            uint32_t axis,
            float value);
  void Enter(struct wl_surface *surface,
             double surfaceX,
             double surfaceY);

  IEventListener &m_listener;
  ICursorManager &m_cursorManager;

  uint32_t m_currentlyPressedButton;
  float    m_lastPointerX;
  float    m_lastPointerY;

  /* There is no defined export for these buttons -
   * wayland appears to just be using the evdev codes
   * directly */
  static const unsigned int WaylandLeftButton = 272;
  static const unsigned int WaylandMiddleButton = 273;
  static const unsigned int WaylandRightButton = 274;

  static const unsigned int WheelUpButton = 4;
  static const unsigned int WheelDownButton = 5;
  
};



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

  boost::scoped_ptr<linux_os::IKeymap> m_keymap;
  IEventListener &m_listener;
  ITimeoutManager &m_timeouts;
  struct wl_surface *m_xbmcWindow;
  
  ITimeoutManager::CallbackPtr m_repeatCallback;
  uint32_t m_repeatSym;
  
  struct xkb_context *m_context;
};
}

namespace xw = xbmc::wayland;
namespace xxkb = xbmc::xkbcommon;
namespace xwe = xbmc::wayland::events;

namespace
{
class WaylandEventLoop :
  public xbmc::IEventListener,
  public xbmc::ITimeoutManager
{
public:

  WaylandEventLoop(IDllWaylandClient &clientLibrary,
                   xwe::IEventQueueStrategy &strategy,
                   struct wl_display *display);
  
  void Dispatch();
  
  struct CallbackTracker
  {
    typedef boost::weak_ptr <Callback> CallbackObserver;
    
    CallbackTracker(uint32_t time,
                    uint32_t initial,
                    const CallbackPtr &callback);
    
    uint32_t time;
    uint32_t remaining;
    CallbackObserver callback;
  };
  
private:

  CallbackPtr RepeatAfterMs(const Callback &callback,
                            uint32_t initial,
                            uint32_t timeout);
  void DispatchTimers();
  
  void OnEvent(XBMC_Event &);
  void OnFocused();
  void OnUnfocused();
  
  IDllWaylandClient &m_clientLibrary;
  
  struct wl_display *m_display;
  std::vector<CallbackTracker> m_callbackQueue;
  CStopWatch m_stopWatch;
  
  xwe::IEventQueueStrategy &m_eventQueue;
};

class WaylandInput :
  public xw::IInputReceiver,
  public xbmc::ICursorManager
{
public:

  WaylandInput(IDllWaylandClient &clientLibrary,
               IDllXKBCommon &xkbCommonLibrary,
               struct wl_seat *seat,
               xbmc::IEventListener &dispatch,
               xbmc::ITimeoutManager &timeouts);

  void SetXBMCSurface(struct wl_surface *s);

private:

  void SetCursor(uint32_t serial,
                 struct wl_surface *surface,
                 double surfaceX,
                 double surfaceY);

  bool InsertPointer(struct wl_pointer *);
  bool InsertKeyboard(struct wl_keyboard *);

  void RemovePointer();
  void RemoveKeyboard();

  IDllWaylandClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;

  xbmc::PointerProcessor m_pointerProcessor;
  xbmc::KeyboardProcessor m_keyboardProcessor;

  boost::scoped_ptr<xw::Seat> m_seat;
  boost::scoped_ptr<xw::Pointer> m_pointer;
  boost::scoped_ptr<xw::Keyboard> m_keyboard;
};

boost::scoped_ptr <WaylandInput> g_inputInstance;
boost::scoped_ptr <WaylandEventLoop> g_eventLoop;
}

xbmc::PointerProcessor::PointerProcessor(IEventListener &listener,
                                         ICursorManager &manager) :
  m_listener(listener),
  m_cursorManager(manager)
{
}

void xbmc::PointerProcessor::Motion(uint32_t time,
                                    const float &x,
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

  m_lastPointerX = x;
  m_lastPointerY = y;

  m_listener.OnEvent(event);
}

void xbmc::PointerProcessor::Button(uint32_t serial,
                                    uint32_t time,
                                    uint32_t button,
                                    enum wl_pointer_button_state state)
{
  static const struct ButtonTable
  {
    unsigned int WaylandButton;
    unsigned int XBMCButton;
  } buttonTable[] =
  {
    { WaylandLeftButton, 1 },
    { WaylandMiddleButton, 2 },
    { WaylandRightButton, 3 }
  };

  size_t buttonTableSize = sizeof(buttonTable) / sizeof(buttonTable[0]);

  unsigned int xbmcButton = 0;

  for (size_t i = 0; i < buttonTableSize; ++i)
    if (buttonTable[i].WaylandButton == button)
      xbmcButton = buttonTable[i].XBMCButton;

  if (!xbmcButton)
    return;

  if (state & WL_POINTER_BUTTON_STATE_PRESSED)
    m_currentlyPressedButton |= 1 << button;
  else if (state & WL_POINTER_BUTTON_STATE_RELEASED)
    m_currentlyPressedButton &= ~(1 << button);

  XBMC_Event event;

  event.type = state & WL_POINTER_BUTTON_STATE_PRESSED ?
               XBMC_MOUSEBUTTONDOWN : XBMC_MOUSEBUTTONUP;
  event.button.button = xbmcButton;
  event.button.state = 0;
  event.button.type = event.type;
  event.button.which = 0;
  event.button.x = ::round(m_lastPointerX);
  event.button.y = ::round(m_lastPointerY);

  m_listener.OnEvent(event);
}

void xbmc::PointerProcessor::Axis(uint32_t time,
                                  uint32_t axis,
                                  float value)
{
  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
  {
    /* Negative is up */
    bool direction = value < 0.0f;
    int  button = direction ? WheelUpButton :
                              WheelDownButton;

    XBMC_Event event;

    event.type = XBMC_MOUSEBUTTONDOWN;
    event.button.button = button;
    event.button.state = 0;
    event.button.type = XBMC_MOUSEBUTTONDOWN;
    event.button.which = 0;
    event.button.x = ::round(m_lastPointerX);
    event.button.y = ::round(m_lastPointerY);

    m_listener.OnEvent(event);
    
    /* We must also send a button up on the same
     * wheel "button" */
    event.type = XBMC_MOUSEBUTTONUP;
    event.button.type = XBMC_MOUSEBUTTONUP;
    
    m_listener.OnEvent(event);
  }
}

void
xbmc::PointerProcessor::Enter(struct wl_surface *surface,
                              double surfaceX,
                              double surfaceY)
{
  m_cursorManager.SetCursor(0, NULL, 0, 0);
}



xbmc::KeyboardProcessor::KeyboardProcessor(IDllXKBCommon &xkbCommonLibrary,
                                           IEventListener &listener,
                                           ITimeoutManager &timeouts) :
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_listener(listener),
  m_timeouts(timeouts),
  m_xbmcWindow(NULL),
  m_repeatSym(0),
  m_context(NULL)
{
  enum xkb_context_flags flags =
    static_cast<enum xkb_context_flags>(0);

  m_context = m_xkbCommonLibrary.xkb_context_new(flags);
  
  if (!m_context)
    throw std::runtime_error("Failed to create xkb context");
}

xbmc::KeyboardProcessor::~KeyboardProcessor()
{
  m_xkbCommonLibrary.xkb_context_unref(m_context);
}

void
xbmc::KeyboardProcessor::SetXBMCSurface(struct wl_surface *s)
{
  m_xbmcWindow = s;
}

void
xbmc::KeyboardProcessor::UpdateKeymap(uint32_t format,
                                      int fd,
                                      uint32_t size)
{
  BOOST_SCOPE_EXIT((fd))
  {
    close(fd);
  } BOOST_SCOPE_EXIT_END

  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    throw std::runtime_error("Server gave us a keymap we don't understand");

  bool successfullyCreatedKeyboard = false;
  
  /* Either throws or returns a valid xkb_keymap * */
  struct xkb_keymap *keymap =
    xxkb::ReceiveXKBKeymapFromSharedMemory(m_xkbCommonLibrary,
                                           m_context,
                                           fd,
                                           size);

  BOOST_SCOPE_EXIT((&m_xkbCommonLibrary)(&successfullyCreatedKeyboard)(keymap))
  {
    if (!successfullyCreatedKeyboard)
      m_xkbCommonLibrary.xkb_keymap_unref(keymap);
  } BOOST_SCOPE_EXIT_END

  struct xkb_state *state =
    xxkb::CreateXKBStateFromKeymap(m_xkbCommonLibrary,
                                   keymap);

  m_keymap.reset(new xxkb::XKBKeymap(m_xkbCommonLibrary,
                                     keymap,
                                     state));
  
  successfullyCreatedKeyboard = true;
}

void
xbmc::KeyboardProcessor::Enter(uint32_t serial,
                               struct wl_surface *surface,
                               struct wl_array *keys)
{
  if (surface == m_xbmcWindow)
  {
    m_listener.OnFocused();
  }
}

void
xbmc::KeyboardProcessor::Leave(uint32_t serial,
                               struct wl_surface *surface)
{
  if (surface == m_xbmcWindow)
  {
    m_listener.OnUnfocused();
  }
}

void
xbmc::KeyboardProcessor::SendKeyToXBMC(uint32_t key,
                                       uint32_t sym,
                                       uint32_t eventType)
{
  XBMC_Event event;
  event.type = eventType;
  event.key.keysym.scancode = key;
  event.key.keysym.sym = static_cast<XBMCKey>(sym);
  event.key.keysym.unicode = static_cast<XBMCKey>(sym);
  event.key.keysym.mod =
    static_cast<XBMCMod>(m_keymap->ActiveXBMCModifiers());
  event.key.state = 0;
  event.key.type = event.type;
  event.key.which = '0';

  m_listener.OnEvent(event);
}

void
xbmc::KeyboardProcessor::RepeatCallback(uint32_t key,
                                        uint32_t sym)
{
  /* Release and press the key again */
  SendKeyToXBMC(key, sym, XBMC_KEYUP);
  SendKeyToXBMC(key, sym, XBMC_KEYDOWN);
}

void
xbmc::KeyboardProcessor::Key(uint32_t serial,
                             uint32_t time,
                             uint32_t key,
                             enum wl_keyboard_key_state state)
{
  if (!m_keymap.get())
    throw std::logic_error("a keymap must be set before processing key events");

  uint32_t sym = XKB_KEY_NoSymbol;

  try
  {
    sym = m_keymap->XBMCKeysymForKeycode(key);
  }
  catch (const std::runtime_error &err)
  {
    CLog::Log(LOGERROR, "%s: Failed to process keycode %i: %s",
              __FUNCTION__, key, err.what());
    return;
  }

  uint32_t keyEventType = 0;

  switch (state)
  {
    case WL_KEYBOARD_KEY_STATE_PRESSED:
      keyEventType = XBMC_KEYDOWN;
      break;
    case WL_KEYBOARD_KEY_STATE_RELEASED:
      keyEventType = XBMC_KEYUP;
      break;
    default:
      CLog::Log(LOGERROR, "%s: Unrecognized key state", __FUNCTION__);
      return;
  }
  
  if (keyEventType == XBMC_KEYDOWN)
  {
    m_repeatCallback =
      m_timeouts.RepeatAfterMs(boost::bind (
                                 &KeyboardProcessor::RepeatCallback,
                                 this,
                                 key,
                                 sym),
                               1000,
                               250);
    m_repeatSym = sym;
  }
  else if (keyEventType == XBMC_KEYUP &&
           sym == m_repeatSym)
    m_repeatCallback.reset();
  
  SendKeyToXBMC(key, sym, keyEventType);
}

void
xbmc::KeyboardProcessor::Modifier(uint32_t serial,
                                  uint32_t depressed,
                                  uint32_t latched,
                                  uint32_t locked,
                                  uint32_t group)
{
  m_keymap->UpdateMask(depressed, latched, locked, group);
}

WaylandInput::WaylandInput(IDllWaylandClient &clientLibrary,
                           IDllXKBCommon &xkbCommonLibrary,
                           struct wl_seat *seat,
                           xbmc::IEventListener &dispatch,
                           xbmc::ITimeoutManager &timeouts) :
  m_clientLibrary(clientLibrary),
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_pointerProcessor(dispatch, *this),
  m_keyboardProcessor(m_xkbCommonLibrary, dispatch, timeouts),
  m_seat(new xw::Seat(clientLibrary, seat, *this))
{
}

void WaylandInput::SetXBMCSurface(struct wl_surface *s)
{
  m_keyboardProcessor.SetXBMCSurface(s);
}

void WaylandInput::SetCursor(uint32_t serial,
                             struct wl_surface *surface,
                             double surfaceX,
                             double surfaceY)
{
  m_pointer->SetCursor(serial, surface, surfaceX, surfaceY);
}

bool WaylandInput::InsertPointer(struct wl_pointer *p)
{
  if (m_pointer.get())
    return false;

  m_pointer.reset(new xw::Pointer(m_clientLibrary,
                                  p,
                                  m_pointerProcessor));
  return true;
}

bool WaylandInput::InsertKeyboard(struct wl_keyboard *k)
{
  if (m_keyboard.get())
    return false;

  m_keyboard.reset(new xw::Keyboard(m_clientLibrary,
                                    k,
                                    m_keyboardProcessor));
  return true;
}

void WaylandInput::RemovePointer()
{
  m_pointer.reset();
}

void WaylandInput::RemoveKeyboard()
{
  m_keyboard.reset();
}

namespace
{
void DispatchEventAction(XBMC_Event &e)
{
  g_application.OnEvent(e);
}

void DispatchFocusedAction()
{
  g_application.m_AppFocused = true;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
}

void DispatchUnfocusedAction()
{
  g_application.m_AppFocused = false;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
}
}

void WaylandEventLoop::OnEvent(XBMC_Event &e)
{
  m_eventQueue.PushAction(boost::bind(DispatchEventAction, e));
}

void WaylandEventLoop::OnFocused()
{
  m_eventQueue.PushAction(boost::bind(DispatchFocusedAction));
}

void WaylandEventLoop::OnUnfocused()
{
  m_eventQueue.PushAction(boost::bind(DispatchUnfocusedAction));
}

WaylandEventLoop::WaylandEventLoop(IDllWaylandClient &clientLibrary,
                                   xwe::IEventQueueStrategy &strategy,
                                   struct wl_display *display) :
  m_clientLibrary(clientLibrary),
  m_display(display),
  m_eventQueue(strategy)
{
  m_stopWatch.StartZero();
}

namespace
{
bool TimeoutInactive(const WaylandEventLoop::CallbackTracker &tracker)
{
  return tracker.callback.expired();
}

void SubtractTimeoutAndTrigger(WaylandEventLoop::CallbackTracker &tracker,
                               int time)
{
  int value = std::max(0, static_cast <int> (tracker.remaining - time));
  if (value == 0)
  {
    tracker.remaining = time;
    xbmc::ITimeoutManager::CallbackPtr callback (tracker.callback.lock());
    
    (*callback) ();
  }
  else
    tracker.remaining = value;
}

bool ByRemaining(const WaylandEventLoop::CallbackTracker &a,
                 const WaylandEventLoop::CallbackTracker &b)
{
  return a.remaining < b.remaining; 
}
}

WaylandEventLoop::CallbackTracker::CallbackTracker(uint32_t time,
                                                   uint32_t initial,
                                                   const xbmc::ITimeoutManager::CallbackPtr &cb) :
  time(time),
  remaining(time > initial ? time : initial),
  callback(cb)
{
}

void WaylandEventLoop::DispatchTimers()
{
  float elapsedMs = m_stopWatch.GetElapsedMilliseconds();
  m_stopWatch.Stop();
  std::for_each(m_callbackQueue.begin(), m_callbackQueue.end (),
                boost::bind(SubtractTimeoutAndTrigger,
                            _1,
                            static_cast<int>(elapsedMs)));
  std::sort(m_callbackQueue.begin(), m_callbackQueue.end(),
            ByRemaining);
  m_stopWatch.StartZero();
}

void WaylandEventLoop::Dispatch()
{
  /* Remove any timers which are no longer active */
  m_callbackQueue.erase (std::remove_if(m_callbackQueue.begin(),
                                        m_callbackQueue.end(),
                                        TimeoutInactive),
                         m_callbackQueue.end());

  DispatchTimers();
  
  /* Calculate the poll timeout based on any current
   * timers on the main loop. */
  uint32_t minTimeout = 0;
  for (std::vector<CallbackTracker>::iterator it = m_callbackQueue.begin();
       it != m_callbackQueue.end();
       ++it)
  {
    if (minTimeout < it->remaining)
      minTimeout = it->remaining;
  }
  
  m_eventQueue.DispatchEventsFromMain();
}

xbmc::ITimeoutManager::CallbackPtr
WaylandEventLoop::RepeatAfterMs(const xbmc::ITimeoutManager::Callback &cb,
                                uint32_t initial,
                                uint32_t time)
{
  CallbackPtr ptr(new Callback(cb));
  
  bool     inserted = false;
  
  for (std::vector<CallbackTracker>::iterator it = m_callbackQueue.begin();
       it != m_callbackQueue.end();
       ++it)
  {
    /* The appropriate place to insert is just before an existing
     * timer which has a greater remaining time than ours */
    if (it->remaining > time)
    {
      m_callbackQueue.insert(it, CallbackTracker(time, initial, ptr));
      inserted = true;
      break;
    }
  }
  
  /* Insert at the back */
  if (!inserted)
    m_callbackQueue.push_back(CallbackTracker(time, initial, ptr));

  return ptr;
}

CWinEventsWayland::CWinEventsWayland()
{
}

void CWinEventsWayland::RefreshDevices()
{
}

bool CWinEventsWayland::IsRemoteLowBattery()
{
  return false;
}

bool CWinEventsWayland::MessagePump()
{
  if (!g_eventLoop.get())
    return false;

  g_eventLoop->Dispatch();

  return true;
}

void CWinEventsWayland::SetWaylandDisplay(IDllWaylandClient &clientLibrary,
                                          xwe::IEventQueueStrategy &strategy,
                                          struct wl_display *d)
{
  g_eventLoop.reset(new WaylandEventLoop(clientLibrary, strategy, d));
}

void CWinEventsWayland::DestroyWaylandDisplay()
{
  g_eventLoop.reset();
}

void CWinEventsWayland::SetWaylandSeat(IDllWaylandClient &clientLibrary,
                                       IDllXKBCommon &xkbCommonLibrary,
                                       struct wl_seat *s)
{
  if (!g_eventLoop.get())
    throw std::logic_error("Must have a wl_display set before setting "
                           "the wl_seat in CWinEventsWayland ");

  g_inputInstance.reset(new WaylandInput(clientLibrary,
                                         xkbCommonLibrary,
                                         s,
                                         *g_eventLoop,
                                         *g_eventLoop));
}

void CWinEventsWayland::DestroyWaylandSeat()
{
  g_inputInstance.reset();
}

void CWinEventsWayland::SetXBMCSurface(struct wl_surface *s)
{
  if (!g_inputInstance.get())
    throw std::logic_error("Must have a wl_seat set before setting "
                           "the wl_surface in CWinEventsWayland");
  
  g_inputInstance->SetXBMCSurface(s);
}

#endif
