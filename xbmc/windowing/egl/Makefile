INCLUDES=-I.

SRCS = WinSystemEGL.cpp
SRCS+= EGLNativeTypeAmlogic.cpp
SRCS+= EGLNativeTypeAndroid.cpp
SRCS+= EGLNativeTypeMir.cpp
SRCS+= EGLNativeTypeRaspberryPI.cpp
SRCS+= EGLNativeTypeWayland.cpp
SRCS+= EGLWrapper.cpp

# Wayland specific detail
SRCS+= wayland/Callback.cpp \
       wayland/Compositor.cpp \
       wayland/Display.cpp \
       wayland/OpenGLSurface.cpp \
       wayland/Output.cpp \
       wayland/Region.cpp \
       wayland/Registry.cpp \
       wayland/Shell.cpp \
       wayland/ShellSurface.cpp \
       wayland/Surface.cpp \
       wayland/WaylandLibraries.cpp \
       wayland/XBMCConnection.cpp \
       wayland/XBMCSurface.cpp

# Mir specific detail
SRCS+= mir/Connection.cpp \
       mir/Surface.cpp \
       mir/MirLibraries.cpp \
       mir/XBMCConnection.cpp \
       mir/XBMCSurface.cpp

LIB = windowing_egl.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
