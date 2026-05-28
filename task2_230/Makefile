# Based on template from Canvas Learning materials:
# PG3401_Lecture_04_Pointers.pdf - Slide 65
# Lecturer: Bengt Ostby

INCLDIR = ./include
CC = gcc
CFLAGS = -O2
CFLAGS += -I$(INCLDIR)

OBJDIR = obj

_DEPS = task2.h
DEPS = $(patsubst %,$(INCLDIR)/%,$(_DEPS))

_OBJS = task2_main.o task2_print.o task2_fib.o task2_prim.o task2_kvad.o task2_cube.o task2_perf.o task2_abun.o task2_def.o task2_odd.o
OBJS = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

$(OBJDIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

task2: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) -lm

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o *~ core $(INCLDIR)/*~ task2 pgexam26_output.bin
