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

#if defined(HAVE_WAYLAND) && defined(HAS_GLES)

#include "WinSystemWayland.h"
#include "utils/log.h"
#include <SDL2/SDL_syswm.h>
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "guilib/Texture.h"
#include <vector>

using namespace std;

// Comment out one of the following defines to select the colourspace to use
//#define RGBA8888
#define RGB565

#if defined(RGBA8888)
#define RSIZE	8
#define GSIZE	8
#define BSIZE	8
#define ASIZE	8
#define DEPTH	8
#define BPP		32
#elif defined(RGB565)
#define RSIZE	5
#define GSIZE	6
#define BSIZE	5
#define ASIZE	0
#define DEPTH	16
#define BPP		16
#endif

CWinSystemWayland::CWinSystemWayland() : CWinSystemBase()
, m_eglOMXContext(0)
{
  m_eWindowSystem = WINDOW_SYSTEM_WAYLAND;
  m_bWasFullScreenBeforeMinimize = false;

  m_SDLSurface = NULL;
  m_SDLWindow = NULL;
  m_SDLGLContext = NULL;
  m_eglDisplay = NULL;
  m_eglContext = NULL;

  m_Surface = NULL;
  m_ShellSurface = NULL;
  m_WlEglWindow = NULL;
  m_Dpy = NULL;
  m_Registry = NULL;
  m_Compositor = NULL;
  m_Output = NULL;
  m_Shell = NULL;
  
  m_iVSyncErrors = 0;
}

CWinSystemWayland::~CWinSystemWayland()
{
}

namespace
{
bool InitWaylandVideoSystem ()
{
  if (SDL_VideoInit ("wayland") < 0)
  {
    CLog::Log(LOGERROR, "EGL Error: Could not initialize Wayland Video");
    return false;
  }

  return true;
}
}

bool CWinSystemWayland::InitWindowSystem()
{
  if (strcmp (SDL_GetCurrentVideoDriver (), "wayland"))
  {
    if (!InitWaylandVideoSystem ())
      return false;
  }

  if (!InitWaylandVideoSystem () ||
      SDL_GL_LoadLibrary (NULL))
  {
    CLog::Log(LOGERROR, "EGL Error: Could not load SDL OpenGL|ES");
    return false;
  }

  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemWayland::DestroyWindowSystem()
{
  SDL_GL_UnloadLibrary ();
  SDL_VideoQuit ();

  return true;
}

bool CWinSystemWayland::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  m_SDLWindow = SDL_CreateWindow ("XBMC Media Center", 0, 0, res.iWidth, res.iHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  m_SDLGLContext = SDL_GL_CreateContext (m_SDLWindow);
  SDL_GL_MakeCurrent (m_SDLWindow, m_SDLGLContext);
  CTexture iconTexture;
  iconTexture.LoadFromFile("special://xbmc/media/icon.png");

  //SDL_WM_SetIcon(SDL_CreateRGBSurfaceFrom(iconTexture.GetPixels(), iconTexture.GetWidth(), iconTexture.GetHeight(), BPP, iconTexture.GetPitch(), 0xff0000, 0x00ff00, 0x0000ff, 0xff000000L), NULL);
  //SDL_WM_SetCaption("XBMC Media Center", NULL);

  m_bWindowCreated = true;

  return true;
}

bool CWinSystemWayland::DestroyWindow()
{
  SDL_DestroyWindow (m_SDLWindow);
  m_SDLWindow = NULL;

  return true;
}

bool CWinSystemWayland::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  if (m_nWidth != newWidth || m_nHeight != newHeight)
  {
    m_nWidth  = newWidth;
    m_nHeight = newHeight;

    SDL_SetWindowSize (m_SDLWindow, m_nWidth, m_nHeight);
    SDL_SetWindowFullscreen (m_SDLWindow, m_bFullScreen ? SDL_TRUE : SDL_FALSE);

    m_SDLSurface = SDL_GetWindowSurface (m_SDLWindow);

    if (m_SDLSurface)
      RefreshEGLContext();
  }

  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight, false, 0);

  return true;
}

bool CWinSystemWayland::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  m_nWidth      = res.iWidth;
  m_nHeight     = res.iHeight;
  m_bFullScreen = fullScreen;

  if (SDL_SetWindowFullscreen (m_SDLWindow, m_bFullScreen ? SDL_TRUE : SDL_FALSE))
    RefreshEGLContext();

  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight, fullScreen, res.fRefreshRate);
  
  return true;
}

void CWinSystemWayland::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  if (SDL_GetNumVideoDisplays())
  {
    SDL_Rect rect;
    SDL_GetDisplayBounds (0, &rect);

    UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, rect.w, rect.h, 0.0);
  }
  else
    CLog::Log(LOGERROR, "No displays attached!");
}

bool CWinSystemWayland::IsExtSupported(const char* extension)
{
  if(strncmp(extension, "EGL_", 4) != 0)
    return CRenderSystemGLES::IsExtSupported(extension);

  CStdString name;

  name  = " ";
  name += extension;
  name += " ";

  return m_eglext.find(name) != string::npos;
}

bool CWinSystemWayland::RefreshEGLContext()
{
  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);
  if (SDL_GetWindowWMInfo(m_SDLWindow, &info) <= 0)
  {
    CLog::Log(LOGERROR, "Failed to get window manager info from SDL");
    return false;
  }

  m_eglContext = info.info.wl.eglContext;
  m_eglDisplay = info.info.wl.eglDisplay;

  const char *ext_c = eglQueryString (m_eglDisplay, EGL_EXTENSIONS);

  if (ext_c)
    m_eglext = string (ext_c);

  return true;
}

bool CWinSystemWayland::PresentRenderImpl(const CDirtyRegionList &dirty)
{
  SDL_GL_SwapWindow (m_SDLWindow);

  return true;
}

void CWinSystemWayland::SetVSyncImpl(bool enable)
{
  if (SDL_GL_SetSwapInterval (enable ? 1 : 0) == -1)
  {
    CLog::Log(LOGERROR, "EGL Error: Could not set vsync");
  }
}

void CWinSystemWayland::ShowOSMouse(bool show)
{
  SDL_ShowCursor(show ? 1 : 0);
  // On BB have show the cursor, otherwise it hangs! (FIXME verify it if fixed)
  //SDL_ShowCursor(1);
}

void CWinSystemWayland::NotifyAppActiveChange(bool bActivated)
{
  if (bActivated && m_bWasFullScreenBeforeMinimize && !g_graphicsContext.IsFullScreenRoot())
    g_graphicsContext.ToggleFullScreenRoot();
}

bool CWinSystemWayland::Minimize()
{
  m_bWasFullScreenBeforeMinimize = g_graphicsContext.IsFullScreenRoot();
  if (m_bWasFullScreenBeforeMinimize)
    g_graphicsContext.ToggleFullScreenRoot();

  SDL_MinimizeWindow(m_SDLWindow);
  return true;
}

bool CWinSystemWayland::Restore()
{
  return false;
}

bool CWinSystemWayland::Hide()
{
  SDL_HideWindow (m_SDLWindow);
  return true;
}

bool CWinSystemWayland::Show(bool raise)
{
  SDL_ShowWindow (m_SDLWindow);
  return true;
}

EGLContext CWinSystemWayland::GetEGLContext() const
{
  return m_eglContext;
}

EGLDisplay CWinSystemWayland::GetEGLDisplay() const
{
  return m_eglDisplay;
}

bool CWinSystemWayland::makeOMXCurrent()
{
  return true;
}


#endif
