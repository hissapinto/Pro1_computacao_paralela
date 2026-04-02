CC = gcc
CFLAGS = -Wall -O2 
LIBS_PAR = -lpthread -lm
LIBS_SEQ = -lm

all: seq par opt

seq: sensor_analyzer_seq.c sensor_types.h
	$(CC) $(CFLAGS) -o seq sensor_analyzer_seq.c $(LIBS_SEQ)

par: sensor_analyzer_par.c sensor_types.h
	$(CC) $(CFLAGS) -o par sensor_analyzer_par.c $(LIBS_PAR)

opt: sensor_analyzer_optimized.c sensor_types.h
	$(CC) $(CFLAGS) -o opt sensor_analyzer_optimized.c $(LIBS_PAR)

clean:
	rm -f seq par opt
