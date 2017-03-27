include ../Makefile.in

EXE = sha1
CFLAGS = -Wall -g -I$(INCLUDE_DIR) -DDEBUG
LDFLAGS = $(LIBRARY_DIR)/librtl.a

$(EXE): sha1.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o tags $(EXE)
