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

#if defined(HAVE_MIR)

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <mir_toolkit/mir_client_library.h>

class IDllMirClient;
class IDllXKBCommon;

typedef struct MirMesaEGLNativeDisplay MirMesaEGLNativeDisplay;
typedef struct MirMesaEGLNativeSurface MirMesaEGLNativeSurface;
typedef MirEGLNativeDisplayType EGLNativeDisplayType;
typedef MirEGLNativeWindowType  EGLNativeWindowType;

namespace xbmc
{
namespace mir
{
class Connection;
class IEventPipe;

class XBMCSurface
{
public:

  struct EventInjector
  {
    typedef void (*RegisterEventPipe)(IDllMirClient &,
                                      IDllXKBCommon &,
                                      xbmc::mir::IEventPipe &);
    typedef void (*DestroyEventPipe)();
    
    typedef void (*HandleKeyboardEvent)(const MirKeyEvent &);
    typedef void (*HandleMotionEvent)(const MirMotionEvent &);
    typedef void (*HandleSurfaceEvent)(const MirSurfaceEvent &);
    
    RegisterEventPipe registerEventPipe;
    DestroyEventPipe destroyEventPipe;
    
    HandleKeyboardEvent handleKeyboardEvent;
    HandleMotionEvent handleMotionEvent;
    HandleSurfaceEvent handleSurfaceEvent;
  };

  XBMCSurface(IDllMirClient &clientLibrary,
              IDllXKBCommon &xkbCommonLibrary,
              const EventInjector &eventInjector,
              const boost::scoped_ptr<Connection> &connection);
  ~XBMCSurface();

  EGLNativeWindowType * NativeWindow() const;

private:

  class Private;
  boost::scoped_ptr<Private> priv;
};
}
}

#endif
