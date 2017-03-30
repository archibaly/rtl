include ../Makefile.in

EXE = file
CFLAGS = -Wall -g -I$(INCLUDE_DIR) -DDEBUG
LDFLAGS = $(LIBRARY_DIR)/librtl.a

$(EXE): file.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o tags $(EXE)
