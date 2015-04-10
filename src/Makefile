CXX=gcc
CXXFLAGS?= -Wall -g -O2 -std=gnu99 -Werror
# Add more c files here if necessary, except for images exported by GIMP (those are included, not linked)
CFILES= main.c text.c gamelib.c
HFILES= $(wildcard *.h)
OBJECTS= $(CFILES:%.c=%.o)
INCLUDES+=
LIBS+= -lrt -lncurses -lm
PROJECT= game

all: $(PROJECT)

%.o : %.c $(HFILES)
	@echo "[CC] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<
$(PROJECT): $(OBJECTS)
	@echo "[LD] $^"
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	@echo "Removing all binaries"
	@rm -f $(PROJECT) $(OBJECTS)
