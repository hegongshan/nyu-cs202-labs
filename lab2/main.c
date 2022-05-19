#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/param.h>

static int err_code;

/*
 * here are some function signatures and macros that may be helpful.
 */

void handle_error(char* fullname, char* action);
bool test_file(char* pathandname);
bool is_dir(char* pathandname);
const char* ftype_to_str(mode_t mode);
void list_file(char* pathandname, char* name, bool list_long, bool human_readable);
void list_dir(char* dirname, bool list_long, bool list_all, bool recursive, bool human_readable);

/*
 * You can use the NOT_YET_IMPLEMENTED macro to error out when you reach parts
 * of the code you have not yet finished implementing.
 */
#define NOT_YET_IMPLEMENTED(msg)                  \
    do {                                          \
        printf("Not yet implemented: " msg "\n"); \
        exit(255);                                \
    } while (0)

/*
 * PRINT_ERROR: This can be used to print the cause of an error returned by a
 * system call. It can help with debugging and reporting error causes to
 * the user. Example usage:
 *     if ( error_condition ) {
 *        PRINT_ERROR();
 *     }
 */
#define PRINT_ERROR(progname, what_happened, pathandname)               \
    do {                                                                \
        printf("%s: %s %s: %s\n", progname, what_happened, pathandname, \
               strerror(errno));                                        \
    } while (0)

/* PRINT_PERM_CHAR:
 *
 * This will be useful for -l permission printing.  It prints the given
 * 'ch' if the permission exists, or "-" otherwise.
 * Example usage:
 *     PRINT_PERM_CHAR(sb.st_mode, S_IRUSR, "r");
 */
#define PRINT_PERM_CHAR(mode, mask, ch) printf("%s", (mode & mask) ? ch : "-");

/*
 * Get username for uid. Return 1 on failure, 0 otherwise.
 */
static int uname_for_uid(uid_t uid, char* buf, size_t buflen) {
    struct passwd* p = getpwuid(uid);
    if (p == NULL) {
        return 1;
    }
    strncpy(buf, p->pw_name, buflen);
    return 0;
}

/*
 * Get group name for gid. Return 1 on failure, 0 otherwise.
 */
static int group_for_gid(gid_t gid, char* buf, size_t buflen) {
    struct group* g = getgrgid(gid);
    if (g == NULL) {
        return 1;
    }
    strncpy(buf, g->gr_name, buflen);
    return 0;
}

/*
 * Format the supplied `struct timespec` in `ts` (e.g., from `stat.st_mtim`) as a
 * string in `char *out`. Returns the length of the formatted string (see, `man
 * 3 strftime`).
 */
static size_t date_string(struct timespec* ts, char* out, size_t len) {
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    struct tm* t = localtime(&ts->tv_sec);
    if (now.tv_sec < ts->tv_sec) {
        // Future time, treat with care.
        return strftime(out, len, "%b %e %Y", t);
    } else {
        time_t difference = now.tv_sec - ts->tv_sec;
        if (difference < 31556952ull) {
            return strftime(out, len, "%b %e %H:%M", t);
        } else {
            return strftime(out, len, "%b %e %Y", t);
        }
    }
}

/*
 * Print help message and exit.
 */
static void help() {
    /* TODO: add to this */
    printf("ls: List files\n");
    printf("\t--help: Print this help\n");
    printf("\t-l: Long format\n");
    printf("\t-R: recursive\n");
    exit(0);
}

/*
 * call this when there's been an error.
 * The function should:
 * - print a suitable error message (this is already implemented)
 * - set appropriate bits in err_code
 */
void handle_error(char* what_happened, char* fullname) {
    PRINT_ERROR("ls", what_happened, fullname);

    // TODO: your code here: inspect errno and set err_code accordingly.
    err_code = -errno;
    return;
}

/*
 * test_file():
 * test whether stat() returns successfully and if not, handle error.
 * Use this to test for whether a file or dir exists
 */
bool test_file(char* pathandname) {
    struct stat sb;
    if (stat(pathandname, &sb)) {
        handle_error("cannot access", pathandname);
        return false;
    }
    return true;
}

/*
 * is_dir(): tests whether the argument refers to a directory.
 * precondition: test_file() returns true. that is, call this function
 * only if test_file(pathandname) returned true.
 */
bool is_dir(char* pathandname) {
    /* TODO: fillin */
    struct stat sb;
    if (stat(pathandname, &sb)) {
        handle_error("cannot access", pathandname);
        return false;
    }
    return S_ISDIR(sb.st_mode);
}

/* convert the mode field in a struct stat to a file type, for -l printing */
const char* ftype_to_str(mode_t mode) {
    /* TODO: fillin */
    if (S_ISREG(mode)) {
        return "-";
    }
    if (S_ISDIR(mode)) {
        return "d";
    }
    if (S_ISLNK(mode)) {
        return "l";
    }
    /*if (S_ISCHR(mode)) {
        return "c";
    }
    if (S_ISBLK(mode)) {
        return  "b";
    }
    if (S_ISFIFO(mode)) {
        return "f";
    }
    if (S_ISSOCK(mode)) {
        return "s";
    }*/
    return "?";
}

/* list_file():
 * implement the logic for listing a single file.
 * This function takes:
 *   - pathandname: the directory name plus the file name.
 *   - name: just the name "component".
 *   - list_long: a flag indicated whether the printout should be in
 *   long mode.
 *
 *   The reason for this signature is convenience: some of the file-outputting
 *   logic requires the full pathandname (specifically, testing for a directory
 *   so you can print a '/' and outputting in long mode), and some of it
 *   requires only the 'name' part. So we pass in both. An alternative
 *   implementation would pass in pathandname and parse out 'name'.
 */
void list_file(char* pathandname, char* name, bool list_long, bool human_readable) {
    /* TODO: fill in*/
    if (!list_long) {
        printf("%s", name);
        if (is_dir(pathandname) && strcmp(name, ".") && strcmp(name, "..")) {
            putchar('/');
        }
        putchar('\n');
        return;
    }

    struct stat sb;
    // Don't use stat function
    if (lstat(pathandname, &sb)) {
        handle_error("cannot access", pathandname);
        return;
    }

    // 1.file mode
    printf("%s", ftype_to_str(sb.st_mode));
    PRINT_PERM_CHAR(sb.st_mode, S_IRUSR, "r");
    PRINT_PERM_CHAR(sb.st_mode, S_IWUSR, "w");
    PRINT_PERM_CHAR(sb.st_mode, S_IXUSR, "x");
    PRINT_PERM_CHAR(sb.st_mode, S_IRGRP, "r");
    PRINT_PERM_CHAR(sb.st_mode, S_IWGRP, "w");
    PRINT_PERM_CHAR(sb.st_mode, S_IXGRP, "x");
    PRINT_PERM_CHAR(sb.st_mode, S_IROTH, "r");
    PRINT_PERM_CHAR(sb.st_mode, S_IWOTH, "w");
    PRINT_PERM_CHAR(sb.st_mode, S_IXOTH, "x");
    putchar(' ');

    // 2.number of links, 
    printf("%lu ", sb.st_nlink);

    // 3.owner name and group name
    char uname[255];
    char group[255];
    uname_for_uid(sb.st_uid, uname, sizeof(uname));
    group_for_gid(sb.st_gid, group, sizeof(group));
    printf("%s %s ",uname, group);

    // 4.file size
    if (human_readable) {
        char unit[] = {'K', 'M', 'G'};
        float base[] = {1024.0, 1024.0 * 1024, 1024.0 * 1024 * 1024};
        int i = sizeof(base) / sizeof(float) - 1;
        while (i >= 0 && base[i] > sb.st_size) {
            i--;
        }
        if (i < 0) {
            printf("%7ld ", sb.st_size);
        } else {
            printf("%6.1f%c ", sb.st_size / base[i], unit[i]);
        }
    } else {
        printf("%ld ", sb.st_size);
    }

    // 5.modify time
    char date_str[255];
    date_string(&sb.st_mtim, date_str, sizeof(date_str));
    printf("%s ", date_str);

    // 6.pathname
    printf("%s", name);
    if (is_dir(pathandname)) {
        putchar('/');
    }

    // handle symbolic link
    if (S_ISLNK(sb.st_mode)) {
        char link[MAXPATHLEN];
        if (readlink(pathandname, link, sizeof(link)) == -1) {
            handle_error("cannot handle", pathandname);
        }
        printf(" -> %s", link);
        if (is_dir(pathandname)) {
            putchar('/');
        }
    }
    putchar('\n');
}

/* list_dir():
 * implement the logic for listing a directory.
 * This function takes:
 *    - dirname: the name of the directory
 *    - list_long: should the directory be listed in long mode?
 *    - list_all: are we in "-a" mode?
 *    - recursive: are we supposed to list sub-directories?
 */
void list_dir(char* dirname, bool list_long, bool list_all, bool recursive, bool human_readable) {
    /* TODO: fill in
     *   You'll probably want to make use of:
     *       opendir()
     *       readdir()
     *       list_file()
     *       snprintf() [to make the 'pathandname' argument to
     *          list_file(). that requires concatenating 'dirname' and
     *          the 'd_name' portion of the dirents]
     *       closedir()
     *   See the lab description for further hints
     */
    DIR *dirp = opendir(dirname);
    
    struct dirent *dent;
    while((dent = readdir(dirp))) {

        // ignore the dot files if list_all is false
        if (!list_all && dent->d_name[0] == '.') {
            continue;
        }

        const int len = strlen(dirname) + 1 + strlen(dent->d_name) + 1;
        char pathandname[len]; 

        snprintf(pathandname, len, "%s/%s", dirname, dent->d_name);
        list_file(pathandname, dent->d_name, list_long, human_readable);

        // list sub-directories
        if (is_dir(pathandname) && recursive) {
            list_dir(pathandname, list_long, list_all, recursive, human_readable);
        }
    }

    closedir(dirp);
}

int main(int argc, char* argv[]) {
    // This needs to be int since C does not specify whether char is signed or
    // unsigned.
    int opt;
    err_code = 0;
    bool list_long = false, list_all = false;
    bool recursive = false;
    bool human_readable = false;
    // We make use of getopt_long for argument parsing, and this
    // (single-element) array is used as input to that function. The `struct
    // option` helps us parse arguments of the form `--FOO`. Refer to `man 3
    // getopt_long` for more information.
    struct option opts[] = {
        {.name = "help", .has_arg = 0, .flag = NULL, .val = '\a'}};

    // This loop is used for argument parsing. Refer to `man 3 getopt_long` to
    // better understand what is going on here.
    while ((opt = getopt_long(argc, argv, "1alRh", opts, NULL)) != -1) {
        switch (opt) {
            case '\a':
                // Handle the case that the user passed in `--help`. (In the
                // long argument array above, we used '\a' to indicate this
                // case.)
                help();
                break;
            case '1':
                // Safe to ignore since this is default behavior for our version
                // of ls.
                break;
            case 'a':
                list_all = true;
                break;
                // TODO: you will need to add items here to handle the
                // cases that the user enters "-l" or "-R"
            case 'l':
                list_long = true;
                break;
            case 'R':
                recursive = true;
                break;
            case 'h':
                human_readable = true;
                break;
            default:
                printf("Unimplemented flag %d\n", opt);
                break;
        }
    }

    // TODO: Replace this.
    /*if (optind < argc) {
        printf("Optional arguments: ");
    }
    for (int i = optind; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    if (optind < argc) {
        printf("\n");
    }

    NOT_YET_IMPLEMENTED("Listing files");*/
    if (optind == argc) {
        list_dir(".", list_long, list_all, recursive, human_readable);
    } else {
        for (int i = optind; i < argc; i++) {
            list_dir(argv[i], list_long, list_all, recursive, human_readable);
        }
    }
    exit(err_code);
}
