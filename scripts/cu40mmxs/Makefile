# Detector Control Gui Test Clients
# Nico Nell

all: sensor-sciences-simulator cu40mmxstest gauss

CC = gcc
CCPP = g++
LIBS = -lpthread

DEBUG_FLAGS = -g
OPTIMIZE_FLAGS = -O3 -mtune=native -march=native

CFLAGS = -Wall ${DEBUG_FLAGS} ${OPTIMIZE_FLAGS}
LDFLAGS = ${LIBS}

options:
	@echo semclient build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

sensor-sciences-simulator: sensor-sciences-simulator.c
	@${CC} ${CFLAGS} -o $@  sensor-sciences-simulator.c ${LDFLAGS}

cu40mmxstest: cu40mmxstest.c
	@${CC} ${CFLAGS} -o $@  cu40mmxstest.c ${LDFLAGS}

gauss: gauss.cpp
	@${CCPP} ${CFLAGS} -o $@  gauss.cpp ${LDFLAGS}

clean:
	@rm -f sensor-sciences-simulator cu40mmxstest gauss *.o *\~

.PHONY: all options clean
