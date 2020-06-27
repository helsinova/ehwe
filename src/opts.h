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
#include <mlist.h>
#include "config.h"
#include "main.h"

#define HELP_USAGE     1
#define HELP_LONG      2
#define HELP_VERSION   4
#define HELP_TRY       8
#define HELP_EXIT     16
#define HELP_EXIT_ERR 32

#define OPT_OK         0
#define E_OPT_REQ     -1        /* User breaks option requirement rule */
#define E_OPT_USAGE   -2        /* User breaks usage of option  */

#define xstr(S) str(S)
#define str(S) #S

/* opt presence validation rules, one per opt. Useful one ore more options
 * are not optional but when we don't want to treat them as arguments (i.e.
 * we want to benefit from the rules offered by option parsing).
 *
 * The presence of each opt, identified by it's short-opt identifier <opt>,
 * is checked against <req> and <max>. <cnt> is just an internal counter,
 * but needs to be initialized with the value 0 if <req> and <max> should
 * retain the meaning above.
 *
 * <req> and <max> have, besides holding min and max number of visibility
 * times, the following extended meaning:
 *
 * <req> can have the special values 0 and 1, meaning not required and
 * required. Convenience names are defined accordingly: "not_req" and
 * "mandatory"
 *
 * <max> is used to further refine the rule when <req> is "mandatory" with
 * the special values 0 and -1. If 0, there is no upper limit. If -1, it
 * means option must have been seen exactly <req> (i.e. minimum) times.
 * Convenience names are defined accordingly: "at_least" and "precisely".
*/
struct req_opt {
    int val;                    /* Flag value (i.e. short-option letter) */
    int req;                    /* Required to be present at least <req> times */
    int max;                    /* Presence must not exceed <max> times */
    int cnt;                    /* Counter which counts presence. Should be
                                   initialized with 0 */
};

/* Options struct used while command-line options are parsed. */
struct opts {
    log_level *loglevel;        /* Verbosity level */
    int daemon;                 /* If to become a daemon or not */
    handle_t adapter_strs;          /* Adapter specifications list. */

    struct req_opt *req_opts;   /* Deep copy of the req_opts list. Used to
                                   extend logic with presence validation. */
};

#include <stdio.h>
void opts_init();
void opts_help(FILE *file, int flags);
int opts_parse(int *argc, char ***argv, struct opts *);
int opts_check(struct opts *);
struct req_opt *req_opt(int val, struct req_opt *rop);

#endif                          //opts_h
