/* pipe_test.c
 * James A. Shackleford <shack@drexel.edu>
 *
 * A simple tool for probing the file descriptor tables
 * of pipelined processes and testing for pipeline stalls.
 *
 * Will write results to a log file with the same name as
 * the executable with the appended file extension .log
 *
 * If executing this program multiple times in a pipeline,
 * be sure to give each program a unique name so that the
 * log files do not clobber each other.
 */

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/stat.h>

#include <ctype.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TIMEOUT 2     /* seconds */
#define DEFAULT_INPUT "hello world\n"

enum error {
    ERROR_NONE = 0,
    ERROR_FD0,
    ERROR_FD1,
    ERROR_SIGPIPE,
    ERROR_BAD_FD,
    ERROR_IO,
    ERROR_TIMEOUT
};

struct fd_entry {
    int fd;
    char *map;
};

struct fd_table {
    struct fd_entry *entry;
    int num_fd;
};


static jmp_buf env;


static void handler(int sig)
{
    switch (sig) {
    case SIGPIPE:
        siglongjmp(env, ERROR_SIGPIPE);
    case SIGALRM:
        siglongjmp(env, ERROR_TIMEOUT);
    }
}


static void table_use_pretty_names(struct fd_table *T)
{
    int i;
    char *off, *tmp;

    for (i=0; i<T->num_fd; i++) {
        /* call ttys and pts "Terminal" or "Keyboard" */
        if (isatty(T->entry[i].fd)) {
            free(T->entry[i].map);
            if (i == 0)
                T->entry[i].map = strdup("Keyboard");
            else
                T->entry[i].map = strdup("Terminal");
        }

        /* remove path prefix from file names */
        if ((off = strrchr(T->entry[i].map, '/'))) {
            tmp = strdup(++off);
            free(T->entry[i].map);
            T->entry[i].map = tmp;
        }
    }
}


static void table_create(struct fd_table *T)
{
    DIR *dirp;
    struct dirent *entry;
    char proc_dir[PATH_MAX];
    char proc_fn[PATH_MAX];
    char link[PATH_MAX];
    ssize_t len;
    int i = 0;

    sprintf(proc_dir, "/proc/%d/fd", getpid());
    dirp = opendir(proc_dir);

    while ((entry = readdir(dirp)))
        if (entry->d_type == DT_LNK)
            i++;

    T->num_fd = i > 0 ? i-1 : 0; /* don't include this probe in table! */
    T->entry = malloc(sizeof(*T->entry) * T->num_fd);
    rewinddir(dirp);

    i = 0;
    while ((entry = readdir(dirp)))
        if (entry->d_type == DT_LNK) {
            sprintf(proc_fn, "%s/%s", proc_dir, entry->d_name);
            len = readlink(proc_fn, link, PATH_MAX);
            link[len] = '\0';

            /* don't include this probe in table! */
            if (!strcmp(link, proc_dir))
                continue;

            T->entry[i].fd = atoi(entry->d_name);
            T->entry[i].map = strdup(link);

            i++;
        }

    closedir(dirp);

    table_use_pretty_names(T);
}


static void table_destroy(struct fd_table *T)
{
    int i;

    for (i=0; i<T->num_fd; i++)
        free(T->entry[i].map);

    T->num_fd = 0;
    free(T->entry);
}


static void table_fprint_num_fd(FILE *out, struct fd_table *T)
{
    if (T->num_fd > 3) {
        fprintf(out, "  - Inherited > 3 fd!\n");
    } else if (T->num_fd < 3) {
        fprintf(out, "  - Inherited < 3 fd!\n");
    }
}


static void table_fprint(FILE *out, struct fd_table *T, const char *name)
{
    int i;

    fprintf(out, "%-15s PID: %-10d \n", name, getpid());
    fprintf(out, "+------------------------------+\n");
    fprintf(out, "| File Descriptor Table        |\n");
    fprintf(out, "+---+--------------------------+\n");

    for (i=0; i<T->num_fd; i++)
        fprintf(out, "|%2d | %-25s|\n", T->entry[i].fd, T->entry[i].map);

    fprintf(out, "+---+--------------------------+\n");
}


static void vargs_fprint(FILE *out, int argc, char **argv)
{
    int i;

    fprintf(out, "\nVARGS: %d\n", argc);

    for (i=0; i<argc; i++)
        fprintf(out, "  [%d] %s\n", i, argv[i]);
}


static enum error do_IPC_pipeline_count(char *buf)
{
    ssize_t n;
    int num, err;
    struct stat sb;

    /* append process pipeline index to output */
    if (!isdigit(buf[0])) {
        n = write(STDOUT_FILENO, "\nProcess Count: 1", 17);
    } else {
        num = atoi(buf);
        num++;
        sprintf(buf, " %d", num);
        n = write(STDOUT_FILENO, buf, strlen(buf));
    }

    if (n < 0)
        switch (errno) {
        case EBADF:
        case EINVAL:
            return ERROR_BAD_FD;
        case EIO:
            return ERROR_IO;
        }
        
    fstat(STDOUT_FILENO, &sb);

    /* are we last in the pipeline? */
    if (!S_ISFIFO(sb.st_mode))
        write (STDOUT_FILENO, "\n", 1);

    return ERROR_NONE;
}


static enum error do_IPC()
{
    ssize_t n;
    int num;
    char buf[3] = { 0 };
    int did_read = 0;


    if (isatty(STDIN_FILENO)) {
        write(STDOUT_FILENO, DEFAULT_INPUT, strlen(DEFAULT_INPUT));
        did_read = 1; /* effectively */
    } else {
        while ((n = read(STDIN_FILENO, &buf[0], 1)) > 0) {
            did_read = 1;

            if (n != write(STDOUT_FILENO, &buf[0], n))
                return ERROR_FD1;
        }
    }

    if (!did_read)
        return ERROR_FD0;

    return do_IPC_pipeline_count(buf);
}


static void error_fprint(FILE *fp, enum error err)
{
    switch (err) {
    case ERROR_FD0:
        fprintf(fp, "  - Failed read from fd 0\n");
        break;
    case ERROR_FD1:
        fprintf(fp, "  - Failed write to fd 1\n");
        break;
    case ERROR_SIGPIPE:
        fprintf(fp, "  - Received SIGPIPE\n");
        break;
    case ERROR_BAD_FD:
        fprintf(fp, "  - Closed file descriptor\n");
        break;
    case ERROR_IO:
        /* really should not happen (!) */
        fprintf(fp, "  - Low-level I/O error\n"); 
        break;
    case ERROR_TIMEOUT:
        fprintf(fp, "  - Process Hung [killed]\n");
        break;
    default:
        break;
    }
}


static void set_signal_handlers()
{
    signal(SIGPIPE, handler);
    signal(SIGALRM, handler);
}


static FILE *open_log(const char *name)
{
    int fd;
    char fn[PATH_MAX];
    FILE *fp;

    sprintf(fn, "%s.log", name);
    fp = fopen(fn, "w");

    return fp;
}


int main(int argc, char **argv)
{
    struct fd_table T;
    FILE *fp;
    char buf[200];
    int err;


    table_create(&T);

    fp = open_log(argv[0]);

    table_fprint(fp, &T, argv[0]);
    vargs_fprint(fp, argc, argv);

    fprintf(fp, "\nERRORS:\n");
    table_fprint_num_fd(fp, &T);

    if ( (err = sigsetjmp(env, 1)) ) {
        error_fprint(fp, err);
        goto out;
    }

    set_signal_handlers();
    alarm(TIMEOUT);

    if ((err = do_IPC()))
        error_fprint(fp, err);

out:
    table_destroy(&T);
    fclose(fp);

    return 0;
}

