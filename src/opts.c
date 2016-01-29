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
/*
 * Parse single options for template
 */
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "assure.h"
#include <log.h>
#include "opts.h"
#include "doc.h"
#include "main.h"

#define not_req    0
#define mandatory  1
#define at_least   0
#define precisely -1

static struct req_opt *_req_opt(int val);

extern struct template etrace;

/* Parse a single option. */
static int opts_parse_opt(const char *cmd,
                          int key, char *arg, struct opts *opts)
{

    switch (key) {
        case 'v':
            _req_opt('v')->cnt++;
            if (arg[0] >= '0' && arg[0] <= '9')
                *opts->loglevel = arg ? atoi(arg) : 0;
            else {
                int ok;
                *opts->loglevel = str2loglevel(arg, &ok);
                if (!ok)
                    LOGW("loglevel [%s] invalid. Falling back to default\n",
                         arg);
            }
            break;
        case 'z':
            _req_opt('z')->cnt++;
            opts->daemon = 1;
            break;
        case 'u':
            _req_opt('u')->cnt++;
            opts_help(stdout, HELP_USAGE | HELP_EXIT);
            break;
        case 'h':
            _req_opt('h')->cnt++;
            opts_help(stdout, HELP_LONG | HELP_EXIT);
            break;
        case 'D':
            _req_opt('D')->cnt++;
            doc_print();
            template_exit(0);
            break;
        case '?':
            /* getopt_long already printed an error message. */
            opts_help(stderr, HELP_TRY | HELP_EXIT_ERR);
            break;
        case ':':
            /* getopt_long already printed an error message. */
            fprintf(stderr, "%s: option `-%c' requires an argument\n",
                    cmd, optopt);
            opts_help(stderr, HELP_TRY | HELP_EXIT_ERR);
            break;
        case 'V':
            _req_opt('V')->cnt++;
            opts_help(stdout, HELP_VERSION | HELP_EXIT);
            break;
        default:
            fprintf(stderr, "template: unrecognized option '-%c'\n", (char)key);
            opts_help(stderr, HELP_TRY | HELP_EXIT_ERR);
            break;
    }
    return OPT_OK;
}

static struct option long_options[] = {
/* *INDENT-OFF* */
    {"verbosity",      required_argument,  0,  'v'},

    {"daemon",         no_argument,        0,  'z'},
    {"documentation",  no_argument,        0,  'D'},
    {"help",           no_argument,        0,  'h'},
    {"usage",          no_argument,        0,  'u'},
    {"version",        no_argument,        0,  'V'},
    {0,                0,                  0,  0}
/* *INDENT-ON* */
};

/* Additional structure to keep track of mandatory options. Each entry-index
 * should correspond exactly to long_options. Therefore keep tables close.
 */
static struct req_opt req_opts[] = {
/* *INDENT-OFF* */
    {'v',  not_req,    at_least,   0},

    {'z',  not_req,    precisely,  0},
    {'D',  not_req,    at_least,   0},
    {'h',  not_req,    at_least,   0},
    {'u',  not_req,    at_least,   0},
    {'V',  not_req,    at_least,   0},
    {0,    0,          0,          0}
/* *INDENT-ON* */
};

/* Access by key to the table above */
struct req_opt *_req_opt(int val)
{
    struct req_opt *rop = req_opts;

    for (rop = req_opts; rop->val != 0; rop++) {
        if (rop->val == val)
            return rop;
    }
    assert("req_opt reached sentinel" == 0);
    return rop;
}

/* Access by key to a req_opt-list, generalized form */
struct req_opt *req_opt(int val, struct req_opt *rop)
{
    for (rop = req_opts; rop->val != 0; rop++) {
        if (rop->val == val)
            return rop;
    }
    assert("req_opt reached sentinel" == 0);
    return rop;
}

/* Returns 0 on success, -1 on error */
static int become_daemon()
{
    switch (fork()) {
        case -1:
            return -1;
        case 0:
            break;
        default:
            _exit(EXIT_SUCCESS);
    }

    if (setsid() == -1)
        return -1;

    switch (fork()) {           /* Ensure we are not session leader */
        case -1:
            return -1;
        case 0:
            break;
        default:
            _exit(EXIT_SUCCESS);
    }

    umask(0);                   /* Clear file mode creation mask */
    chdir("/");                 /* Change to root directory */

    return 0;
}

/*Initializes (resets) whatever is state-dependent here */
void opts_init()
{
    struct req_opt *rop = req_opts;
    struct option *op = long_options;

    for (rop = req_opts, op = long_options; op->name != NULL; rop++, op++) {
        rop->cnt = 0;
    }
}

/* Checks options to fulfill extended criteria. If successful appends a
 * deep-copy of the validation-structure. */
int opts_check(struct opts *opts)
{
    int resok = OPT_OK;
    struct option *op = long_options;
    struct req_opt *rop = req_opts;

    LOGD("Checking if mandatory options are set\n");

    for (rop = req_opts, op = long_options; op->name != NULL; rop++, op++) {
        assert(op->val == rop->val);
        if (op->flag)
            LOGV("%-15s %d %4d %c %2d %2d %2d\n",
                 op->name, op->has_arg, &op->flag, op->val,
                 rop->req, rop->cnt, rop->max);
        else
            LOGV("%-15s %d NULL %c %2d %2d %2d\n",
                 op->name, op->has_arg, op->val, rop->req, rop->cnt, rop->max);

        if (rop->cnt < rop->req) {
            LOGE("Mandatory option [\"%s\",'%c'] requirement failed. "
                 "Seen [%d] times, required [%d]",
                 op->name, op->val, rop->cnt, rop->req, rop->max);
            resok = E_OPT_REQ;
        }
        if (rop->max > 0 && rop->cnt > rop->max) {
            LOGE("Count of option [\"%s\",'%c'] requirement failed. "
                 "Seen [%d] times, permitted [%d]",
                 op->name, op->val, rop->cnt, rop->req, rop->max);
            resok = E_OPT_REQ;
        }
        if (rop->max == precisely && rop->cnt != rop->max && rop->req > 0) {
            LOGE("Count of option [\"%s\",'%c'] requirement failed. "
                 "Seen [%d] times, expected [%d]",
                 op->name, op->val, rop->cnt, rop->req, rop->max);
            resok = E_OPT_REQ;
        }
    }

    /* If all passed OK, make a deep-copy of check-struct  and attach it to
     * the opts-struct */
    {
        void *p;

        opts->req_opts =
            ((p =
              malloc(sizeof(req_opts))) ? memcpy(p, &req_opts,
                                                 sizeof(req_opts)) : NULL);
    }

    return resok;
}

/* Parse options
 *
 * Returns number of options parsed.
 * Negative return-value indicate error.
 *
 * */
int opts_parse(int argc, char **argv, struct opts *opts)
{
    int rc, parsed_options = 0;

    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv,
                            "v:zDuhV",
                            long_options,
                            &option_index);
        /* Detect the end of the options. */
        if (c == -1)
            break;
        ASSURE_E((rc =
                  opts_parse_opt(argv[0], c, optarg, opts)) == OPT_OK,
                 return rc);
        parsed_options++;
    }

    /* Handle any remaining command line arguments (not options). */
    if (optind < argc) {
        perror("template: Too many arguments, \"template\" takes only options.\n");
        fflush(stderr);
        opts_help(stderr, HELP_TRY | HELP_EXIT_ERR);
		return -1;
    }

    if (opts->daemon) {
        become_daemon();
    }
    return parsed_options;
}
