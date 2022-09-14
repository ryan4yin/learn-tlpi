#include <tlpi_hdr.h>
#include <fcntl.h>

#define MAXLINE 4096

int getopt(int argc, char *const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

int main(int argc, char **argv)
{
    int opt = 0;
    int is_append = 0;
    int tee_fd[MAXLINE] = {0};
    int fd_no = 0;
    char temp[MAXLINE] = {0};
    ssize_t n = 0;

    // read the command line arguments
    // If there are no more option characters, getopt() returns -1.
    while ((opt = getopt(argc, argv, "a-:")) != -1)
    {
        switch (opt)
        {
        case 'a':
            is_append = 1;
            break;
        case '-':
            if (strcmp(optarg, "help") == 0)
            {
                printf(
                    "Usage: tee [Option]... [File]...\n"
                    "Copy standard input to each file, and also to standard output.\n"
                    "-a append to the given FILEs, do not overwrite.\n");
            }
            exit(0);
        case '?':
        default:
            exit(0);
        }
    }

    // The variable optind is the index of the next element to be
    //   processed in argv.  The system initializes this value to 1.
    for (int i = optind; i < argc; i++, fd_no++)
    {
        if (is_append)
        {
            // open all the files, which is non-option arguments.
            tee_fd[fd_no] = open(argv[i], O_CREAT | O_WRONLY | O_APPEND, 0666);
            if (tee_fd[fd_no] < 0)
            {
                exit(1);
            }
        }
        else
        {
            // open all the files, which is non-option arguments.
            tee_fd[fd_no] = open(argv[i], O_CREAT | O_WRONLY | O_TRUNC, 0666);
            if (tee_fd[fd_no] < 0)
            {
                exit(1);
            }
        }
    }

    // read and write to multiple outputs
    while ((n = read(STDIN_FILENO, temp, MAXLINE)) > 0)
    {
        // write to stdout
        if (write(STDOUT_FILENO, temp, n) != n)
        {
            exit(1);
        }

        // write to other output files
        for (int i = 0; i < fd_no; i++)
        {
            if ((n = write(tee_fd[i], temp, n)) != n)
            {
                exit(1);
            }
        }
    }

    // failed to read
    if (n < 0)
    {
        exit(1);
    }

    // close all the files
    for (int i = 0; i < fd_no; i++)
    {
        close(tee_fd[i]);
    }
}