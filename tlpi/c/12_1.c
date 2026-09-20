/*
编写程序列出所有正在运行的程序与其命令
*/

#include <fcntl.h>
#include<sys/stat.h>
#include<ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <tlpi_hdr.h>


#define CHAR_MAX 4096

char *ltrim(char *s);
char *rtrim(char *s);
char *trim(char *s);

void print_process(char *file, int pid, int uid);


FILE *f;
char buf[CHAR_MAX] = {0};
char *char_p = NULL;
int i = 0;
char pname[CHAR_MAX];
Boolean is_my_process = FALSE;

int main(int argc, char *argv[]) {
    char file[CHAR_MAX] = {0};
    struct stat s = {0};

    if (argc != 2) {
        printf("usage: <user-name>\n");
        return 0;
    }

    struct passwd *pwd = getpwnam(argv[1]);
    int uid = pwd->pw_uid;

    printf("pid\tname\n");
    for(int i = 0; i < 65535; i++) {
        memset(file, 0, sizeof file);
        
        sprintf(file, "/proc/%d/status", i);
        
        // check if file exists
        if (stat(file, &s) < 0) {
            // process not exists, skip it.
            continue;
        }

        print_process(file, i, uid);
    }

    return 0;
}


void print_process(char *file, int pid, int uid) {    
    f = fopen(file, "r");
    if (f == NULL) {
        if (errno == ENOENT) {
            return;
        }
        errExit("fopen");
    }

    // printf("checking file: %s\n", file);

    is_my_process = FALSE;
    while (fgets(buf, sizeof buf, f)) {
        if (strncmp(buf, "Name:", 5) == 0) {
            strcpy(pname, trim(buf + 5));
        } else if (strncmp(buf, "Uid:", 4) == 0) {
            // trim left spaces
            char_p = ltrim(buf + 4);
            // replace the first space by \0
            for(i=0; (char_p[i] != ' ') && (char_p[i] != '\t'); i++);
            char_p[i] = 0;

            // convert the string before spaces into int
            // and compare the result with uid
            if (atoi(char_p) == uid) {
                is_my_process = TRUE;
            }
        }
    }

    if (is_my_process) {
        printf("%d\t%s\n", pid, pname);
    }

    fclose(f);
}

char *ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}

char *rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}

char *trim(char *s)
{
    return rtrim(ltrim(s)); 
}
