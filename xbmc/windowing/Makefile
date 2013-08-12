SRCS=WinEventsSDL.cpp \
     WinEventsLinux.cpp \
     WinEventsWayland.cpp \
     WinSystem.cpp \

# Wayland implementation detail
SRCS += wayland/Seat.cpp \
        wayland/Pointer.cpp \
        wayland/Keyboard.cpp \
        wayland/PollThread.cpp \
        wayland/Wayland11EventQueueStrategy.cpp \
        wayland/Wayland12EventQueueStrategy.cpp

LIB=windowing.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
