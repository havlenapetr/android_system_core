#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/klog.h>
#include <string.h>

#define KLOG_BUF_SHIFT	17	/* CONFIG_LOG_BUF_SHIFT from our kernel */
#define KLOG_BUF_LEN	(1 << KLOG_BUF_SHIFT)

#define LOG_START "- - - - - - - - - - - - - - - - -\n"
#define LOG_START_LENGTH 34

int fd;

static void dmesg_close() {
    if (fd != STDOUT_FILENO && fd != 0) {
        close(fd);
        fd = 0;
    }
}

static void dmesg_os_handler(int signum, siginfo_t *info, void *ptr) {
    //printf("Received signal %d\n", signum);
    //printf("Signal originates from process %lu\n",
    //       (unsigned long)info->si_pid);
    dmesg_close();
}

static int dmesg_is_file_exist(char *file) {
    int f = open(file, O_WRONLY);
    if (f != 0) {
        close(f);
        return 1;
    }
    return -1;
}

int dmesg_main(int argc, char **argv)
{
    char buffer[KLOG_BUF_LEN + 1];
    struct sigaction act;
    char *p = buffer;
    ssize_t ret;
    int n, op;

    do {
        if (argc > 1 && !strcmp(*argv, "-c")) {
            op = KLOG_READ_CLEAR;
        } else if (argc > 1 && !strcmp(*argv, "-d")) {
            op = KLOG_READ;
        } else if (argc > 1 && !strcmp(*argv, "-f")) {
            *argv++;
            fd = open(*argv, O_WRONLY|O_CREAT|O_APPEND);
            write(fd, LOG_START, LOG_START_LENGTH);
        } else {
            op = KLOG_READ_ALL;
            fd = 0;
        }
        *argv++;
    } while (*argv);

    // init os signal handler
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = dmesg_os_handler;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGKILL, &act, NULL);

    do {
        memset(&buffer, 0, KLOG_BUF_LEN + 1);
        p = buffer;
        n = klogctl(op, buffer, KLOG_BUF_LEN);
        if (n < 0) {
            perror("klogctl");
            return EXIT_FAILURE;
        }
        buffer[n] = '\0';

        if (fd == 0) {
            fd = STDOUT_FILENO;
        }

        while((ret = write(fd, p, n))) {
            if (ret == -1) {
	            if (errno == EINTR)
                    continue;
	            perror("write");
	            return EXIT_FAILURE;
            }
            p += ret;
            n -= ret;
        }
    } while (op == KLOG_READ);

    dmesg_close();
    return 0;
}
