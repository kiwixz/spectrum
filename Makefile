ifneq (,$(findstring d,$(MAKECMDGOALS))) # debug
  $(info * Debugging!)
  DEBUG=1

  DIR=bin/debug
  CFLAGS=-g -Og -DDEBUG
else # release
  DIR=bin/release
  CFLAGS=-DRELEASE
endif
DEPEND=$(addprefix $(DIR)/,.depend)
$(shell mkdir -p $(DIR))
$(shell touch -a $(DEPEND))

NAME=spectrum
PKGLIBS=glew gtk+-2.0 gtkglext-1.0 gstreamer-1.0 libtiff-4 opencv
LIBS=-lm
MACRO=-D_XOPEN_SOURCE -D_POSIX_C_SOURCE=199309L
OPT=-O3 -ffast-math

CC=c99
WARN=-Wall -Wextra -Wformat=2 -Winit-self -Wmissing-include-dirs -Wdeclaration-after-statement -Wshadow -Wno-aggressive-loop-optimizations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wstack-protector -Wno-missing-field-initializers -Wno-switch -Wno-unused-parameter
CFLAGS+=$(MACRO) $(WARN) $(shell pkg-config --cflags $(PKGLIBS)) $(OPT)
LDFLAGS=-march=native -pipe -m64 -m128bit-long-double -fdiagnostics-color=always $(shell pkg-config --libs $(PKGLIBS)) $(LIBS)
SRC=$(wildcard src/*.c)
OBJ=$(addprefix $(DIR)/,$(notdir $(SRC:.c=.o)))
PATHNAME=$(addprefix $(DIR)/,$(NAME))

all: depend $(NAME)

run: all
	@echo -e "\x1b[33;1mLaunching...\x1b[0m"
	@echo
ifdef DEBUG
	@gdb --args $(PATHNAME) $(ARGS)
else
	@bash -c "time $(PATHNAME) $(ARGS)"
endif

include $(DEPEND)

$(NAME): $(OBJ)
	@echo -e "\x1b[36mLinkage of $@...\x1b[0m"
	@$(CC) -o $(addprefix $(DIR)/,$@) $^ $(LDFLAGS)

$(addprefix $(DIR)/,%.o): src/%.c
	@echo -e "\x1b[32mCompilation of $@...\x1b[0m"
	@$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: depend clean

depend:
	@$(CC) -MM $(SRC) | sed 's~^\(\w*\.o\)~$(DIR)\/\1~g' > $(DEPEND)

clean:
	@echo -e "\x1b[35mCleaning...\x1b[0m"
	@rm -Rf bin
	@find . -name '*~' -delete
