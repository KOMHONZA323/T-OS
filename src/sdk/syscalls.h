#ifndef SYSCALLS_H
#define SYSCALLS_H

/*
 * T-OS System Call Table
 *
 * This header defines the syscall numbers used by the kernel and user-space libraries.
 * It is not directly included by user applications (which use std.glh), but is
 * critical for kernel implementation and runtime development.
 */

#define SYS_EXIT            1
#define SYS_PRINT           2
#define SYS_GET_CHAR        3
#define SYS_CREATE_WINDOW   4
#define SYS_SPAWN_PROCESS   5
#define SYS_SLEEP           6

#endif
