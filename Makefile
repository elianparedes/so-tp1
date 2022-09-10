include Makefile.inc

EXECUTABLES = app slave view
OBJECTS = $(EXECUTABLES:=.o)

all: $(EXECUTABLES)

app : app.o
	$(GCC) -o $@ $< -lrt

view : view.o
	$(GCC) -o $@ $< -lrt

slave: slave.o
	$(GCC) -o $@ $<

%.o : src/%.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

clean:
	rm -f $(EXECUTABLES) $(OBJECTS)

.PHONY: all clean