# Based on template from Canvas Learning materials:
# PG3401_Lecture_04_Pointers.pdf - Slide 65
# Lecturer: Bengt Ostby

INCLDIR = ./include
CC = gcc
CFLAGS = -O2 -pthread -include stddef.h
CFLAGS += -I$(INCLDIR) 

OBJDIR = obj

_DEPS = task4_prime.h
DEPS = $(patsubst %,$(INCLDIR)/%,$(_DEPS))

_OBJS = task4_prime.o task4_threads.o xtea.o ipc.o
OBJS = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

$(OBJDIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

task4: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o *~ core $(INCLDIR)/*~ task4 task4_plain.txt task4_plain.hash

