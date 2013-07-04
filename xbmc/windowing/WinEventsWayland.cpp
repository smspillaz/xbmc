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

#include <memory>

#include <boost/noncopyable.hpp>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "Application.h"
#include "WinEvents.h"
#include "WinEventsWayland.h"

struct wl_display *display = NULL;

namespace xbmc
{
namespace wayland
{
class InputReciever
{
public:

  virtual ~InputReciever () {}

  virtual bool InsertPointer (struct wl_pointer *pointer) = 0;
  virtual bool InsertKeyboard (struct wl_keyboard *keyboard) = 0;

  virtual void RemovePointer () = 0;
  virtual void RemoveKeyboard () = 0;
};

class PointerReciever
{
public:

  virtual ~PointerReciever () {}
  virtual void Motion (uint32_t time,
                       const float &x,
                       const float &y) = 0;
  virtual void Button (uint32_t serial,
                       uint32_t time,
                       uint32_t button,
                       enum wl_pointer_button_state state) = 0;
  virtual void Axis (uint32_t time,
                     uint32_t axis,
                     float value) = 0;
};

class Keymap
{
public:

  virtual ~Keymap() {};
  
  virtual uint32_t KeysymForKeycode (uint32_t code) const = 0;
  virtual void UpdateMask (uint32_t depressed,
                           uint32_t latched,
                           uint32_t locked,
                           uint32_t group) = 0;
  virtual uint32_t CurrentModifiers () = 0;
};

class KeyboardReciever
{
public:

  virtual ~KeyboardReciever() {}

  virtual void UpdateKeymap(Keymap *) = 0;
  virtual void Enter(uint32_t serial,
                     struct wl_surface *surface,
                     struct wl_array *keys) = 0;
  virtual void Leave(uint32_t serial,
                     struct wl_surface *surface) = 0;
  virtual void Key(uint32_t serial,
                   uint32_t time,
                   uint32_t key,
                   uint32_t state) = 0;
  virtual void Modifier(uint32_t serial,
                        uint32_t depressed,
                        uint32_t latched,
                        uint32_t locked,
                        uint32_t group) = 0;
};

class Pointer
{
public:

  Pointer (struct wl_pointer *,
           PointerReciever &);
  ~Pointer ();

  struct wl_pointer * GetWlPointer ();

  static void HandleEnterCallback (void *,
                                   struct wl_pointer *,
                                   uint32_t,
                                   struct wl_surface *,
                                   wl_fixed_t, 
                                   wl_fixed_t);
  static void HandleLeaveCallback (void *,
                                   struct wl_pointer *,
                                   uint32_t,
                                   struct wl_surface *);
  static void HandleMotionCallback (void *,
                                    struct wl_pointer *,
                                    uint32_t,
                                    wl_fixed_t,
                                    wl_fixed_t);
  static void HandleButtonCallback (void *,
                                    struct wl_pointer *,
                                    uint32_t,
                                    uint32_t,
                                    uint32_t,
                                    uint32_t);
  static void HandleAxisCallback (void *,
                                  struct wl_pointer *,
                                  uint32_t,
                                  uint32_t,
                                  wl_fixed_t);

private:

  void HandleEnter (uint32_t serial,
                    struct wl_surface *surface,
                    wl_fixed_t surfaceXFixed,
                    wl_fixed_t surfaceYFixed);
  void HandleLeave (uint32_t serial,
                    struct wl_surface *surface);
  void HandleMotion (uint32_t time,
                     wl_fixed_t surfaceXFixed,
                     wl_fixed_t surfaceYFixed);
  void HandleButton (uint32_t serial,
                     uint32_t time,
                     uint32_t button,
                     uint32_t state);
  void HandleAxis (uint32_t time,
                   uint32_t axis,
                   wl_fixed_t value);

  static const struct wl_pointer_listener listener;

  struct wl_pointer *pointer;
  PointerReciever &reciever;
};

const struct wl_pointer_listener Pointer::listener =
{
  Pointer::HandleEnterCallback,
  Pointer::HandleLeaveCallback,
  Pointer::HandleMotionCallback,
  Pointer::HandleButtonCallback,
  Pointer::HandleAxisCallback
};

class Keyboard :
  public boost::noncopyable
{
};

class Seat :
  public boost::noncopyable
{
public:

  Seat (struct wl_seat *,
        InputReciever &);
  ~Seat ();

  struct wl_seat * GetWlSeat ();

  static void HandleCapabilitiesCallback (void *,
                                          struct wl_seat *,
                                          uint32_t);

private:

  static const struct wl_seat_listener listener;

  void HandleCapabilities (enum wl_seat_capability);

  struct wl_seat * seat;
  InputReciever &input;

  enum wl_seat_capability currentCapabilities;
};

const struct wl_seat_listener Seat::listener =
{
  Seat::HandleCapabilitiesCallback
};
}

class EventListener
{
public:

  virtual ~EventListener () {}
  virtual bool OnEvent (XBMC_Event &) = 0;
};

class PointerProcessor :
  public wayland::PointerReciever
{
public:

  PointerProcessor (EventListener &);

private:

  void Motion (uint32_t time,
               const float &x,
               const float &y);
  void Button (uint32_t serial,
               uint32_t time,
               uint32_t button,
               enum wl_pointer_button_state state);
  void Axis (uint32_t time,
             uint32_t axis,
             float value);

  EventListener &listener;

  uint32_t currentlyPressedButton;
  float    lastPointerX;
  float    lastPointerY;

  /* There is no defined export for these buttons -
   * wayland appears to just be using the evdev codes
   * directly */
  static const unsigned int WaylandLeftButton = 272;
  static const unsigned int WaylandMiddleButton = 273;
  static const unsigned int WaylandRightButton = 274;

  static const unsigned int WheelUpButton = 4;
  static const unsigned int WheelDownButton = 5;
  
};

class EventDispatch :
  public EventListener
{
public:

  bool OnEvent (XBMC_Event &);
};
}

namespace xw = xbmc::wayland;

namespace
{
class WaylandInput :
  public xw::InputReciever
{
public:

  WaylandInput (struct wl_seat *seat,
                xbmc::EventDispatch &dispatch);

private:

  bool InsertPointer (struct wl_pointer *);
  bool InsertKeyboard (struct wl_keyboard *);

  void RemovePointer ();
  void RemoveKeyboard ();

  bool OnEvent (XBMC_Event &);

  xbmc::PointerProcessor pointerProcessor;

  std::auto_ptr <xw::Seat> seat;
  std::auto_ptr <xw::Pointer> pointer;
};

static xbmc::EventDispatch dispatch;
static std::auto_ptr <WaylandInput> inputInstance;
}

xw::Seat::Seat(struct wl_seat *seat,
               InputReciever &reciever) :
  seat (seat),
  input (reciever),
  currentCapabilities(static_cast <enum wl_seat_capability> (0))
{
  wl_seat_add_listener (seat,
                        &listener,
                        this);
}

xw::Seat::~Seat()
{
  wl_seat_destroy (seat);
}

void xw::Seat::HandleCapabilitiesCallback(void *data,
                                          struct wl_seat *seat,
                                          uint32_t cap)
{
  enum wl_seat_capability capabilities =
    static_cast <enum wl_seat_capability> (cap);
  static_cast <Seat *> (data)->HandleCapabilities(capabilities);
}

void xw::Seat::HandleCapabilities(enum wl_seat_capability cap)
{
  enum wl_seat_capability newCaps =
    static_cast <enum wl_seat_capability> (~currentCapabilities & cap);
  enum wl_seat_capability lostCaps =
    static_cast <enum wl_seat_capability> (currentCapabilities & ~cap);

  currentCapabilities = cap;

  if (newCaps & WL_SEAT_CAPABILITY_POINTER)
    input.InsertPointer(wl_seat_get_pointer(seat));
  if (newCaps & WL_SEAT_CAPABILITY_KEYBOARD)
    input.InsertKeyboard(NULL);

  if (lostCaps & WL_SEAT_CAPABILITY_POINTER)
    input.RemovePointer();
  if (lostCaps & WL_SEAT_CAPABILITY_KEYBOARD)
    input.RemoveKeyboard();
}

xw::Pointer::Pointer (struct wl_pointer *pointer,
                      PointerReciever &reciever) :
  pointer (pointer),
  reciever (reciever)
{
  wl_pointer_add_listener (pointer,
                           &listener,
                           this);
}

xw::Pointer::~Pointer()
{
  wl_pointer_destroy(pointer);
}

void xw::Pointer::HandleEnterCallback(void *data,
                                      struct wl_pointer *pointer,
                                      uint32_t serial,
                                      struct wl_surface *surface,
                                      wl_fixed_t x,
                                      wl_fixed_t y)
{
  static_cast <Pointer *> (data)->HandleEnter (serial,
                                               surface,
                                               x,
                                               y);
}

void xw::Pointer::HandleLeaveCallback(void *data,
                                      struct wl_pointer *pointer,
                                      uint32_t serial,
                                      struct wl_surface *surface)
{
  static_cast <Pointer *> (data)->HandleLeave(serial, surface);
}

void xw::Pointer::HandleMotionCallback(void *data,
                                       struct wl_pointer *pointer,
                                       uint32_t time,
                                       wl_fixed_t x,
                                       wl_fixed_t y)
{
  static_cast <Pointer *> (data)->HandleMotion(time,
                                               x,
                                               y);
}

void xw::Pointer::HandleButtonCallback(void *data,
                                       struct wl_pointer * pointer,
                                       uint32_t serial,
                                       uint32_t time,
                                       uint32_t button,
                                       uint32_t state)
{
  static_cast <Pointer *> (data)->HandleButton(serial,
                                               time,
                                               button,
                                               state);
}

void xw::Pointer::HandleAxisCallback(void *data,
                                     struct wl_pointer *pointer,
                                     uint32_t time,
                                     uint32_t axis,
                                     wl_fixed_t value)
{
  static_cast <Pointer *> (data)->HandleAxis (time,
                                              axis,
                                              value);
}

void xw::Pointer::HandleEnter(uint32_t serial,
                              struct wl_surface *surface,
                              wl_fixed_t surfaceXFixed,
                              wl_fixed_t surfaceYFixed)
{
}

void xw::Pointer::HandleLeave(uint32_t serial,
                              struct wl_surface *surface)
{
}

void xw::Pointer::HandleMotion(uint32_t time,
                               wl_fixed_t surfaceXFixed,
                               wl_fixed_t surfaceYFixed)
{
  reciever.Motion(time,
                  wl_fixed_to_double (surfaceXFixed),
                  wl_fixed_to_double (surfaceYFixed));
}

void xw::Pointer::HandleButton(uint32_t serial,
                               uint32_t time,
                               uint32_t button,
                               uint32_t state)
{
  reciever.Button(serial,
                  time,
                  button,
                  static_cast <enum wl_pointer_button_state> (state));
}

void xw::Pointer::HandleAxis(uint32_t time,
                             uint32_t axis,
                             wl_fixed_t value)
{
  reciever.Axis(time,
                axis,
                wl_fixed_to_double (value));
}

xbmc::PointerProcessor::PointerProcessor(EventListener &listener) :
  listener (listener)
{
}

void xbmc::PointerProcessor::Motion(uint32_t time,
                                    const float &x,
                                    const float &y)
{
  XBMC_Event event;

  event.type = XBMC_MOUSEMOTION;
  event.motion.xrel = ::round (x);
  event.motion.yrel = ::round (y);
  event.motion.state = 0;
  event.motion.type = XBMC_MOUSEMOTION;
  event.motion.which = 0;
  event.motion.x = event.motion.xrel;
  event.motion.y = event.motion.yrel;

  lastPointerX = x;
  lastPointerY = y;

  listener.OnEvent(event);
}

void xbmc::PointerProcessor::Button(uint32_t serial,
                                    uint32_t time,
                                    uint32_t button,
                                    enum wl_pointer_button_state state)
{
  struct ButtonTable
  {
    unsigned int WaylandButton;
    unsigned int XBMCButton;
  } buttonTable[] =
  {
    { WaylandLeftButton, 1 },
    { WaylandMiddleButton, 2 },
    { WaylandRightButton, 3 }
  };

  size_t buttonTableSize = sizeof (buttonTable) / sizeof (buttonTable[0]);

  unsigned int xbmcButton = 0;

  for (size_t i = 0; i < buttonTableSize; ++i)
    if (buttonTable[i].WaylandButton == button)
      xbmcButton = buttonTable[i].XBMCButton;

  if (!xbmcButton)
    return;

  if (state & WL_POINTER_BUTTON_STATE_PRESSED)
    currentlyPressedButton |= 1 << button;
  else if (state & WL_POINTER_BUTTON_STATE_RELEASED)
    currentlyPressedButton &= ~(1 << button);

  XBMC_Event event;

  event.type = state & WL_POINTER_BUTTON_STATE_PRESSED ?
               XBMC_MOUSEBUTTONDOWN : XBMC_MOUSEBUTTONUP;
  event.button.button = xbmcButton;
  event.button.state = 0;
  event.button.type = event.type;
  event.button.which = 0;
  event.button.x = ::round (lastPointerX);
  event.button.y = ::round (lastPointerY);

  listener.OnEvent(event);
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
    event.button.x = ::round (lastPointerX);
    event.button.y = ::round (lastPointerY);

    listener.OnEvent(event);
    
    /* We must also send a button up on the same
     * wheel "button" */
    event.type = XBMC_MOUSEBUTTONUP;
    event.button.type = XBMC_MOUSEBUTTONUP;
    
    listener.OnEvent(event);
  }
}

bool xbmc::EventDispatch::OnEvent(XBMC_Event &e)
{
  return g_application.OnEvent(e);
}

WaylandInput::WaylandInput (struct wl_seat *seat,
                            xbmc::EventDispatch &dispatch) :
  pointerProcessor (dispatch),
  seat (new xw::Seat (seat, *this))
{
}

bool WaylandInput::InsertPointer (struct wl_pointer *p)
{
  if (pointer.get ())
    return false;

  pointer.reset (new xw::Pointer (p, pointerProcessor));
  return true;
}

bool WaylandInput::InsertKeyboard (struct wl_keyboard *k)
{
  return false;
}

void WaylandInput::RemovePointer()
{
  pointer.reset ();
}

void WaylandInput::RemoveKeyboard()
{
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
  if (!display)
    return false;

  wl_display_dispatch_pending (display);
  wl_display_flush (display);
  wl_display_dispatch (display);

  return true;
}

void CWinEventsWayland::SetWaylandDisplay (struct wl_display *d)
{
  display = d;
}

void CWinEventsWayland::DestroyWaylandDisplay ()
{
  MessagePump();
  display = NULL;
}

void CWinEventsWayland::SetWaylandSeat (struct wl_seat *s)
{
  inputInstance.reset (new WaylandInput (s, dispatch));
}

void CWinEventsWayland::DestroyWaylandSeat ()
{
  inputInstance.reset ();
}

#endif
