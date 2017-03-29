CFLAGS = `pkg-config --cflags opencv` -g -O3
LIBS = `pkg-config --libs opencv`
DEFS =
CPP = g++
OBJS = main.o

main: $(OBJS)
	$(CPP) $(OBJS) $(CFLAGS) $(DEFS) $(LIBS) -o $@

main.o: main.cpp main.h disjoint-set.h mouse.h
	$(CPP) $< $(CFLAGS) $(DEFS) -c

clean:
	rm -f LocalGc *.o *~