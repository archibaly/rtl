include ../Makefile.in

EXE = wget
CFLAGS = -Wall -I$(INCLUDE_DIR) -DDEBUG
LDFLAGS = $(LIBRARY_DIR)/librtl.a.0.9.2 -lssl

$(EXE): wget.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o $(EXE)
