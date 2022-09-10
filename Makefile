include Makefile.inc

APP = app
OBJECTS = slave.o md5.o

all: $(APP)

$(APP): $(OBJECTS) 
	$(GCC) -o $(APP) $?

md5.o: md5.c
	$(GCC) $(GCCFLAGS) -lrt -c $< -o $@

slave.o: slave.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

clean:
	rm -f $(APP) $(OBJECTS)

.PHONY: all clean