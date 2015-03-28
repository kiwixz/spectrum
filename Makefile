ifneq (,$(findstring d,$(MAKECMDGOALS))) # debug
  $(info * Debugging!)
  DEBUG=1

  DIR=./bin/debug
  CFLAGS=-g -Og -DDEBUG
else # release
  DIR=./bin/release
  CFLAGS=-O3 -DRELEASE
endif
$(shell mkdir -p $(DIR))

NAME=spectrum
PKGLIBS=glew gtk+-2.0 gtkglext-1.0 gstreamer-1.0
LIBS=

PATHNAME=$(addprefix $(DIR)/,$(NAME))
CC=c99
WARNINGS=-Wall -Wextra -Wformat=2 -Winit-self -Wmissing-include-dirs -Wdeclaration-after-statement -Wshadow -Wno-aggressive-loop-optimizations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wstack-protector -Wno-missing-field-initializers -Wno-switch -Wno-unused-parameter
CFLAGS+=$(WARNINGS) $(shell pkg-config --cflags $(PKGLIBS))
LDFLAGS=-march=native -pipe -m64 -m128bit-long-double -fdiagnostics-color=always $(shell pkg-config --libs $(PKGLIBS)) $(LIBS)
SRC=$(wildcard src/*.c)
OBJ=$(addprefix $(DIR)/,$(notdir $(SRC:.c=.o)))


all: .depend $(NAME)

run: all
	@echo -e "\x1b[33;1mLaunching...\x1b[0m"
	@echo
ifdef DEBUG
	@gdb --args $(PATHNAME) $(ARGS)
else
	@bash -c "time $(PATHNAME) $(ARGS)"
endif

.depend: $(SRC) $(wildcard *.h)
	@echo "Recomputing dependencies..."
	@rm -f ./.depend
	@$(CC) -MM $^ > ./.depend

include .depend

$(NAME): $(OBJ)
	@echo -e "\x1b[36mLinkage of $@...\x1b[0m"
	@$(CC) -o $(addprefix $(DIR)/,$@) $^ $(LDFLAGS)

$(addprefix $(DIR)/,%.o): src/%.c
	@echo -e "\x1b[32mCompilation of $@...\x1b[0m"
	@$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	@echo -e "\x1b[35mCleaning...\x1b[0m"
	@rm -Rf bin .depend
	@find . -name '*~' -delete
