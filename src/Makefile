CC = cc
AR = ar
INCLUDE_DIR = ../include
LIBRARY_DIR = ../lib

SONAME = librtl.so.0
CFLAGS = -Wall -fPIC -I$(INCLUDE_DIR) -DDEBUG
LDFLAGS = -Wl,-soname,$(SONAME) -shared

MAJOR_VERSION = 0
MICRO_VERSION = 9
MINOR_VERSION = 6

LIB.a = librtl.a
LIB.so = librtl.so.$(MAJOR_VERSION).$(MICRO_VERSION).$(MINOR_VERSION)

OBJS = rtl_json.o rtl_config.o rtl_crc32.o rtl_hash.o rtl_http.o rtl_https.o \
	   rtl_inet.o rtl_kmp.o rtl_flock.o rtl_log.o rtl_md5.o rtl_str.o \
	   rtl_pid.o rtl_readn.o rtl_sem.o rtl_send_file.o rtl_signal.o rtl_writen.o \
	   rtl_socket.o rtl_tea.o rtl_time.o rtl_dir.o rtl_url.o rtl_wget.o \
	   rtl_spt.o rtl_tea.o rtl_iconv.o rtl_shm.o rtl_sem.o rtl_proc.o rtl_table.o \
	   rtl_lock.o rtl_thread.o rtl_event.o rtl_epoll.o rtl_sha1.o rtl_sha256.o \
	   rtl_base64.o rtl_blowfish.o rtl_file.o rtl_fio.o rtl_io.o

all: $(LIBRARY_DIR)/$(LIB.a) $(LIBRARY_DIR)/$(LIB.so)

$(LIBRARY_DIR)/$(LIB.a): $(OBJS)
	$(AR) -crv $@ $^

$(LIBRARY_DIR)/$(LIB.so): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	ln -sf $(LIB.so) $(LIBRARY_DIR)/$(SONAME)
	ln -sf $(SONAME) $(LIBRARY_DIR)/librtl.so

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o $(LIBRARY_DIR)/librtl.so $(LIBRARY_DIR)/$(SONAME) \
			  $(LIBRARY_DIR)/$(LIB.a) $(LIBRARY_DIR)/$(LIB.so)
