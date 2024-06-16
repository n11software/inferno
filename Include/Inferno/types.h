#pragma once

typedef signed int signed_dword;
typedef signed int signed_word;
typedef signed int signed_byte;

typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef int pid_t;

typedef unsigned long int size_t;
typedef long int ssize_t;

typedef unsigned int ino_t;
typedef signed int off_t;

typedef unsigned int dev_t;
typedef unsigned int mode_t;
typedef unsigned int nlink_t;
typedef unsigned int blksize_t;
typedef unsigned int blkcnt_t;
typedef unsigned int time_t;
typedef unsigned int suseconds_t;

struct timeval {
    time_t tv_sec;
    suseconds_t tv_usec;
};

struct stat {
    dev_t st_dev;         /* ID of device containing file */
    ino_t st_ino;         /* inode number */
    mode_t st_mode;       /* protection */
    nlink_t st_nlink;     /* number of hard links */
    uid_t st_uid;         /* user ID of owner */
    gid_t st_gid;         /* group ID of owner */
    dev_t st_rdev;        /* device ID (if special file) */
    off_t st_size;        /* total size, in bytes */
    blksize_t st_blksize; /* blocksize for file system I/O */
    blkcnt_t st_blocks;   /* number of 512B blocks allocated */
    time_t st_atime;      /* time of last access */
    time_t st_mtime;      /* time of last modification */
    time_t st_ctime;      /* time of last status change */
};

#ifdef __cplusplus
#    define NULL nullptr
#else
#    define NULL 0
#endif
