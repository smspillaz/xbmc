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

#if defined (HAVE_MIR)

#include <deque>
#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <mir_toolkit/mir_client_library.h>

#include "windowing/mir/EventPipe.h"
#include "windowing/mir/SurfaceEventProcessor.h"

class IDllMirClient;

typedef void * MirEGLNativeWindowType;

namespace xbmc
{
namespace mir
{
class Surface :
  public IEventPipe,
  boost::noncopyable
{
public:

  Surface(IDllMirClient &clientLibrary,
          MirSurface *surface,
          ISurfaceEventProcessor &eventProcessor);
  ~Surface();
  
  MirSurface * GetMirSurface();
  MirEGLNativeWindowType * GetEGLWindow();
  
  MirWaitHandle * SetState(MirSurfaceState);

private:

  /* Called from event handler threads */
  static void HandleEventFromThreadCallback(MirSurface *,
                                            MirEvent const *,
                                            void *);
  void HandleEventFromThread(MirEvent const *);
  
  /* Reads the message pipe */
  void Pump();
  void HandleEvent(const MirEvent &event);
  
  IDllMirClient &m_clientLibrary;

  MirSurface *m_surface;
  ISurfaceEventProcessor &m_eventProcessor;

  MirEGLNativeWindowType m_eglWindow;

  pthread_mutex_t m_mutex;
  std::deque<MirEvent> m_events;
};
}
}

#endif
