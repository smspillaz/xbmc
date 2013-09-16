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

#if defined(HAVE_MIR)

#include <deque>
#include <stdexcept>
#include <pthread.h>
#include <mir_toolkit/mir_client_library.h>

#include "windowing/DllMirClient.h"
#include "Connection.h"
#include "Surface.h"

namespace
{
class MutexLock :
  boost::noncopyable
{
public:

  MutexLock(pthread_mutex_t *mutex) :
    m_mutex(mutex)
  {
    pthread_mutex_lock(m_mutex);
  }
  
  ~MutexLock()
  {
    pthread_mutex_unlock(m_mutex);
  }
  
private:
  
  pthread_mutex_t *m_mutex;
};
}

namespace xm = xbmc::mir;

xm::Surface::Surface(IDllMirClient &clientLibrary,
                     MirSurface *surface,
                     ISurfaceEventProcessor &eventProcessor) :
  m_clientLibrary(clientLibrary),
  m_surface(surface),
  m_eventProcessor(eventProcessor),
  m_eglWindow(NULL)
{
  if (!m_surface)
    throw std::runtime_error("Returned surface was not valid");

  pthread_mutexattr_t attributes;

  pthread_mutexattr_init(&attributes);
  pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);

  pthread_mutex_init(&m_mutex, &attributes);
  
  MirEventDelegate delegate =
  {
    xm::Surface::HandleEventFromThreadCallback,
    reinterpret_cast<void *>(this)
  };
  
  m_clientLibrary.mir_surface_set_event_handler(m_surface,
                                                &delegate);
}

void
xm::Surface::HandleEvent(const MirEvent &event)
{
  switch (event.type)
  {
    case mir_event_type_key:
      m_eventProcessor.KeyboardEvent(reinterpret_cast<const MirKeyEvent &>(event));
      break;
    case mir_event_type_motion:
      m_eventProcessor.MotionEvent(reinterpret_cast<const MirMotionEvent &>(event));
      break;
    case mir_event_type_surface:
      m_eventProcessor.SurfaceEvent(reinterpret_cast<const MirSurfaceEvent &>(event));
      break;
    default:
      break;
  }
}

void
xm::Surface::Pump()
{
  MutexLock lock(&m_mutex);
  while(!m_events.empty())
  {
    /* Create a copy so that any recursive call to Pump()
     * does not call pop_front on the deque twice */
    MirEvent event(m_events.front());
    m_events.pop_front();
    HandleEvent(event);
  }
}

void
xm::Surface::HandleEventFromThreadCallback(MirSurface *surface,
                                           const MirEvent *event,
                                           void *context)
{
  reinterpret_cast<Surface *>(context)->HandleEventFromThread(event);
}

void
xm::Surface::HandleEventFromThread(const MirEvent *event)
{
  MutexLock lock(&m_mutex);
  m_events.push_back(*event);
}

xm::Surface::~Surface()
{
  pthread_mutex_destroy(&m_mutex);
  m_clientLibrary.mir_surface_release_sync(m_surface);
}

MirEGLNativeWindowType *
xm::Surface::GetEGLWindow()
{
  if (!m_eglWindow)
  {
    m_eglWindow =
      m_clientLibrary.mir_surface_get_egl_native_window(m_surface);
  }
  
  return &m_eglWindow;
}

MirWaitHandle *
xm::Surface::SetState(MirSurfaceState state)
{
  return m_clientLibrary.mir_surface_set_state(m_surface, state);
}

#endif
