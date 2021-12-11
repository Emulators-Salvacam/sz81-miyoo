CCFLAGS=-Ofast -Wall
CXXFLAGS=-Ofast -Wall -DUSE_SND_EXTERNAL_BUFFER

SOURCES = sndrender.cpp sndcounter.cpp sndbuffer.cpp sndchip.cpp sndinterface.cpp
OBJECTS=$(patsubst %.cpp, %.o, $(SOURCES))

	# comment for destkop, only for Miyoo
	CC = /opt/miyoo/usr/bin/arm-linux-gcc
	CC_AS = /opt/miyoo/usr/bin/arm-linux-gcc
	CXX = /opt/miyoo/usr/bin/arm-linux-g++
	AR = /opt/miyoo/usr/bin/arm-linux-ar
	# comment for destkop, only for Miyoo

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

libsndrender.a: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

clean:
	rm -f *.o *.a *~
