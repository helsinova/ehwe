/***************************************************************************
 *   Copyright (C) 2015 by Michael Ambrus                                  *
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
#include <stdio.h>
#include <config.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

//#undef NDEBUG
#define NDEBUG
#include <assert.h>
#include "assure.h"
#include <log.h>
#include <mtime.h>
#include "opts.h"
#include "main.h"
#include <mlist.h>

extern log_level log_filter_level;

struct opts opts = {
/* *INDENT-OFF* */
    .loglevel       = &log_filter_level,
    .daemon         = 0
/* *INDENT-ON* */
};

struct template template = {
/* *INDENT-OFF* */
    .opts           = &opts,
    .list           = 1
/* *INDENT-ON* */
};

/* Cleanup before exit even if exit-with-error */
void template_exit(int status)
{
    LOGD("template_exit initiated\n");

    free(opts.req_opts);
    exit(status);
}

int main(int argc, char **argv)
{
    int rc;
    LOGI("\"template\" version v%s \n", VERSION);

    opts_init();
    /* Any zeroing here (begin) */
    /* Any zeroing here (done) */

    ASSURE_E((rc = opts_parse(argc, argv, &opts)) >= 0, goto err);
    LOGI("Parsed %d options.\n", rc);
    ASSURE_E(opts_check(&opts) == OPT_OK, goto err);
    LOGI("Option passed rule-check OK\n", rc);

    template_exit(0);
err:
    template_exit(1);
    /* GCC, please shut up! */
    return 0;
}
