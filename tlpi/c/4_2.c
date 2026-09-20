#include <tlpi_hdr.h>
#include <fcntl.h>

#define MAXCHARS 4096

// generate file with hole
// int main(int argc, char **argv)
// {
//     int fd;
//     char string1[MAXCHARS] = "it is a dream that will never come true...\n";
//     char string2[MAXCHARS] = "\nbut never give up, always try to do something meaningful...\n";

//     if ((fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0666)) < 0)
//     {
//         exit(1);
//     }

//     if (write(fd, string1, sizeof string1) < 0)
//     {
//         exit(1);
//     }

//     int64_t index = 1024 * 1024 * 10; // 10 mib
//     lseek(fd, index, SEEK_SET);

//     if (write(fd, string2, sizeof string1) < 0)
//     {
//         exit(1);
//     }

//     close(fd);
// }



int main(int argc, char **argv)
{
    int input_fd, output_fd;
    char temp[MAXCHARS] = {0};
    ssize_t n = 0;
    ssize_t seek_index = 0;
    ssize_t i = 0;

    if ((input_fd = input_fd = open(argv[1], O_RDONLY)) < 0)
    {
        fprintf(stderr, "input exit with code %d", n);
        fflush(stderr);
        exit(1);
    }
    if ((output_fd = open(argv[2], O_CREAT | O_WRONLY | O_TRUNC, 0666)) < 0)
    {
        fprintf(stderr, "output exit with code %d", n);
        fflush(stderr);
        exit(1);
    }

    // read and write to multiple outputs
    while ((n = read(input_fd, temp, MAXCHARS)) > 0)
    {
        // check file hole
        int byte_start = -1;
        for (i = 0; i < n; i++)
        {
            if (temp[i] == 0)
            {
                // skip file holes
                continue;
            }
            
            if (byte_start == -1)
            {
                // start of bytes
                byte_start = i;
            }
            else if (temp[i + 1] != 0)
            {
                // middle of bytes
                continue;
            }
            else
            {
                // end of bytes
                lseek(output_fd, seek_index + byte_start, SEEK_SET);

                // write to stdout
                if (write(output_fd, temp + byte_start, i - byte_start) != i - byte_start)
                {
                    exit(1);
                }

                // reset index
                byte_start = -1;
            }
        }

        seek_index += n;
    }


    // failed to read
    if (n < 0)
    {
        fprintf(stderr, "final exit with code %d", n);
        fflush(stderr);
        exit(1);
    }

    close(input_fd);
    close(output_fd);
}