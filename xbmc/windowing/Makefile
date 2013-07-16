SRCS=WinEventsSDL.cpp \
     WinEventsLinux.cpp \
     WinEventsWayland.cpp \
     WinSystem.cpp \

# Wayland implementation detail
SRCS += wayland/Seat.cpp

LIB=windowing.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
