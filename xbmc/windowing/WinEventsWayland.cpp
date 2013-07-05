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
#include <sstream>

#include <boost/noncopyable.hpp>
#include <boost/scope_exit.hpp>

#include <sys/mman.h>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "Application.h"
#include "WindowingFactory.h"
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
                   enum wl_keyboard_key_state state) = 0;
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
public:

  Keyboard(struct wl_keyboard *,
           struct xkb_context *,
           KeyboardReciever &);
  ~Keyboard();

  struct wl_keyboard * GetWlKeyboard();

  static void HandleKeymapCallback(void *,
                                   struct wl_keyboard *,
                                   uint32_t,
                                   int,
                                   uint32_t);
  static void HandleEnterCallback(void *,
                                  struct wl_keyboard *,
                                  uint32_t,
                                  struct wl_surface *,
                                  struct wl_array *);
  static void HandleLeaveCallback(void *,
                                  struct wl_keyboard *,
                                  uint32_t,
                                  struct wl_surface *);
  static void HandleKeyCallback(void *,
                                struct wl_keyboard *,
                                uint32_t,
                                uint32_t,
                                uint32_t,
                                uint32_t);
  static void HandleModifiersCallback(void *,
                                      struct wl_keyboard *,
                                      uint32_t,
                                      uint32_t,
                                      uint32_t,
                                      uint32_t,
                                      uint32_t);

private:

  void HandleKeymap(uint32_t format,
                    int fd,
                    uint32_t size);
  void HandleEnter(uint32_t serial,
                   struct wl_surface *surface,
                   struct wl_array *keys);
  void HandleLeave(uint32_t serial,
                   struct wl_surface *surface);
  void HandleKey(uint32_t serial,
                 uint32_t time,
                 uint32_t key,
                 uint32_t state);
  void HandleModifiers(uint32_t serial,
                       uint32_t mods_depressed,
                       uint32_t mods_latched,
                       uint32_t mods_locked,
                       uint32_t group);

  static const struct wl_keyboard_listener listener;

  struct wl_keyboard *keyboard;
  struct xkb_context *context;
  KeyboardReciever &reciever;
};

const struct wl_keyboard_listener Keyboard::listener =
{
  Keyboard::HandleKeymapCallback,
  Keyboard::HandleEnterCallback,
  Keyboard::HandleLeaveCallback,
  Keyboard::HandleKeyCallback,
  Keyboard::HandleModifiersCallback
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
  virtual bool OnFocused() = 0;
  virtual bool OnUnfocused() = 0;
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

class XKBKeymap :
  public wayland::Keymap
{
public:

  XKBKeymap(struct xkb_keymap *keymap,
            struct xkb_state *state);
  ~XKBKeymap();

private:

  uint32_t KeysymForKeycode(uint32_t code) const;
  void UpdateMask(uint32_t depressed,
                  uint32_t latched,
                  uint32_t locked,
                  uint32_t group);
  uint32_t CurrentModifiers();

  struct xkb_keymap *keymap;
  struct xkb_state *state;

  xkb_mod_index_t internalLeftControlIndex;
  xkb_mod_index_t internalLeftShiftIndex;
  xkb_mod_index_t internalLeftSuperIndex;
  xkb_mod_index_t internalLeftAltIndex;
  xkb_mod_index_t internalLeftMetaIndex;

  xkb_mod_index_t internalRightControlIndex;
  xkb_mod_index_t internalRightShiftIndex;
  xkb_mod_index_t internalRightSuperIndex;
  xkb_mod_index_t internalRightAltIndex;
  xkb_mod_index_t internalRightMetaIndex;

  xkb_mod_index_t internalCapsLockIndex;
  xkb_mod_index_t internalNumLockIndex;
  xkb_mod_index_t internalModeIndex;
};

class KeyboardProcessor :
  public wayland::KeyboardReciever
{
public:

  KeyboardProcessor(EventListener &listener);
  
  void SetXBMCSurface(struct wl_surface *xbmcWindow);

private:

  void UpdateKeymap(wayland::Keymap *keymap);
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

  std::auto_ptr<wayland::Keymap> keymap;
  EventListener &listener;
  struct wl_surface *xbmcWindow;
};

class EventDispatch :
  public EventListener
{
public:

  bool OnEvent(XBMC_Event &);
  bool OnFocused();
  bool OnUnfocused();
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
  
  void SetXBMCSurface(struct wl_surface *surf);

private:

  bool InsertPointer (struct wl_pointer *);
  bool InsertKeyboard (struct wl_keyboard *);

  void RemovePointer ();
  void RemoveKeyboard ();

  bool OnEvent (XBMC_Event &);

  xbmc::PointerProcessor pointerProcessor;
  xbmc::KeyboardProcessor keyboardProcessor;

  std::auto_ptr <xw::Seat> seat;
  std::auto_ptr <xw::Pointer> pointer;
  std::auto_ptr <xw::Keyboard> keyboard;
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
    input.InsertKeyboard(wl_seat_get_keyboard(seat));

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

xw::Keyboard::Keyboard(struct wl_keyboard *keyboard,
                       struct xkb_context *context,
                       KeyboardReciever &reciever) :
  keyboard (keyboard),
  context (context),
  reciever (reciever)
{
  wl_keyboard_add_listener(keyboard,
                           &listener,
                           this);
}

xw::Keyboard::~Keyboard()
{
  wl_keyboard_destroy(keyboard);
  xkb_context_unref(context);
}

void xw::Keyboard::HandleKeymapCallback(void *data,
                                        struct wl_keyboard *keyboard,
                                        uint32_t format,
                                        int fd,
                                        uint32_t size)
{
  static_cast <Keyboard *>(data)->HandleKeymap(format,
                                               fd,
                                               size);
}

void xw::Keyboard::HandleEnterCallback(void *data,
                                       struct wl_keyboard *keyboard,
                                       uint32_t serial,
                                       struct wl_surface *surface,
                                       struct wl_array *keys)
{
  static_cast<Keyboard *>(data)->HandleEnter(serial,
                                             surface,
                                             keys);
}

void xw::Keyboard::HandleLeaveCallback(void *data,
                                       struct wl_keyboard *keyboard,
                                       uint32_t serial,
                                       struct wl_surface *surface)
{
  static_cast<Keyboard *>(data)->HandleLeave(serial,
                                             surface);
}

void xw::Keyboard::HandleKeyCallback(void *data,
                                     struct wl_keyboard *keyboard,
                                     uint32_t serial,
                                     uint32_t time,
                                     uint32_t key,
                                     uint32_t state)
{
  static_cast<Keyboard *>(data)->HandleKey(serial,
                                           time,
                                           key,
                                           state);
}

void xw::Keyboard::HandleModifiersCallback(void *data,
                                           struct wl_keyboard *keyboard,
                                           uint32_t serial,
                                           uint32_t mods_depressed,
                                           uint32_t mods_latched,
                                           uint32_t mods_locked,
                                           uint32_t group)
{
  static_cast<Keyboard *>(data)->HandleModifiers(serial,
                                                 mods_depressed,
                                                 mods_latched,
                                                 mods_locked,
                                                 group);
}

void xw::Keyboard::HandleKeymap(uint32_t format,
                                int fd,
                                uint32_t size)
{
  BOOST_SCOPE_EXIT((fd))
  {
    close(fd);
  } BOOST_SCOPE_EXIT_END

  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    throw std::runtime_error("Server gave us a keymap we don't understand");

  const char *keymapString = static_cast<const char *>(mmap(NULL,
                                                            size,
                                                            PROT_READ,
                                                            MAP_SHARED,
                                                            fd,
                                                            0));
  if (keymapString == MAP_FAILED)
  {
    std::stringstream ss;
    ss << "mmap: " << strerror(errno);
    throw std::runtime_error(ss.str());
  }

  BOOST_SCOPE_EXIT((keymapString)(size))
  {
    munmap(const_cast<void *>(static_cast<const void *>(keymapString)),
                              size);
  } BOOST_SCOPE_EXIT_END

  bool successfullyCreatedKeyboard = false;

  enum xkb_keymap_compile_flags flags =
    static_cast<enum xkb_keymap_compile_flags>(0);
  struct xkb_keymap *keymap = xkb_map_new_from_string(context,
                                                      keymapString,
                                                      XKB_KEYMAP_FORMAT_TEXT_V1,
                                                      flags);

  if (!keymap)
    throw std::runtime_error("Failed to compile keymap");

  BOOST_SCOPE_EXIT((&successfullyCreatedKeyboard)(keymap))
  {
    if (!successfullyCreatedKeyboard)
      xkb_map_unref(keymap);
  } BOOST_SCOPE_EXIT_END

  struct xkb_state *state = xkb_state_new(keymap);

  if (!state)
    throw std::runtime_error("Failed to create keyboard state");

  reciever.UpdateKeymap(new XKBKeymap(keymap, state));
  
  successfullyCreatedKeyboard = true;
}

void xw::Keyboard::HandleEnter(uint32_t serial,
                               struct wl_surface *surface,
                               struct wl_array *keys)
{
  reciever.Enter(serial, surface, keys);
}

void xw::Keyboard::HandleLeave(uint32_t serial,
                               struct wl_surface *surface)
{
  reciever.Leave(serial, surface);
}

void xw::Keyboard::HandleKey(uint32_t serial,
                             uint32_t time,
                             uint32_t key,
                             uint32_t state)
{
  reciever.Key(serial,
               time,
               key,
               static_cast <enum wl_keyboard_key_state> (state));
}

void xw::Keyboard::HandleModifiers(uint32_t serial,
                                   uint32_t mods_depressed,
                                   uint32_t mods_latched,
                                   uint32_t mods_locked,
                                   uint32_t group)
{
  reciever.Modifier(serial,
                    mods_depressed,
                    mods_latched,
                    mods_locked,
                    group);
}

xbmc::XKBKeymap::XKBKeymap(struct xkb_keymap *keymap,
                           struct xkb_state *state) :
  keymap(keymap),
  state(state),
  internalLeftControlIndex(xkb_keymap_mod_get_index(keymap,
                                                    XKB_MOD_NAME_CTRL)),
  internalLeftShiftIndex(xkb_keymap_mod_get_index(keymap,
                                                  XKB_MOD_NAME_SHIFT)),
  internalLeftSuperIndex(xkb_keymap_mod_get_index(keymap,
                                                  XKB_MOD_NAME_LOGO)),
  internalLeftAltIndex(xkb_keymap_mod_get_index(keymap,
                                                XKB_MOD_NAME_ALT)),
  internalLeftMetaIndex(xkb_keymap_mod_get_index(keymap,
                                                "Meta")),
  internalRightControlIndex(xkb_keymap_mod_get_index(keymap,
                                                     "RControl")),
  internalRightShiftIndex(xkb_keymap_mod_get_index(keymap,
                                                   "RShift")),
  internalRightSuperIndex(xkb_keymap_mod_get_index(keymap,
                                                   "Hyper")),
  internalRightAltIndex(xkb_keymap_mod_get_index(keymap,
                                                 "AltGr")),
  internalRightMetaIndex(xkb_keymap_mod_get_index(keymap,
                                                 "Meta")),
  internalCapsLockIndex(xkb_keymap_mod_get_index(keymap,
                                                 XKB_LED_NAME_CAPS)),
  internalNumLockIndex(xkb_keymap_mod_get_index(keymap,
                                                XKB_LED_NAME_NUM)),
  internalModeIndex(xkb_keymap_mod_get_index(keymap,
                                             XKB_LED_NAME_SCROLL))
{
}

xbmc::XKBKeymap::~XKBKeymap()
{
  xkb_state_unref(state);
  xkb_map_unref(keymap);
}

uint32_t
xbmc::XKBKeymap::KeysymForKeycode(uint32_t code) const
{
  const xkb_keysym_t *syms;
  uint32_t numSyms;

  numSyms = xkb_key_get_syms(state, code + 8, &syms);

  if (numSyms == 1)
    return static_cast<uint32_t>(syms[0]);

  std::stringstream ss;
  ss << "Pressed key "
     << std::hex
     << code
     << std::dec
     << " is unspported";

  throw std::runtime_error(ss.str());
}

uint32_t
xbmc::XKBKeymap::CurrentModifiers()
{
  XBMCMod xbmcModifiers = XBMCKMOD_NONE;
  enum xkb_state_component components =
    static_cast <xkb_state_component>(XKB_STATE_DEPRESSED |
                                      XKB_STATE_LATCHED |
                                      XKB_STATE_LOCKED);
  xkb_mod_mask_t mask = xkb_state_serialize_mods(state,
                                                 components);
  struct ModTable
  {
    xkb_mod_index_t xkbMod;
    XBMCMod xbmcMod;
  } modTable[] =
  {
    { internalLeftShiftIndex, XBMCKMOD_LSHIFT },
    { internalRightShiftIndex, XBMCKMOD_RSHIFT },
    { internalLeftShiftIndex, XBMCKMOD_LSUPER },
    { internalRightSuperIndex, XBMCKMOD_RSUPER },
    { internalLeftControlIndex, XBMCKMOD_LCTRL },
    { internalRightControlIndex, XBMCKMOD_RCTRL },
    { internalLeftAltIndex, XBMCKMOD_LALT },
    { internalRightAltIndex, XBMCKMOD_RALT },
    { internalLeftMetaIndex, XBMCKMOD_LMETA },
    { internalRightMetaIndex, XBMCKMOD_RMETA },
    { internalNumLockIndex, XBMCKMOD_NUM },
    { internalCapsLockIndex, XBMCKMOD_CAPS },
    { internalModeIndex, XBMCKMOD_MODE }
  };

  size_t modTableSize = sizeof(modTable) / sizeof(modTable[0]);

  for (size_t i = 0; i < modTableSize; ++i)
  {
    if (mask & (1 << modTable[i].xkbMod))
      xbmcModifiers = static_cast <XBMCMod> (xbmcModifiers |
                                             modTable[i].xbmcMod);
  }

  return static_cast<uint32_t>(xbmcModifiers);
}

void
xbmc::XKBKeymap::UpdateMask(uint32_t depressed,
                            uint32_t latched,
                            uint32_t locked,
                            uint32_t group)
{
  xkb_state_update_mask(state, depressed, latched, locked, 0, 0, group);
}

xbmc::KeyboardProcessor::KeyboardProcessor(EventListener &listener) :
  listener(listener),
  xbmcWindow(NULL)
{
}

void
xbmc::KeyboardProcessor::SetXBMCSurface(struct wl_surface *s)
{
  xbmcWindow = s;
}

void
xbmc::KeyboardProcessor::UpdateKeymap(xw::Keymap *newKeymap)
{
  keymap.reset(newKeymap);
}

void
xbmc::KeyboardProcessor::Enter(uint32_t serial,
                               struct wl_surface *surface,
                               struct wl_array *keys)
{
  if (surface == xbmcWindow)
  {
    listener.OnFocused();
  }
}

void
xbmc::KeyboardProcessor::Leave(uint32_t serial,
                               struct wl_surface *surface)
{
  if (surface == xbmcWindow)
  {
    listener.OnUnfocused();
  }
}

void
xbmc::KeyboardProcessor::Key(uint32_t serial,
                             uint32_t time,
                             uint32_t key,
                             enum wl_keyboard_key_state state)
{
  if (!keymap.get())
    throw std::logic_error("Keymap must be set before processing key events");

  uint32_t sym = XKB_KEY_NoSymbol;

  try
  {
    sym = keymap->KeysymForKeycode(key);
  }
  catch (const std::runtime_error &err)
  {
    printf ("xbmc::KeyboardProcessor: %s\n", err.what());
    return;
  }
  
  /* Strip high bits from functional keys */
  if ((sym & ~(0xff00)) < 0x1b)
    sym = sym & ~(0xff00);
  else if ((sym & ~(0xff00)) == 0xff)
    sym = static_cast<uint32_t>(XBMCK_DELETE);
  
  const bool isNavigationKey = (sym >= 0xff50 && sym <= 0xff58);
  const bool isModifierKey = (sym >= 0xffe1 && sym <= 0xffee);
  const bool isKeyPadKey = (sym >= 0xffbd && sym <= 0xffb9);
  const bool isFKey = (sym >= 0xffbe && sym <= 0xffcc);
  const bool isMediaKey = (sym >= 0x1008ff26 && sym <= 0x1008ffa2);

  if (isNavigationKey ||
      isModifierKey ||
      isKeyPadKey ||
      isFKey ||
      isMediaKey)
  {
    /* Navigation keys are not in line, so we need to
     * look them up */
    struct NavigationKeySyms
    {
      uint32_t xkb;
      XBMCKey xbmc;
    } navigationKeySyms[] =
    {
      { XKB_KEY_Home, XBMCK_HOME },
      { XKB_KEY_Left, XBMCK_LEFT },
      { XKB_KEY_Right, XBMCK_RIGHT },
      { XKB_KEY_Down, XBMCK_DOWN },
      { XKB_KEY_Up, XBMCK_UP },
      { XKB_KEY_Page_Up, XBMCK_PAGEUP },
      { XKB_KEY_Page_Down, XBMCK_PAGEDOWN },
      { XKB_KEY_End, XBMCK_END },
      { XKB_KEY_Insert, XBMCK_INSERT },
      { XKB_KEY_KP_0, XBMCK_KP0 },
      { XKB_KEY_KP_1, XBMCK_KP1 },
      { XKB_KEY_KP_2, XBMCK_KP2 },
      { XKB_KEY_KP_3, XBMCK_KP3 },
      { XKB_KEY_KP_4, XBMCK_KP4 },
      { XKB_KEY_KP_5, XBMCK_KP5 },
      { XKB_KEY_KP_6, XBMCK_KP6 },
      { XKB_KEY_KP_7, XBMCK_KP7 },
      { XKB_KEY_KP_8, XBMCK_KP8 },
      { XKB_KEY_KP_9, XBMCK_KP9 },
      { XKB_KEY_KP_Decimal, XBMCK_KP_PERIOD },
      { XKB_KEY_KP_Divide, XBMCK_KP_DIVIDE },
      { XKB_KEY_KP_Multiply, XBMCK_KP_MULTIPLY },
      { XKB_KEY_KP_Add, XBMCK_KP_PLUS },
      { XKB_KEY_KP_Separator, XBMCK_KP_MINUS },
      { XKB_KEY_KP_Equal, XBMCK_KP_EQUALS },
      { XKB_KEY_F1, XBMCK_F1 },
      { XKB_KEY_F2, XBMCK_F2 },
      { XKB_KEY_F3, XBMCK_F3 },
      { XKB_KEY_F4, XBMCK_F4 },
      { XKB_KEY_F5, XBMCK_F5 },
      { XKB_KEY_F6, XBMCK_F6 },
      { XKB_KEY_F7, XBMCK_F7 },
      { XKB_KEY_F8, XBMCK_F8 },
      { XKB_KEY_F9, XBMCK_F9 },
      { XKB_KEY_F10, XBMCK_F10 },
      { XKB_KEY_F11, XBMCK_F11 },
      { XKB_KEY_F12, XBMCK_F12 },
      { XKB_KEY_F13, XBMCK_F13 },
      { XKB_KEY_F14, XBMCK_F14 },
      { XKB_KEY_F15, XBMCK_F15 },
      { XKB_KEY_Caps_Lock, XBMCK_CAPSLOCK },
      { XKB_KEY_Shift_Lock, XBMCK_SCROLLOCK },
      { XKB_KEY_Shift_R, XBMCK_RSHIFT },
      { XKB_KEY_Shift_L, XBMCK_LSHIFT },
      { XKB_KEY_Alt_R, XBMCK_RALT },
      { XKB_KEY_Alt_L, XBMCK_LALT },
      { XKB_KEY_Control_R, XBMCK_RCTRL },
      { XKB_KEY_Control_L, XBMCK_LCTRL },
      { XKB_KEY_Meta_R, XBMCK_RMETA },
      { XKB_KEY_Meta_L, XBMCK_LMETA },
      { XKB_KEY_Super_R, XBMCK_RSUPER },
      { XKB_KEY_Super_L, XBMCK_LSUPER },
      { XKB_KEY_XF86Eject, XBMCK_EJECT },
      { XKB_KEY_XF86AudioStop, XBMCK_STOP },
      { XKB_KEY_XF86AudioRecord, XBMCK_RECORD },
      { XKB_KEY_XF86AudioRewind, XBMCK_REWIND },
      { XKB_KEY_XF86AudioPlay, XBMCK_PLAY },
      { XKB_KEY_XF86AudioRandomPlay, XBMCK_SHUFFLE },
      { XKB_KEY_XF86AudioForward, XBMCK_FASTFORWARD }
    };
  
    size_t navigationKeySymsSize = sizeof(navigationKeySyms) /
                                   sizeof(navigationKeySyms[0]);
                                   
    for (size_t i = 0; i < navigationKeySymsSize; ++i)
    {
      if (navigationKeySyms[i].xkb == sym)
      {
        sym = navigationKeySyms[i].xbmc;
        break;
      }
    }
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
      throw std::runtime_error("Unrecognized key state");
  }

  XBMC_Event event;
  event.type = keyEventType;
  event.key.keysym.scancode = key;
  event.key.keysym.sym = static_cast<XBMCKey>(sym);
  event.key.keysym.unicode = static_cast<XBMCKey>(sym);
  event.key.keysym.mod = static_cast<XBMCMod>(keymap->CurrentModifiers());
  event.key.state = 0;
  event.key.type = event.type;
  event.key.which = '0';

  listener.OnEvent(event);
}

void
xbmc::KeyboardProcessor::Modifier(uint32_t serial,
                                  uint32_t depressed,
                                  uint32_t latched,
                                  uint32_t locked,
                                  uint32_t group)
{
  keymap->UpdateMask(depressed, latched, locked, group);
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

WaylandInput::WaylandInput (struct wl_seat *seat,
                            xbmc::EventDispatch &dispatch) :
  pointerProcessor (dispatch),
  keyboardProcessor (dispatch),
  seat (new xw::Seat (seat, *this))
{
}

void WaylandInput::SetXBMCSurface(struct wl_surface *s)
{
  keyboardProcessor.SetXBMCSurface(s);
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
  if (keyboard.get())
    return false;

  enum xkb_context_flags flags =
    static_cast <enum xkb_context_flags> (0);

  struct xkb_context *context = xkb_context_new(flags);

  if (!context)
    throw std::runtime_error("Failed to create xkb context");

  keyboard.reset(new xw::Keyboard(k,
                                  context,
                                  keyboardProcessor));
  return true;
}

void WaylandInput::RemovePointer()
{
  pointer.reset ();
}

void WaylandInput::RemoveKeyboard()
{
  keyboard.reset();
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

void CWinEventsWayland::SetXBMCSurface(struct wl_surface *s)
{
  if (!inputInstance.get())
    throw std::logic_error("Must have a wl_seat set before setting "
                           "the wl_surface in CWinEventsWayland");
  
  inputInstance->SetXBMCSurface(s);
}

#endif
