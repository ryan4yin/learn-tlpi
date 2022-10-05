/*
编写程序列出打开了某一文件句柄的所有进程
*/

#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <malloc.h>
#include <dirent.h>
#include <tlpi_hdr.h>

#define BUF_SIZE 4096

void check_process_fd(int pid, char *filename);

int main(int argc, char *argv[])
{
    DIR *proc_dir = NULL;
    struct dirent *tmp_dirent = NULL;
    char *filename = NULL;
    long n = 0;

    if (argc != 2)
    {
        printf("usage: <filename>\n");
        return 0;
    }
    filename = argv[1];

    proc_dir = opendir("/proc");
    if (proc_dir == NULL)
    {
        errExit("opendir: /proc");
    }

    while ((tmp_dirent = readdir(proc_dir)) != NULL)
    {
        /*
          the calling program should set errno to 0 before the
          call, and then determine if an error occurred by checking whether
          errno has a nonzero value after the call.
       */
        errno = 0;
        n = strtol(tmp_dirent->d_name, NULL, 10);
        if (errno != 0)
        {
            // d_name is not a process id, skip it
            continue;
        }
        // check process's opening fd
        check_process_fd(n, filename);
    }
    closedir(proc_dir);

    return 0;
}

void check_process_fd(int pid, char *filename)
{
    static char dir_path[BUF_SIZE] = {0};
    static char fd_path[BUF_SIZE] = {0};
    static char buf[BUF_SIZE] = {0};
    static DIR *fd_dir = NULL;
    static struct dirent *tmp_dirent = NULL;

    sprintf(dir_path, "/proc/%d/fd", pid);

    fd_dir = opendir(dir_path);
    if (fd_dir == NULL)
    {
        // the process had gone.
        return;
    }

    while ((tmp_dirent = readdir(fd_dir)) != NULL)
    {
        /*
          define _DEFAULT_SOURCE on glibc versions since 2.19, glibc
          defines the following macro constants for the value
          returned in d_type:

          DT_REG: a regular file
        */
        if (tmp_dirent->d_type != DT_LNK) {
            continue;
        }
        sprintf(fd_path, "%s/%s", dir_path, tmp_dirent->d_name);
        // read fd's real file path
        readlink(fd_path, buf, BUF_SIZE);
        if (strcmp(buf, filename) == 0)
        {
            printf("%s => %s\n", fd_path, filename);
        }
    }
    closedir(fd_dir);
}
