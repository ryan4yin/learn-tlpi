/*
编写程序列出所有正在运行的程序与其命令
*/

#include <fcntl.h>
#include<sys/stat.h>
#include<ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <malloc.h>
#include <tlpi_hdr.h>


#define BUF_SIZE 4096

struct proc_node {
    int pid;
    int ppid;
    char *name;
    char *args;

    struct proc_node *children[BUF_SIZE];
    int children_count;
};

char *ltrim(char *s);
char *rtrim(char *s);
char *trim(char *s);

int comp_proc_node (const void * elem1, const void * elem2);
struct proc_node *parse_process_info(int pid);
void print_process_tree(struct proc_node *head, int layer);


FILE *f;
struct stat s = {0};

char file[BUF_SIZE] = {0};
char buf[BUF_SIZE] = {0};
struct proc_node *processes[BUF_SIZE] = {0};


int main(int argc, char *argv[]) {
    int p_count = 0;
    for(int i = 0; i < 65535; i++) {
        struct proc_node *p = parse_process_info(i);
        if (p == NULL) {
            continue;
        }

        processes[p_count++] = p;
        if (p_count >= BUF_SIZE) {
            errExit("process count exceeded limit!");
        }
    }

    // sort by ppid
    qsort(processes, p_count, sizeof(struct proc_node *), comp_proc_node);

    // determine the process tree
    int i, j;
    for(i = 0; i < p_count - 1; i++) {
        for(j = p_count - 1; j > 0; j--) {
            // printf("i %d j %d, ppid %d pid %d\n", i, j, processes[j]->ppid, processes[i]->pid);
            if (processes[j]->ppid == processes[i]->pid){
                processes[i]->children[processes[i]->children_count] = processes[j];
                processes[i]->children_count++;
            }
        }
    }

    // print the processes tree
    print_process_tree(processes[0], 0);
    return 0;
}


struct proc_node *parse_process_info(int pid) { 
    char *char_p = NULL;

    // 1. read process's status  
    sprintf(file, "/proc/%d/status", pid);

    // check if file exists
    if (stat(file, &s) < 0) {
        // process not exists, skip it.
        return NULL;
    }

    f = fopen(file, "r");
    if (f == NULL) {
        if (errno == ENOENT) {
            return NULL;
        }
        errExit("fopen");
    }

    struct proc_node *p = malloc(sizeof(struct proc_node));
    memset(p, 0, sizeof *p);
    p->pid = pid;

    while (fgets(buf, sizeof buf, f)) {
        if (strncmp(buf, "Name:", 5) == 0) {
            char_p = trim(buf + 5);
            p->name = malloc(strlen(char_p) + 1);
            memset(p->name, 0, strlen(p->name) + 1);
            strcpy(p->name, char_p);
        } else if (strncmp(buf, "PPid:", 5) == 0) {
            char_p = trim(buf + 5);
            p->ppid = atoi(char_p);
        }
    }
    fclose(f);

    // read process's cmdline
    memset(buf, 0, BUF_SIZE);
    sprintf(file, "/proc/%d/cmdline", pid);

    f = fopen(file, "r");
    if (f == NULL) {
        if (errno == ENOENT) {
            return NULL;
        }
        errExit("fopen");
    }

    int nbytesread = fread(buf, BUF_SIZE, BUF_SIZE, f);
    fclose(f);

    char *end = buf + nbytesread;
    for (char *p = buf; p < end; /**/)
    {
        while (*p++); // skip until start of next 0-terminated section
        *p = ' '; // replace delimeter \0 with ' '
    }

    p->args = malloc(strlen(buf) + 1);
    memset(p->args, 0, strlen(buf) + 1);
    strcpy(p->args, buf);

    return p;
}

int comp_proc_node (const void * elem1, const void * elem2) 
{
    struct proc_node *first = ((struct proc_node*)elem1);
    struct proc_node *second = ((struct proc_node*)elem2);
    if (first->ppid > second->ppid) return  1;
    if (first->ppid < second->ppid) return -1;
    return 0;
}

void print_process_tree(struct proc_node *head, int layer) {
    // print layer prefix
    for(int i = 0; i < layer; i++) {
        if (i == layer -1) {
            printf("├");
        } else {
            printf("\t");
        }
    }

    // print process
    printf("[%d]%s\n", head->pid, head->args);

    // print its children processes
    layer += 1;
    for(int i = 0; i < head->children_count; i++) {
        print_process_tree(head->children[i], layer);
    }
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
