#ifndef _RTL_FILE_H_
#define _RTL_FILE_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/uio.h>

typedef enum file_open_mode {
	RTL_F_RDONLY,
	RTL_F_WRONLY,
	RTL_F_RDWR,
	RTL_F_CREATE,
	RTL_F_WRCLEAR,
} rtl_file_open_mode_t;

struct rtl_file_desc {
	union {
		int fd;
		FILE *fp;
	};
	char *name;
};

struct rtl_file {
	struct rtl_file_desc *fd;
	const struct rtl_file_ops *ops;
	uint64_t size;
};

struct rtl_file_info {
	struct timespec time_modify;
	struct timespec time_access;
	uint64_t size;
};

struct rtl_file_systat {
	uint64_t size_total;
	uint64_t size_avail;
	uint64_t size_free;
	char fs_type_name[32];
};

typedef struct rtl_file_ops {
	struct rtl_file_desc * (*open)(const char *path, rtl_file_open_mode_t mode);
	ssize_t (*write)(struct rtl_file_desc *fd, const void *buf, size_t count);
	ssize_t (*read)(struct rtl_file_desc *fd, void *buf, size_t count);
	off_t (*seek)(struct rtl_file_desc *fd, off_t offset, int whence);
	int (*sync)(struct rtl_file_desc *fd);
	size_t (*size)(struct rtl_file_desc *fd);
	void (*close)(struct rtl_file_desc *fd);
} rtl_file_ops_t;

typedef enum rtl_file_backend_type {
	RTL_FILE_BACKEND_IO,
	RTL_FILE_BACKEND_FIO,
} rtl_file_backend_type_t;

void rtl_file_backend(rtl_file_backend_type_t type);
struct rtl_file *rtl_file_open(const char *path, rtl_file_open_mode_t mode);
void rtl_file_close(struct rtl_file *file);
ssize_t rtl_file_read(struct rtl_file *file, void *data, size_t size);
ssize_t rtl_file_write(struct rtl_file *file, const void *data, size_t size);
ssize_t rtl_file_size(struct rtl_file *file);
ssize_t rtl_file_get_size(const char *path);
struct iovec *rtl_file_dump(const char *path);
struct rtl_file_systat *rtl_file_get_systat(const char *path);
int rtl_file_sync(struct rtl_file *file);
off_t rtl_file_seek(struct rtl_file *file, off_t offset, int whence);
struct rtl_rtl_file_systat *file_get_systat(const char *path);
char *rtl_file_path_pwd(void);
char *rtl_file_path_suffix(char *path);
char *rtl_file_path_prefix(char *path);

#endif /* _RTL_FILE_H_ */
