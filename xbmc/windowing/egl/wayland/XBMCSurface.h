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
#include <boost/scoped_ptr.hpp>

class IDllWaylandClient;
class IDllWaylandEGL;

struct wl_region;

typedef struct wl_egl_window * EGLNativeWindowType;

namespace xbmc
{
namespace wayland
{
class Callback;
class Compositor;
class OpenGLSurface;
class Shell;
class ShellSurface;
class Surface;

class XBMCSurface
{
public:

  XBMCSurface(IDllWaylandClient &clientLibrary,
              IDllWaylandEGL &eglLibrary,
              const boost::scoped_ptr<Compositor> &compositor,
              const boost::scoped_ptr<Shell> &shell,
              uint32_t width,
              uint32_t height);
  ~XBMCSurface();

  void Show();
  void Resize(uint32_t width, uint32_t height);
  EGLNativeWindowType * EGLNativeWindow() const;

private:

  class Private;
  boost::scoped_ptr<Private> priv;
};
}
}

#endif
