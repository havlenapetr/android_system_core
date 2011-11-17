/**
 *
 * Copyright (C) 1999,2000 by Lineo, inc.
 * Written by Erik Andersen <andersen@lineo.com>, <andersee@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int chroot_main(int argc, char **argv)
{
    char *prog;

    if ((argc < 2) || (**(argv + 1) == '-')) {
        fprintf(stderr, "Usage: chroot NEWROOT [COMMAND...]\n");
        return 10;
    }

    argc--;
    argv++;

    if (chroot(*argv) || (chdir("/"))) {
        fprintf(stderr, "Cannot change root directory to %s: %s\n", 
                *argv, strerror(errno));
        return -1;
    }

    argc--;
    argv++;
    if (argc >= 1) {
        prog = *argv;
        return execvp(*argv, argv);
    } else {
        prog = getenv("SHELL");
        if (!prog)
            prog = "/bin/sh";
        return execlp(prog, prog, NULL);
    }
    fprintf(stderr, "Cannot execute %s: %s\n", prog, strerror(errno));
    return -1;
}