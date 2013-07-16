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

#include <sstream>
#include <iostream>
#include <stdexcept>

#include <wayland-client.h>
#include <wayland-version.h>

#include "windowing/DllWaylandClient.h"
#include "windowing/WaylandProtocol.h"
#include "Output.h"

#define WAYLAND_VERSION_GREATER_OR_EQUAL(maj, min, micro) \
  WAYLAND_MAJOR_VERSION >= maj || \
  (WAYLAND_MAJOR_VERSION == maj && WAYLAND_MINOR_VERSION >= min) || \
  (WAYLAND_MAJOR_VERSION == maj && WAYLAND_MINOR_VERSION == min && WAYLAND_MICRO_VERSION >= micro) 

namespace xw = xbmc::wayland;

const wl_output_listener xw::Output::m_listener = 
{
  Output::GeometryCallback,
#if WAYLAND_VERSION_GREATER_OR_EQUAL(1, 1, 90)
  Output::ModeCallback,
  Output::DoneCallback,
  Output::ScaleCallback
#else
  Output::ModeCallback
#endif
};

xw::Output::Output(IDllWaylandClient &clientLibrary,
                   struct wl_output *output) :
  m_clientLibrary(clientLibrary),
  m_output(output),
  m_scaleFactor(1.0),
  m_current(NULL),
  m_preferred(NULL)
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_output,
                                       &m_listener,
                                       reinterpret_cast<void *>(this));
}

xw::Output::~Output()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_output);
}

struct wl_output *
xw::Output::GetWlOutput()
{
  return m_output;
}

const xw::Output::ModeGeometry &
xw::Output::CurrentMode()
{
  if (!m_current)
    throw std::logic_error("No current mode has been set by the server"
                           " yet");
  
  return *m_current;
}

const xw::Output::ModeGeometry &
xw::Output::PreferredMode()
{
  if (!m_preferred)
    throw std::logic_error("No preferred mode has been set by the "
                           " server yet");

  return *m_preferred;
}

const std::vector <xw::Output::ModeGeometry> &
xw::Output::AllModes()
{
  return m_modes;
}

const xw::Output::PhysicalGeometry &
xw::Output::Geometry()
{
  return m_geometry;
}

uint32_t
xw::Output::ScaleFactor()
{
  return m_scaleFactor;
}

void
xw::Output::GeometryCallback(void *data,
                             struct wl_output *output,
                             int32_t x,
                             int32_t y,
                             int32_t physicalWidth,
                             int32_t physicalHeight,
                             int32_t subpixelArrangement,
                             const char *make,
                             const char *model,
                             int32_t transform)
{
  return static_cast<xw::Output *>(data)->Geometry(x,
                                                   y,
                                                   physicalWidth,
                                                   physicalHeight,
                                                   subpixelArrangement,
                                                   make,
                                                   model,
                                                   transform);
}

void
xw::Output::ModeCallback(void *data,
                         struct wl_output *output,
                         uint32_t flags,
                         int32_t width,
                         int32_t height,
                         int32_t refresh)
{
  return static_cast<xw::Output *>(data)->Mode(flags,
                                               width,
                                               height,
                                               refresh);
}

void
xw::Output::DoneCallback(void *data,
                         struct wl_output *output)
{
  return static_cast<xw::Output *>(data)->Done();
}

void
xw::Output::ScaleCallback(void *data,
                          struct wl_output *output,
                          int32_t factor)
{
  return static_cast<xw::Output *>(data)->Scale(factor);
}

void
xw::Output::Geometry(int32_t x,
                     int32_t y,
                     int32_t physicalWidth,
                     int32_t physicalHeight,
                     int32_t subpixelArrangement,
                     const char *make,
                     const char *model,
                     int32_t transform)
{
  m_geometry.x = x;
  m_geometry.y = y;
  m_geometry.physicalWidth = physicalWidth;
  m_geometry.physicalHeight = physicalHeight;
  m_geometry.subpixelArrangement =
    static_cast<enum wl_output_subpixel>(subpixelArrangement);
  m_geometry.outputTransformation =
    static_cast<enum wl_output_transform>(transform);
}

void
xw::Output::Mode(uint32_t flags,
                 int32_t width,
                 int32_t height,
                 int32_t refresh)
{
  xw::Output::ModeGeometry *update = NULL;
  
  for (std::vector<ModeGeometry>::iterator it = m_modes.begin();
       it != m_modes.end();
       ++it)
  { 
    if (it->width == width &&
        it->height == height &&
        it->refresh == refresh)
    {
      update = &(*it);
      break;
    }
  }
  
  enum wl_output_mode outputFlags =
    static_cast<enum wl_output_mode>(flags);
  
  if (!update)
  {
    /* New output created */
    m_modes.push_back(ModeGeometry());
    ModeGeometry &next(m_modes.back());
    
    next.width = width;
    next.height = height;
    next.refresh = refresh;
    
    update = &next;
  }
  
  /* We may get a mode notification for a new or
   * or existing mode. In both cases we need to
   * update the current and preferred modes */
  if (outputFlags & WL_OUTPUT_MODE_CURRENT)
    m_current = update;
  if (outputFlags & WL_OUTPUT_MODE_PREFERRED)
    m_preferred = update;
}

void
xw::Output::Done()
{
}

void
xw::Output::Scale(int32_t factor)
{
  m_scaleFactor = factor;
}

#endif
