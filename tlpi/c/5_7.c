#include <sys/stat.h>
#include <fcntl.h>
#include <tlpi_hdr.h>
// #include<sys/uio.h>


struct my_iovec
{
    void *iov_base;	/* Pointer to data.  */
    size_t iov_len;	/* Length of data.  */
};


ssize_t my_readv (int __fd, const struct my_iovec *__iovec, int __count){
    ssize_t numRead = 0;
    ssize_t buffer_remain = 0;
    ssize_t numReadTmp = 0;
    for (int i = 0; i < __count; i++){
        buffer_remain = __iovec[i].iov_len;
        while ((numReadTmp = read(__fd, __iovec[i].iov_base, buffer_remain)) > 0) {
            printf("n: %ld\n", numReadTmp);
            numRead += numReadTmp;
            buffer_remain -= numReadTmp;
            if (buffer_remain <= 0) {
                break;
            }
        }

        if (numRead == -1) {
            errExit("failed to read");
        }
        
        if (numRead == 0) {
            return numRead;
        }
    }

    return numRead;
}

ssize_t my_writev (int __fd, const struct my_iovec *__iovec, int __count){
    ssize_t numWrite = 0;
    ssize_t numWriteTmp = 0;
    for (int i = 0; i < __count; i++){
        if((numWriteTmp = write(__fd, __iovec[i].iov_base, __iovec[i].iov_len)) == -1) {
            err_exit("failed to write");
        }

        numWrite += numWriteTmp;

        if (numWriteTmp < __iovec[i].iov_len){
            return numWrite;
        }
    }

    return numWrite;
}



int main(int argc, char *argv[]) {
    int fd;
    char s1[] = "It's a nice day!";
    char s2[] = "Yes, Really a nice day.";
    char s3[] = "Did you see my cat? I can't find my little cat now!";
    struct my_iovec *iovec_arr;

    iovec_arr = (struct my_iovec *)calloc(3, sizeof(struct my_iovec));
    iovec_arr[0].iov_base = s1;
    iovec_arr[0].iov_len = sizeof s1;
    iovec_arr[1].iov_base = s2;
    iovec_arr[1].iov_len = sizeof s2;
    iovec_arr[2].iov_base = s3;
    iovec_arr[2].iov_len = sizeof s3;
    
    fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        errExit("open failed");
    }

    if (my_writev(fd, iovec_arr, 3) <= 0) {
        errExit("failed to writev.");
    }

    for (int i = 0; i < 3; i++){
        memset(iovec_arr[i].iov_base, 0, iovec_arr[i].iov_len);
    }

    for (int i = 0; i < 3; i++){
        printf("iovec[%d] before read:\n", i);
        printf("%s\n", iovec_arr[i]);
        printf("=============================\n");
    }

    lseek(fd, 0, SEEK_SET);
    if (my_readv(fd, iovec_arr, 3) <= 0) {
        errExit("failed to readv.");
    }

    for (int i = 0; i < 3; i++){
        printf("=============================\n");
        printf("iovec[%d] after read:\n", i);
        printf("%s\n", iovec_arr[i]);
        printf("=============================\n");
    }

    free(iovec_arr);
    return 0;
}

