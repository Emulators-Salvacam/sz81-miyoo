CCFLAGS=-Ofast -Wall
CXXFLAGS=-Ofast -Wall -DUSE_SND_EXTERNAL_BUFFER

SOURCES = sndrender.cpp sndcounter.cpp sndbuffer.cpp sndchip.cpp sndinterface.cpp
OBJECTS=$(patsubst %.cpp, %.o, $(SOURCES))

    ifeq ($(TOOLCHAIN),miyoo)
		# comment for destkop, only for Miyoo
		CC = /opt/miyoo/usr/bin/arm-linux-gcc
		CC_AS = /opt/miyoo/usr/bin/arm-linux-gcc
		CXX = /opt/miyoo/usr/bin/arm-linux-g++
		AR = /opt/miyoo/usr/bin/arm-linux-ar
		# comment for destkop, only for Miyoo
    endif

    ifeq ($(TOOLCHAIN),miyoo2)
		# comment for destkop, only for Miyoo
		CC = /opt/miyoo2/usr/bin/arm-linux-gcc
		CC_AS = /opt/miyoo2/usr/bin/arm-linux-gcc
		CXX = /opt/miyoo2/usr/bin/arm-linux-g++
		AR = /opt/miyoo2/usr/bin/arm-linux-ar
		# comment for destkop, only for Miyoo
    endif

    ifeq ($(TOOLCHAIN),gcw0)
		# comment for destkop, only for GCW0
		CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
		CC_AS = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
		CXX = /opt/gcw0-toolchain/usr/bin/mipsel-linux-g++
		AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
		# comment for destkop, only for GCW0
    endif


%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

libsndrender.a: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

clean:
	rm -f *.o *.a *~
