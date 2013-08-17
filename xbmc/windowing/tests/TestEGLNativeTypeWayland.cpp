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
#define WL_EGL_PLATFORM

#include "system.h"

#if defined(HAVE_WAYLAND)

#include <sstream>
#include <stdexcept>

#include <iostream>

#include <boost/bind.hpp>

#include <gtest/gtest.h>

#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include "wayland/xbmc_wayland_test_client_protocol.h"

#include "windowing/egl/wayland/Callback.h"
#include "windowing/egl/wayland/Display.h"
#include "windowing/egl/wayland/Registry.h"
#include "windowing/egl/wayland/Surface.h"
#include "windowing/egl/EGLNativeTypeWayland.h"
#include "windowing/WinEvents.h"

#include "windowing/DllWaylandClient.h"

#include "test/TestUtils.h"

namespace
{
static const int DefaultProcessWaitTimeout = 3000; // 3000ms
}

namespace xbmc
{
namespace test
{
namespace wayland
{
class XBMCWayland :
  boost::noncopyable
{
public:

  XBMCWayland(struct xbmc_wayland *xbmcWayland,
              xbmc::wayland::Display &display);
  ~XBMCWayland();

  struct wl_surface * MostRecentSurface();

  void AddMode(int width,
               int height,
               uint32_t refresh,
               enum wl_output_mode mode);
  void MovePointerTo(struct wl_surface *surface,
                     wl_fixed_t x,
                     wl_fixed_t y);
  void SendKeyToKeyboard(struct wl_surface *surface,
                         uint32_t key,
                         enum wl_keyboard_key_state state);
  void SendModifiersToKeyboard(struct wl_surface *surface,
                               uint32_t depressed,
                               uint32_t latched,
                               uint32_t locked,
                               uint32_t group);
  void GiveSurfaceKeyboardFocus(struct wl_surface *surface);
  void PingSurface (struct wl_surface *surface,
                    uint32_t serial);

private:

  static const struct xbmc_wayland_listener m_listener;

  static void MostRecentSurfaceResultCallback(void *,
                                              struct xbmc_wayland *,
                                              struct wl_surface *);

  void MostRecentSurfaceResult(struct wl_surface *surface);

  void SynchronizeCallback();
  void WaitForSynchronize();
  
  DllWaylandClient m_clientLibrary;

  struct xbmc_wayland *m_xbmcWayland;
  struct wl_surface *m_mostRecentSurface;
  
  boost::scoped_ptr<xbmc::wayland::Callback> m_syncCallback;
  bool m_synchronized;
  
  xbmc::wayland::Display &m_display;
};

const struct xbmc_wayland_listener XBMCWayland::m_listener =
{
  XBMCWayland::MostRecentSurfaceResultCallback
};
}

class Process
{
public:

  Process(const CStdString &,
          const CStdString &,
          const CStdString &);
  ~Process();

  void WaitForSignal(int signal, int timeout);
  void WaitForStatus(int status, int timeout);

  void Interrupt();
  void Terminate();
  void Kill();
  
  pid_t Pid();

private:
  
  void SendSignal(int signal);
  
  void Child(const char *program,
             char * const *options);
  void ForkError();
  void Parent();
  
  pid_t m_pid;
};
}
}

namespace xt = xbmc::test;
namespace xw = xbmc::wayland;
namespace xtw = xbmc::test::wayland;

xtw::XBMCWayland::XBMCWayland(struct xbmc_wayland *xbmcWayland,
                              xw::Display &display) :
  m_xbmcWayland(xbmcWayland),
  m_mostRecentSurface(NULL),
  m_display(display)
{
  m_clientLibrary.Load();
  xbmc_wayland_add_listener(m_xbmcWayland,
                            &m_listener,
                            this);
}

xtw::XBMCWayland::~XBMCWayland()
{
  xbmc_wayland_destroy(m_xbmcWayland);
}

void
xtw::XBMCWayland::SynchronizeCallback()
{
  m_synchronized = true;
  m_syncCallback.reset();
}

void
xtw::XBMCWayland::WaitForSynchronize()
{
  m_synchronized = false;
  m_syncCallback.reset(new xw::Callback(m_clientLibrary,
                                        m_display.Sync(),
                                        boost::bind(&XBMCWayland::SynchronizeCallback,
                                                    this)));
  
  while (!m_synchronized)
    CWinEvents::MessagePump();
}

struct wl_surface *
xtw::XBMCWayland::MostRecentSurface()
{
  assert(m_mostRecentSurface);
  return m_mostRecentSurface;
}

void
xtw::XBMCWayland::MostRecentSurfaceResultCallback(void *data,
                                                  struct xbmc_wayland *xbmcWayland,
                                                  struct wl_surface *surface)
{
  static_cast<XBMCWayland *>(data)->MostRecentSurfaceResult(surface);
}

void
xtw::XBMCWayland::MostRecentSurfaceResult(struct wl_surface *surface)
{
  m_mostRecentSurface = surface;
}

void
xtw::XBMCWayland::AddMode(int width,
                          int height,
                          uint32_t refresh,
                          enum wl_output_mode flags)
{
  xbmc_wayland_add_mode(m_xbmcWayland,
                        width,
                        height,
                        refresh,
                        static_cast<uint32_t>(flags));
}

void
xtw::XBMCWayland::MovePointerTo(struct wl_surface *surface,
                                wl_fixed_t x,
                                wl_fixed_t y)
{
  xbmc_wayland_move_pointer_to_on_surface(m_xbmcWayland,
                                          surface,
                                          x,
                                          y);
}

void
xtw::XBMCWayland::SendKeyToKeyboard(struct wl_surface *surface,
                                    uint32_t key,
                                    enum wl_keyboard_key_state state)
{
  xbmc_wayland_send_key_to_keyboard(m_xbmcWayland,
                                    surface,
                                    key,
                                    state);
}

void
xtw::XBMCWayland::SendModifiersToKeyboard(struct wl_surface *surface,
                                          uint32_t depressed,
                                          uint32_t latched,
                                          uint32_t locked,
                                          uint32_t group)
{
  xbmc_wayland_send_modifiers_to_keyboard(m_xbmcWayland,
                                          surface,
                                          depressed,
                                          latched,
                                          locked,
                                          group);
}

void
xtw::XBMCWayland::GiveSurfaceKeyboardFocus(struct wl_surface *surface)
{
  xbmc_wayland_give_surface_keyboard_focus(m_xbmcWayland,
                                           surface);
}

void
xtw::XBMCWayland::PingSurface(struct wl_surface *surface,
                              uint32_t serial)
{
  xbmc_wayland_ping_surface(m_xbmcWayland, surface, serial);
}

namespace
{
class TempFileWrapper :
  boost::noncopyable
{
public:

  TempFileWrapper(const CStdString &suffix);
  ~TempFileWrapper();
  
  void FetchDirectory(CStdString &directory);
  void FetchFilename(CStdString &name);
private:

  XFILE::CFile *m_file;
};

TempFileWrapper::TempFileWrapper(const CStdString &suffix) :
  m_file(CXBMCTestUtils::Instance().CreateTempFile(suffix))
{
}

TempFileWrapper::~TempFileWrapper()
{
  CXBMCTestUtils::Instance().DeleteTempFile(m_file);
}

void TempFileWrapper::FetchDirectory(CStdString &directory)
{
  directory = CXBMCTestUtils::Instance().TempFileDirectory(m_file);
}

void TempFileWrapper::FetchFilename(CStdString &name)
{
  CStdString path(CXBMCTestUtils::Instance().TempFilePath(m_file));
  CStdString directory(CXBMCTestUtils::Instance().TempFileDirectory(m_file));
  
  name = path.substr(directory.size());
}

class SavedTempSocket :
  boost::noncopyable
{
public:

  SavedTempSocket();

  const CStdString & FetchFilename();
  const CStdString & FetchDirectory();

private:

  CStdString m_filename;
  CStdString m_directory;
};

SavedTempSocket::SavedTempSocket()
{
  TempFileWrapper wrapper("");
  wrapper.FetchDirectory(m_directory);
  wrapper.FetchFilename(m_filename);
}

const CStdString &
SavedTempSocket::FetchFilename()
{
  return m_filename;
}

const CStdString &
SavedTempSocket::FetchDirectory()
{
  return m_directory;
}

class TmpEnv :
  boost::noncopyable
{
public:

  TmpEnv(const char *env, const char *val);
  ~TmpEnv();

private:

  const char *m_env;
  const char *m_previous;
};

TmpEnv::TmpEnv(const char *env,
               const char *val) :
  m_env(env),
  m_previous(getenv(env))
{
  setenv(env, val, 1);
}

TmpEnv::~TmpEnv()
{
  if (m_previous)
    setenv(m_env, m_previous, 1);
  else
    unsetenv(m_env);
}
}

xt::Process::Process(const CStdString &xbmcTestBase,
                     const CStdString &tempFileDirectory,
                     const CStdString &tempFileName) :
  m_pid(0)
{
  TmpEnv xdgRuntimeDir("XDG_RUNTIME_DIR", tempFileDirectory.c_str());
  
  std::stringstream socketOptionStream;
  socketOptionStream << "--socket=";
  socketOptionStream << tempFileName.c_str();
  
  std::string socketOption(socketOptionStream.str());

  std::stringstream modulesOptionStream;
  modulesOptionStream << "--modules=";
  modulesOptionStream << xbmcTestBase.c_str();
  modulesOptionStream << "xbmc/windowing/tests/wayland/xbmc-wayland-test-extension.so";
  
  std::string modulesOption(modulesOptionStream.str());
  
  const char *program = "/home/smspillaz/Applications/Compiz/bin/weston";
  const char *options[] =
  {
    program,
    "--backend=headless-backend.so",
    modulesOption.c_str(),
    socketOption.c_str(),
    NULL
  };
  
  m_pid = fork();
  
  switch (m_pid)
  {
    case 0:
      Child(program,
            const_cast <char * const *>(options));
    case -1:
      ForkError();
    default:
      Parent();
  }
}

pid_t
xt::Process::Pid()
{
  return m_pid;
}

void
xt::Process::Child(const char *program,
                   char * const *options)
{
  signal(SIGUSR2, SIG_IGN);
  
  /* Unblock SIGUSR2 */
  sigset_t signalMask;
  sigemptyset(&signalMask);
  sigaddset(&signalMask, SIGUSR1);
  if (sigprocmask(SIG_UNBLOCK, &signalMask, NULL))
  {
    std::stringstream ss;
    ss << "sigprocmask: " << strerror(errno);
    throw std::runtime_error(ss.str());
  }
  
  if (execvpe(program, options, environ) == -1)
  {
    std::stringstream ss;
    ss << "execvpe: " << strerror(errno);
    throw std::runtime_error(ss.str());
  }
}

void
xt::Process::Parent()
{
}

void
xt::Process::ForkError()
{
  std::stringstream ss;
  ss << "fork: "
     << strerror(errno);
  throw std::runtime_error(ss.str());
}

void
xt::Process::WaitForSignal(int signal, int timeout)
{
  sigset_t signalMask;
  sigemptyset(&signalMask);
  sigaddset(&signalMask, signal);
  sigaddset(&signalMask, SIGCHLD);
  
  if (sigprocmask(SIG_BLOCK, &signalMask, NULL))
  {
    std::stringstream ss;
    ss << "sigprogmask: "
       << strerror(errno);
    throw std::runtime_error(ss.str());
  }
  
  if (timeout >= 0)
  {
    static const uint32_t MsecToNsec = 1000000;
    static const uint32_t SecToMsec = 1000;
    int seconds = timeout / SecToMsec;
    
    /* Remove seconds from timeout */
    timeout -= seconds * SecToMsec;
    struct timespec ts = { seconds, timeout * MsecToNsec };
    
    sigemptyset(&signalMask);
    sigaddset(&signalMask, signal);
    
    errno = 0;
    int received = sigtimedwait(&signalMask,
                                NULL,
                                &ts);
    if (received != signal)
    {
      std::stringstream ss;
      ss << "sigtimedwait: "
         << strerror(errno)
         << " received signal "
         << received
         << " but expected signal "
         << signal;
      throw std::runtime_error(ss.str());
    }
    
    return;
  }
  else
  {
    int received = sigwaitinfo(&signalMask, NULL);
    
    if (received != signal)
    {
      std::stringstream ss;
      ss << "sigwaitinfo: "
         << strerror(errno);
    }
  }
}

void
xt::Process::WaitForStatus(int code, int timeout)
{
  struct timespec startTime;
  struct timespec currentTime;
  clock_gettime(CLOCK_MONOTONIC, &startTime);
  clock_gettime(CLOCK_MONOTONIC, &currentTime);
  
  const uint32_t SecToMsec = 1000;
  const uint32_t MsecToNsec = 1000000;
  
  int32_t startTimestamp = startTime.tv_sec * SecToMsec +
                           startTime.tv_nsec / MsecToNsec;
  int32_t currentTimestamp = currentTime.tv_sec * SecToMsec +
                             currentTime.tv_nsec / MsecToNsec;
  
  int options = WUNTRACED;
  
  std::stringstream statusMessage;
  
  if (timeout >= 0)
    options |= WNOHANG;
  
  do
  {
    clock_gettime(CLOCK_MONOTONIC, &currentTime);
    
    currentTimestamp = currentTime.tv_sec * SecToMsec +
                       currentTime.tv_nsec / MsecToNsec;
    
    int returnedStatus;
    pid_t pid = waitpid(m_pid, &returnedStatus, options);
    
    if (pid)
    {
      /* At least one child has exited */
      if (WIFEXITED(returnedStatus))
      {
        int returnedExitCode = WEXITSTATUS(returnedStatus);
        statusMessage << "process exited with status "
                      << returnedExitCode;
        if (returnedExitCode == code)
          return;
        
        /* Since we've been terminated but not in the way we
         * expect, we can just break here */
        break;
      }
      else if (WIFSIGNALED(returnedStatus))
      {
        int returnedSignal = WTERMSIG(returnedStatus);
        statusMessage << "process was terminated by signal "
                      << returnedSignal;
        if (returnedSignal == code)
          return;

        /* Since we've been terminated but not in the way we
         * expect, we can just break here */
        break;
      }
    }
    
    struct timespec ts;
    ts.tv_sec = 0;
    
    /* Don't sleep the whole time, we might have just missed
     * the signal */
    ts.tv_nsec = timeout * MsecToNsec / 10;
    
    nanosleep(&ts, NULL);
  }
  while (timeout == -1 ||
         (timeout > currentTimestamp - startTimestamp));

  /* If we didn't get out early, it means something went
   * wrong */

  std::stringstream ss;
  ss << "Process did not exit with code "
     << code
     << " within "
     << timeout
     << " milliseconds - "
     << statusMessage.str()
     << std::endl;
  throw std::runtime_error(ss.str());
}

void
xt::Process::SendSignal(int signal)
{
  if (kill(m_pid, signal) == -1)
  {
    /* Already dead ... lets see if it exited normally */
    if (errno == ESRCH)
    {
      try
      {
        WaitForStatus(0, 0);
      }
      catch (std::runtime_error &err)
      {
        std::stringstream ss;
        ss << err.what();
        ss << "Process was already dead";
        throw std::runtime_error(ss.str());
      }
    }
    else
    {
      std::stringstream ss;
      ss << "failed to send signal "
         << signal
         << " to process "
         << m_pid
         << ": " << strerror(errno);
      throw std::runtime_error(ss.str());
    }
  }
}

void
xt::Process::Interrupt()
{
  SendSignal(SIGINT);
}

void
xt::Process::Terminate()
{
  SendSignal(SIGTERM);
}

void
xt::Process::Kill()
{
  SendSignal(SIGKILL);
}

xt::Process::~Process()
{
  const static struct DeathActions
  {
    typedef void (Process::*SignalAction)(void);
    
    SignalAction action;
    int status;
  } deathActions[] =
  {
    { &Process::Interrupt, 0 },
    { &Process::Terminate, 0 },
    { &Process::Kill, SIGKILL }
  };

  static const size_t deathActionsSize = sizeof(deathActions) /
                                         sizeof(deathActions[0]);

  size_t i = 0;

  for (; i < deathActionsSize; ++i)
  {
    try
    {
      DeathActions::SignalAction func(deathActions[i].action);
      ((*this).*(func))();
      WaitForStatus(deathActions[i].status, DefaultProcessWaitTimeout);
      break;
    }
    catch (const std::runtime_error &err)
    {
      std::cout << err.what();
    }
  }
  
  if (i == deathActionsSize)
  {
    std::stringstream ss;
    ss << "Failed to terminate "
       << m_pid;
    
    /* Remove the socket lock so that we don't block other tests */
    remove("/run/user/1000/wayland-0.lock");
    
    if (getenv("ALLOW_WESTON_MISBEHAVIOUR"))
      std::cout << ss.str() << std::endl;
    else
      throw std::runtime_error(ss.str());
  }
}

class WestonTest :
  public ::testing::Test
{
public:

  WestonTest();
  pid_t Pid();
  
  virtual void SetUp();

protected:

  CEGLNativeTypeWayland m_nativeType;
  xw::Display *m_display;
  boost::scoped_ptr<xtw::XBMCWayland> m_xbmcWayland;
  struct wl_surface *m_mostRecentSurface;
  
  CStdString m_xbmcTestBase;
  SavedTempSocket m_tempSocketName;
  
private:

  void Global(struct wl_registry *, uint32_t, const char *, uint32_t);
  void DisplayAvailable(xw::Display &display);
  void SurfaceCreated(xw::Surface &surface);

  xt::Process m_process;
};

WestonTest::WestonTest() :
  m_display(NULL),
  m_mostRecentSurface(NULL),
  m_xbmcTestBase(CXBMCTestUtils::Instance().ReferenceFilePath("")),
  m_process(m_xbmcTestBase,
            m_tempSocketName.FetchDirectory(),
            m_tempSocketName.FetchFilename())
{
}

pid_t
WestonTest::Pid()
{
  return m_process.Pid();
}

void
WestonTest::SetUp()
{
  m_process.WaitForSignal(SIGUSR2, DefaultProcessWaitTimeout);
}

void
WestonTest::Global(struct wl_registry *registry,
                   uint32_t name,
                   const char *interface,
                   uint32_t version)
{
  if (std::string(interface) == "xbmc_wayland")
    m_xbmcWayland.reset(new xtw::XBMCWayland(static_cast<xbmc_wayland *>(wl_registry_bind(registry,
                                                                                          name,
                                                                                          &xbmc_wayland_interface,
                                                                                          version)),
                                             *m_display));
}

void
WestonTest::DisplayAvailable(xw::Display &display)
{
  m_display = &display;
}

void
WestonTest::SurfaceCreated(xw::Surface &surface)
{
  m_mostRecentSurface = surface.GetWlSurface();
}

TEST_F(WestonTest, TestCheckCompatibilityWithEnvSet)
{
  TmpEnv env("WAYLAND_DISPLAY", m_tempSocketName.FetchFilename().c_str());
  EXPECT_TRUE(m_nativeType.CheckCompatibility());
}

TEST_F(WestonTest, TestCheckCompatibilityWithEnvNotSet)
{
  EXPECT_FALSE(m_nativeType.CheckCompatibility());
}

TEST_F(WestonTest, TestConnection)
{
  TmpEnv env("WAYLAND_DISPLAY", m_tempSocketName.FetchFilename().c_str());
  m_nativeType.CheckCompatibility();
  m_nativeType.CreateNativeDisplay();
}

#endif
