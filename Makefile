include Makefile.inc

EXECUTABLES = app slave view
OBJECTS = $(EXECUTABLES:=.o)
EXE_DIR = ./bin
all: $(EXECUTABLES)
	mkdir -p $(EXE_DIR) ; mv -t $(EXE_DIR) $(EXECUTABLES)

app : app.o
	$(GCC) -o $@ $< -lrt -lpthread ; mv -t ./src app.o

view : view.o
	$(GCC) -o $@ $< -lrt -lpthread ; mv -t ./src view.o

slave: slave.o
	$(GCC) -o $@ $< ; mv -t ./src slave.o

%.o : src/%.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

clean:
	rm -r $(EXE_DIR) $(OBJECTS)

.PHONY: all clean