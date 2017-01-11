include ../Makefile.in

EXE = wget
CFLAGS = -Wall -I$(INCLUDE_DIR) -DDEBUG -g
LDFLAGS = $(LIBRARY_DIR)/librtl.a -lssl

$(EXE): wget.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o $(EXE)
