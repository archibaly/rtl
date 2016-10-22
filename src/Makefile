CC = cc
AR = ar
INCLUDE_DIR = ../include
LIBRARY_DIR = ../lib

SONAME = librtl.so.0
CFLAGS = -Wall -fPIC -I$(INCLUDE_DIR)
LDFLAGS = -Wl,-soname,$(SONAME) -shared

MAJOR_VERSION = 0
MICRO_VERSION = 9
MINOR_VERSION = 2

LIB.a = librtl.a.$(MAJOR_VERSION).$(MICRO_VERSION).$(MINOR_VERSION)
LIB.so = librtl.so.$(MAJOR_VERSION).$(MICRO_VERSION).$(MINOR_VERSION)

OBJS = rtl_cjson.o rtl_config.o rtl_crc32.o rtl_hash.o rtl_http.o rtl_https.o \
	   rtl_inet.o rtl_kmp.o rtl_lock.o rtl_log.o rtl_md5.o rtl_writen.o rtl_str.o \
	   rtl_pid.o rtl_readn.o rtl_sem.o rtl_send_file.o rtl_signal.o rtl_daemon.o \
	   rtl_socket.o rtl_tea.o rtl_time.o rtl_unlink_r.o rtl_url.o rtl_wget.o rtl_setproctitle.o \
	   rtl_tea.o

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
