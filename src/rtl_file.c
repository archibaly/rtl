#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>

#include "rtl_file.h"

/*
 * Most of these MAGIC constants are defined in /usr/include/linux/magic.h,
 * and some are hardcoded in kernel sources.
 */
typedef enum fs_type_supported {
	FS_CIFS     = 0xFF534D42,
	FS_CRAMFS   = 0x28cd3d45,
	FS_DEBUGFS  = 0x64626720,
	FS_DEVFS    = 0x1373,
	FS_DEVPTS   = 0x1cd1,
	FS_EXT      = 0x137D,
	FS_EXT2_OLD = 0xEF51,
	FS_EXT2     = 0xEF53,
	FS_EXT3     = 0xEF53,
	FS_EXT4     = 0xEF53,
	FS_FUSE     = 0x65735546,
	FS_JFFS2    = 0x72b6,
	FS_MQUEUE   = 0x19800202,
	FS_MSDOS    = 0x4d44,
	FS_NFS      = 0x6969,
	FS_NTFS     = 0x5346544e,
	FS_PROC     = 0x9fa0,
	FS_RAMFS    = 0x858458f6,
	FS_ROMFS    = 0x7275,
	FS_SELINUX  = 0xf97cff8c,
	FS_SMB      = 0x517B,
	FS_SOCKFS   = 0x534F434B,
	FS_SQUASHFS = 0x73717368,
	FS_SYSFS    = 0x62656572,
	FS_TMPFS    = 0x01021994
} fs_type_supported_t;

static struct {
	const char name[32];
	const int value;
} fs_type_info[] = {
	{"CIFS    ", FS_CIFS    },
	{"CRAMFS  ", FS_CRAMFS  },
	{"DEBUGFS ", FS_DEBUGFS },
	{"DEVFS   ", FS_DEVFS   },
	{"DEVPTS  ", FS_DEVPTS  },
	{"EXT     ", FS_EXT     },
	{"EXT2_OLD", FS_EXT2_OLD},
	{"EXT2    ", FS_EXT2    },
	{"EXT3    ", FS_EXT3    },
	{"EXT4    ", FS_EXT4    },
	{"FUSE    ", FS_FUSE    },
	{"JFFS2   ", FS_JFFS2   },
	{"MQUEUE  ", FS_MQUEUE  },
	{"MSDOS   ", FS_MSDOS   },
	{"NFS     ", FS_NFS     },
	{"NTFS    ", FS_NTFS    },
	{"PROC    ", FS_PROC    },
	{"RAMFS   ", FS_RAMFS   },
	{"ROMFS   ", FS_ROMFS   },
	{"SELINUX ", FS_SELINUX },
	{"SMB     ", FS_SMB     },
	{"SOCKFS  ", FS_SOCKFS  },
	{"SQUASHFS", FS_SQUASHFS},
	{"SYSFS   ", FS_SYSFS   },
	{"TMPFS   ", FS_TMPFS   },
};

extern const struct rtl_file_ops rtl_io_ops;
extern const struct rtl_file_ops rtl_fio_ops;

static const struct rtl_file_ops *file_ops[] = {
	&rtl_io_ops,
	&rtl_fio_ops,
	NULL
};

#define array_size(array)	(sizeof(array)/sizeof(array[0]))

static rtl_file_backend_type_t backend = RTL_FILE_BACKEND_IO;
static char local_path[PATH_MAX];

void rtl_file_backend(rtl_file_backend_type_t type)
{
	backend = type;
}

struct rtl_file *rtl_file_open(const char *path, rtl_file_open_mode_t mode)
{
	struct rtl_file *file = (struct rtl_file *)calloc(1, sizeof(struct rtl_file));
	if (!file) {
		fprintf(stderr, "malloc failed!\n");
		return NULL;
	}
	file->ops = file_ops[backend];
	file->fd = file->ops->open(path, mode);
	if (!file->fd) {
		free(file);
		return NULL;
	}
	return file;
}

void rtl_file_close(struct rtl_file *file)
{
	return file->ops->close(file->fd);
}

ssize_t rtl_file_read(struct rtl_file *file, void *data, size_t size)
{
	return file->ops->read(file->fd, data, size);
}

ssize_t rtl_file_write(struct rtl_file *file, const void *data, size_t size)
{
	return file->ops->write(file->fd, data, size);
}

ssize_t rtl_file_size(struct rtl_file *file)
{
	return file->ops->size(file->fd);
}

int rtl_file_sync(struct rtl_file *file)
{
	return file->ops->sync(file->fd);
}

off_t rtl_file_seek(struct rtl_file *file, off_t offset, int whence)
{
	return file->ops->seek(file->fd, offset, whence);
}

ssize_t rtl_file_get_size(const char *path)
{
	struct stat st;
	off_t size = 0;
	if (stat(path, &st) < 0) {
		fprintf(stderr, "%s stat error: %s\n", path, strerror(errno));
	} else {
		size = st.st_size;
	}
	return size;
}

struct iovec *rtl_file_dump(const char *path)
{
	ssize_t size = rtl_file_get_size(path);
	if (size == 0) {
		return NULL;
	}
	struct iovec *buf = (struct iovec *)calloc(1, sizeof(struct iovec));
	if (!buf) {
		fprintf(stderr, "malloc failed!\n");
		return NULL;
	}
	buf->iov_len = size;
	buf->iov_base = calloc(1, buf->iov_len);
	if (!buf->iov_base) {
		fprintf(stderr, "malloc failed!\n");
		return NULL;
	}

	struct rtl_file *f = rtl_file_open(path, RTL_F_RDONLY);
	if (!f) {
		fprintf(stderr, "file open failed!\n");
		free(buf->iov_base);
		free(buf);
		return NULL;
	}
	rtl_file_read(f, buf->iov_base, buf->iov_len);
	rtl_file_close(f);
	return buf;
}

struct rtl_file_systat *rtl_file_get_systat(const char *path)
{
	if (!path) {
		fprintf(stderr, "path can't be null\n");
		return NULL;
	}
	struct statfs stfs;
	if (statfs(path, &stfs) < 0) {
		fprintf(stderr, "statfs %s failed: %s\n", path, strerror(errno));
		return NULL;
	}
	struct rtl_file_systat *fi = (struct rtl_file_systat *)calloc(1,
			sizeof(struct rtl_file_systat));
	if (!fi) {
		fprintf(stderr, "malloc failed!\n");
		return NULL;
	}
	fi->size_total = stfs.f_bsize * stfs.f_blocks;
	fi->size_avail = stfs.f_bsize * stfs.f_bavail;
	fi->size_free  = stfs.f_bsize * stfs.f_bfree;

	int i;
	for (i = 0; i < array_size(fs_type_info); i++) {
		if (stfs.f_type == fs_type_info[i].value) {
			stfs.f_type = i;
			strncpy(fi->fs_type_name, fs_type_info[i].name,
					sizeof(fi->fs_type_name));
			break;
		}
	}
	return fi;
}

char *rtl_file_path_pwd(void)
{
	char *tmp = getcwd(local_path, sizeof(local_path));
	if (!tmp) {
		fprintf(stderr, "getcwd failed: %s\n", strerror(errno));
	}
	return tmp;
}

char *rtl_file_path_suffix(char *path)
{
	return basename(path);
}

char *rtl_file_path_prefix(char *path)
{
	return dirname(path);
}
