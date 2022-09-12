include Makefile.inc

EXECUTABLES = app slave view
OBJECTS = $(EXECUTABLES:=.o)
EXE_DIR = ./bin
SRC_DIR = ./src

all: $(EXECUTABLES)
	@mkdir -p $(EXE_DIR) ; mv -t $(EXE_DIR) $(EXECUTABLES)

app : app.o
	@$(GCC) -o $@ $< -lrt -lpthread -lm; mv -t $(SRC_DIR) $<

view : view.o
	@$(GCC) -o $@ $< -lrt -lpthread ; mv -t $(SRC_DIR) $<

slave: slave.o
	@$(GCC) -o $@ $< ; mv -t $(SRC_DIR) $<

%.o : $(SRC_DIR)/%.c
	@$(GCC) $(GCCFLAGS) -c $< -o $@

clean: cleanexe cleanobj

cleanexe:
	@rm -rf $(EXE_DIR)

cleanobj:
	 @rm -f $(addprefix $(SRC_DIR)/,$(OBJECTS))

.PHONY: all clean