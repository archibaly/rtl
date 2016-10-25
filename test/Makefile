CC = cc
EXE = test
CFLAGS = -Wall -I../include -DDEBUG
LDFLAGS = ../lib/librtl.a.0.9.2 -lssl

$(EXE): main.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o $(EXE)
