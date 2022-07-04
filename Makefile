CC       ?= gcc
CFLAGS   ?= 
CXX      ?= g++
CXXFLAGS ?= -std=c++11
LIBS      = -lpthread
INCLUDES ?= 


#Uncomment for Debuging Output
DEFINES += -DDEBUG


OBJCOMMON = memory.o mttp.o display.o mtcpserver.o tracerctr.o modbus.o modbusserver.o
OBJCLIENT = tracerclient.o client.o
OBJSERVER = tracerserver.o server.o
OBJDISPLAY = displaytest.o modbus.o
OBJTRACER  = tracertest.o modbus.o
OBJMODBUSSEND = modbussend.o modbus.o

all: client server

client: $(OBJCLIENT) $(OBJCOMMON)
	$(CXX) $(CXXFLAGS) -o tracerclient $(OBJCLIENT) $(OBJCOMMON) $(LIBS)

server: $(OBJSERVER) $(OBJCOMMON)
	$(CXX) $(CXXFLAGS) -o tracerserver $(OBJSERVER) $(OBJCOMMON) $(LIBS)
	
displaytest: $(OBJDISPLAY)
	$(CXX) $(CXXFLAGS) -o displaytest $(OBJDISPLAY) $(LIBS)
	
tracertest: $(OBJTRACER)
	$(CXX) $(CXXFLAGS) -o tracertest $(OBJTRACER) $(LIBS)
	
modbussend: $(OBJMODBUSSEND)
	$(CXX) $(CXXFLAGS) -o modbussend $(OBJMODBUSSEND) $(LIBS)


%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

clean:
	rm -rf tracerclient tracerserver tracertest displaytest modbussend $(OBJCOMMON) $(OBJCLIENT) $(OBJSERVER) $(OBJDISPLAY) $(OBJTRACER) $(OBJMODBUSSEND)


