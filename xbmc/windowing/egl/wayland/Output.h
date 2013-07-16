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

#include <vector>

#include <boost/noncopyable.hpp>

#include <wayland-client.h>

class IDllWaylandClient;

namespace xbmc
{
namespace wayland
{
struct Output :
  boost::noncopyable
{
public:

  Output(IDllWaylandClient &,
         struct wl_output *);
  ~Output();

  struct ModeGeometry
  {
    int32_t width;
    int32_t height;
    int32_t refresh;
  };

  struct PhysicalGeometry
  {
    int32_t x;
    int32_t y;
    int32_t physicalWidth;
    int32_t physicalHeight;
    enum wl_output_subpixel subpixelArrangement;
    enum wl_output_transform outputTransformation;
  };

  struct wl_output * GetWlOutput();

  const ModeGeometry & CurrentMode();
  const ModeGeometry & PreferredMode();

  const std::vector <ModeGeometry> & AllModes();

  const PhysicalGeometry & Geometry();
  uint32_t ScaleFactor();

  static void GeometryCallback(void *,
                               struct wl_output *,
                               int32_t,
                               int32_t,
                               int32_t,
                               int32_t,
                               int32_t,
                               const char *,
                               const char *,
                               int32_t);
  static void ModeCallback(void *,
                           struct wl_output *,
                           uint32_t,
                           int32_t,
                           int32_t,
                           int32_t);
  static void ScaleCallback(void *,
                            struct wl_output *,
                            int32_t);
  static void DoneCallback(void *,
                           struct wl_output *);

private:

  static const wl_output_listener m_listener;

  void Geometry(int32_t x,
                int32_t y,
                int32_t physicalWidth,
                int32_t physicalHeight,
                int32_t subpixel,
                const char *make,
                const char *model,
                int32_t transform);
  void Mode(uint32_t flags,
            int32_t width,
            int32_t height,
            int32_t refresh);
  void Scale(int32_t);
  void Done();

  IDllWaylandClient &m_clientLibrary;

  struct wl_output *m_output;

  PhysicalGeometry m_geometry;
  std::vector<ModeGeometry> m_modes;

  uint32_t m_scaleFactor;

  ModeGeometry *m_current;
  ModeGeometry *m_preferred;
};
}
}

#endif
