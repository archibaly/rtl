include Makefile.in

EXE = base64 blowfish dir file hash http https inet json \
	list pid proc sha1 sha256 shm socket spt str thread url wget \
	fcgi tar rbtree ini

all: $(EXE)

base64: base64.o
	$(CC) -o $@ $< $(LDFLAGS)

blowfish: blowfish.o
	$(CC) -o $@ $< $(LDFLAGS)

dir: dir.o
	$(CC) -o $@ $< $(LDFLAGS)

file: file.o
	$(CC) -o $@ $< $(LDFLAGS)

hash: hash.o
	$(CC) -o $@ $< $(LDFLAGS)

http: http.o
	$(CC) -o $@ $< $(LDFLAGS)

https: https.o
	$(CC) -o $@ $< $(LDFLAGS) -lssl

inet: inet.o
	$(CC) -o $@ $< $(LDFLAGS)

json: json.o
	$(CC) -o $@ $< $(LDFLAGS)

list: list.o
	$(CC) -o $@ $< $(LDFLAGS)

pid: pid.o
	$(CC) -o $@ $< $(LDFLAGS)

proc: proc.o
	$(CC) -o $@ $< $(LDFLAGS) -lrt

sha1: sha1.o
	$(CC) -o $@ $< $(LDFLAGS)

sha256: sha256.o
	$(CC) -o $@ $< $(LDFLAGS)

shm: shm.o
	$(CC) -o $@ $< $(LDFLAGS)

socket: socket.o
	$(CC) -o $@ $< $(LDFLAGS)

spt: spt.o
	$(CC) -o $@ $< $(LDFLAGS)

str: str.o
	$(CC) -o $@ $< $(LDFLAGS)

thread: thread.o
	$(CC) -o $@ $< $(LDFLAGS) -pthread -lrt

url: url.o
	$(CC) -o $@ $< $(LDFLAGS)

wget: wget.o
	$(CC) -o $@ $< $(LDFLAGS) -lssl

fcgi: fcgi.o
	$(CC) -o $@ $< $(LDFLAGS)

tar: tar.o
	$(CC) -o $@ $< $(LDFLAGS) -larchive

rbtree: rbtree.o
	$(CC) -o $@ $< $(LDFLAGS)

ini: ini.o
	$(CC) -o $@ $< $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o tags $(EXE)
