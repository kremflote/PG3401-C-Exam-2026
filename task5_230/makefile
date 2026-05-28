# Based on template from Canvas Learning materials:
# PG3401_Lecture_04_Pointers.pdf - Slide 65
# Lecturer: Bengt Ostby

INCLDIR = ./include
CC      = gcc
CFLAGS  = -O2
CFLAGS += -I$(INCLDIR)

OBJDIR = obj

_DEPS = server.h
DEPS  = $(patsubst %,$(INCLDIR)/%,$(_DEPS))

_OBJS = task5_server.o
OBJS  = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

$(OBJDIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

task5: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o *~ core $(INCLDIR)/*~

