include Makefile.inc

EXECUTABLES = app slave
OBJECTS = $(EXECUTABLES:=.o)

all: $(EXECUTABLES)

app : app.o
	$(GCC) -o $@ $< -lrt

slave: slave.o
	$(GCC) -o $@ $<

%.o : src/%.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

clean:
	rm -f $(EXECUTABLES) $(OBJECTS)

.PHONY: all clean