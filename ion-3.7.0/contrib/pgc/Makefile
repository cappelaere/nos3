IONPATH=/usr/local
CC=gcc
OPTFLAGS=-g
#OPTFLAGS=-O3 -fomit-frame-pointer -ffast-math -fmerge-all-constants -maccumulate-outgoing-args -march=core2 -U_FORTIFY_SOURCE -fno-stack-protector
CFLAGS=-I$(IONPATH)/include -std=gnu99 -Wall $(OPTFLAGS)
#LDFLAGS=-pthread -L$(IONPATH)/lib -lipnfw -limcfw -ldtn2fw -lbp -lici
#LDFLAGS=-pthread -L$(IONPATH)/lib  -lbp -lici
LDFLAGS=-fPIC -shared -lpthread -lm -L$(IONPATH)/lib  -lbp -lici

all: ccsds_sink ccsds_source 

ccsds_source: ccsds_source.o
	$(CC) ccsds_source.o -o ccsds_source $(LDFLAGS)

ccsds_sink.o: ccsds_sink.c
	$(CC) -c ccsds_sink.c $(CFLAGS)

clean:
	rm -rf *.o ccsds_sink ccsds_source
