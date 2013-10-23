INCLUDE_FLAGS=-I../../plugins/deweplugin

CFLAGS=$(INCLUDE_FLAGS)

CPP=g++

dewecli: dewecli.cpp ../../plugins/deweplugin/socket.cpp
	$(CPP) dewecli.cpp ../../plugins/deweplugin/socket.cpp $(CFLAGS) -o $@
