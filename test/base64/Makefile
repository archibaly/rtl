include ../Makefile.in

EXE = base64
CFLAGS = -Wall -g -I$(INCLUDE_DIR) -DDEBUG
LDFLAGS = $(LIBRARY_DIR)/librtl.a

$(EXE): base64.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o tags $(EXE)
