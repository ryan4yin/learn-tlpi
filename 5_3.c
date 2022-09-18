// 需要在 32 位机上才能测试出，有没有下面这个测试宏的区别
#define _LARGEFILE64_SOURCE
#include <sys/stat.h>
#include <fcntl.h>
#include <tlpi_hdr.h>

int main(int argc, char *argv[])
{
    int fd;
    int open_flags = O_APPEND;
    off64_t off;
    if (argc < 3 || strcmp(argv[1], "--help") == 0)
    {
        usageErr("%s pathname offset [x]\n", argv[0]);
    }

    if (argc == 4)
    {
        if (argv[3][0] == 'x')
        {
            open_flags = 0000;
        }
        else
        {
            usageErr("%s pathname offset [x]\n", argv[0]);
        }
    }

    fd = open(argv[1], O_RDWR | O_CREAT | open_flags, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        errExit("open failed");
    }
    lseek(fd, 0, SEEK_END);

    off = atoll(argv[2]);
    printf("offset: %lld", off);
    for (int i = 0; i < off; i++)
    {
        if (open_flags == O_APPEND && lseek(fd, 0, SEEK_END) == -1)
        {
            errExit("lseek");
        }

        usleep(1);

        if (write(fd, "1", 1) == -1)
        {
            errExit("write");
        }
    }

    exit(EXIT_SUCCESS);
}