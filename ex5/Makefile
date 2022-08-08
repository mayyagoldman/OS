CC=g++
SRC=Container.cpp Sockets.cpp
BJS=$(SRC:.cpp=.o)
TARGETS=container sockets

TAR=tar
TARFLAGS=-cvf
TARNAME=ex5.tar
TARSRCS=$(SRC) Makefile README

all:$(TARGETS)

container: Container.cpp
	$(CC) $^ -o container

sockets: Sockets.cpp
	$(CC) $^ -o sockets
clean:
	$(RM) $(TARGETS) *~ *core
tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
