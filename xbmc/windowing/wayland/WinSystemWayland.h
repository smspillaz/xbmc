#ifndef WINDOW_SYSTEM_WAYLAND_H
#define WINDOW_SYSTEM_WAYLAND_H

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
#include "windowing/WinSystem.h"
#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GlobalsHandling.h"

class CWinSystemWayland : public CWinSystemBase, public CRenderSystemGLES
{
public:
  CWinSystemWayland();
  virtual ~CWinSystemWayland();

  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool DestroyWindow();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void UpdateResolutions();

  virtual void ShowOSMouse(bool show);

  virtual void NotifyAppActiveChange(bool bActivated);

  virtual bool Minimize();
  virtual bool Restore() ;
  virtual bool Hide();
  virtual bool Show(bool raise = true);

  virtual bool IsExtSupported(const char* extension);

  virtual bool makeOMXCurrent();

  EGLContext GetEGLContext() const;
  EGLDisplay GetEGLDisplay() const;
protected:
  bool RefreshEGLContext();

  SDL_Window* m_SDLWindow;
  SDL_Surface* m_SDLSurface;
  SDL_GLContext m_SDLGLContext;
  EGLDisplay   m_eglDisplay;
  EGLContext   m_eglContext;
  EGLContext   m_eglOMXContext;
  EGLSurface   m_eglSurface;
  struct wl_surface       *m_Surface;
  struct wl_shell_surface *m_ShellSurface;
  struct wl_egl_window    *m_WlEglWindow;
  struct wl_display       *m_Dpy;
  struct wl_registry      *m_Registry;
  struct wl_compositor    *m_Compositor;
  struct wl_output        *m_Output;
  struct wl_shell         *m_Shell;

  struct xkb_context      *m_XkbContext;

  bool         m_bWasFullScreenBeforeMinimize;

  virtual bool PresentRenderImpl(const CDirtyRegionList &dirty);
  virtual void SetVSyncImpl(bool enable);
  
  CStdString m_eglext;

  int m_iVSyncErrors;
};

XBMC_GLOBAL_REF(CWinSystemWayland,g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(CWinSystemWayland)

#endif // WINDOW_SYSTEM_H
