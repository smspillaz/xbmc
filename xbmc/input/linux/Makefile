SRCS=LIRC.cpp \
  LinuxInputDevices.cpp

# xkbcommon detail
SRCS += XKBCommonKeymap.cpp

LIB=input_linux.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
