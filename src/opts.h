/***************************************************************************
 *   Copyright (C) 2014 by Michael Ambrus                                  *
 *   ambrmi09@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* Options and opts */

#ifndef opts_h
#define opts_h
#include <limits.h>
#include <sys/types.h>

#include <log.h>
#include "config.h"

#define HELP_USAGE     1
#define HELP_LONG      2
#define HELP_VERSION   4
#define HELP_TRY       8
#define HELP_EXIT     16
#define HELP_EXIT_ERR 32

#define OPT_OK         0
#define E_OPT_REQ     -1        /* User breaks option requirement rule */
#define E_OPT_USAGE   -2        /* User breaks usage of option  */

#ifndef DEF_PTIME
#define DEF_PTIME           1000
#endif
#ifndef DEF_DEBUGFS_PATH
#define DEF_DEBUGFS_PATH    "/sys/kernel/debug"
#endif
#ifndef DEF_WORKDIR
#define DEF_WORKDIR         "./"
#endif

#define xstr(S) str(S)
#define str(S) #S

/* opt validator, one per opt */
struct req_opt {
    int val;                    /* Flag value (option letter) */
    int req;                    /* Times required to be seen at least */
    int max;                    /* Seen no more than */
    int cnt;                    /* Seen number of times */
};

/* General opts */
struct opts {
    log_level *loglevel;        /* Verbosity level */
    int daemon;                 /* If to become a daemon or not */

    struct req_opt *req_opts;   /* Deep copy of the req_opts list. Can be used to
                                   extend logic. */
};

#include <stdio.h>
void opts_init();
void opts_help(FILE *file, int flags);
int opts_parse(int argc, char **argv, struct opts *);
int opts_check(struct opts *);
struct req_opt *req_opt(int val, struct req_opt *rop);

#endif                          //opts_h
