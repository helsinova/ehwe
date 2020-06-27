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
#include <adapters.h>
#include <apis.h>
#include <stdlib.h>

extern log_level log_filter_level;

struct opts opts = {
/* *INDENT-OFF* */
    .loglevel       = &log_filter_level,
    .daemon         = 0
/* *INDENT-ON* */
};

/* Static initialization (fields possible to initialize) */
struct ehwe ehwe = {
/* *INDENT-OFF* */
    .opts           = &opts
/* *INDENT-ON* */
};

/* Cleanup before exit even if exit-with-error */
void ehwe_exit(int status)
{
    LOGD("ehwe_exit initiated\n");

    free(opts.req_opts);
    exit(status);
}

int main(int argc, char **argv)
{
    int rc, new_argc = argc;
    char **new_argv = argv;
    LOGI("\"ehwe\" version v%s \n", VERSION);

    /* Storage for adapter-specification strings */
    ASSURE((rc = mlist_opencreate(sizeof(char *), NULL, &opts.adapter_strs)) == 0);

    opts_init();
    /* /begin/ zeroing of opt vars */
    /* /end/   zeroing of opt vars */

    ASSURE_E((rc = opts_parse(&new_argc, &new_argv, &opts)) >= 0, goto err);
    LOGI("Parsed %d options.\n", rc);
    ASSURE_E(opts_check(&opts) == OPT_OK, goto err);
    LOGI("Option passed rule-check OK\n", rc);

    /* Storage for adapter-specification strings */
    ASSURE((rc =
            mlist_opencreate(sizeof(struct adapter), NULL, &ehwe.adapters)) == 0);

    /* Convert adapter strings to adapters */
    LOGD("List of adapter definition strings:\n");
#undef LDATA
#define LDATA char *
    ITERATE(opts.adapter_strs) {
        struct adapter adapter;

        LOGD("  %s\n", CDATA(opts.adapter_strs));
        if (adapters_parse(CDATA(opts.adapter_strs), &adapter) != 0) {
            LOGE("Bad adapter option (-d): [%s] \n", CDATA(opts.adapter_strs));
            mlist_close(ehwe.adapters);
            goto err;
        }
        /* OK to add stack-variable as deep-copy to new location occurs. */
        ASSURE(mlist_add_last(ehwe.adapters, &adapter));
    }
#undef LDATA

    /* Close storage of adapter-specification strings */
    //ASSURE((rc = mlist_close(opts.adapter_strs)) == 0);

    LOGD("Initialing adapters:\n");
#undef LDATA
#define LDATA struct adapter
    ITERATE(ehwe.adapters) {
        LOGD("  %d\n", CDATA(ehwe.adapters).devid);
        ASSURE_E((rc =
                  adapters_init_adapter(CREF(ehwe.adapters))) == 0, goto err2);;
        ASSURE_E((rc =
                  apis_init_api(CREF(ehwe.adapters))) == 0,
                 goto err2);
    }
#undef LDATA

    /* Call the workbench */
    LOGI("Executing workbench\n");
    rc = embedded_main(new_argc, new_argv);
    LOGI("Workbench ended\n");

    LOGD("De-initializing adapters:\n");
#define LDATA struct adapter
    ITERATE(ehwe.adapters) {
        LOGD("  %d\n", CDATA(ehwe.adapters).devid);
        ASSURE_E((rc =
                  adapters_deinit_adapter(CREF(ehwe.adapters))) == 0, goto err2);;
    }
#undef LDATA
    /* Close storage of adapter-list. OK as all of payload is also freed,
     * as long struct adapter does not contain any 2:nd depth-level heap
     * variable. */
    ASSURE((rc = mlist_close(ehwe.adapters)) == 0);

    ehwe_exit(rc);
err2:
    LOGE("Current rc: %d\n", rc);
    ASSURE((rc = mlist_close(ehwe.adapters)) == 0);
    ehwe_exit(1);
err:
    LOGE("Current rc: %d\n", rc);
    ASSURE((rc = mlist_close(opts.adapter_strs)) == 0);
    ehwe_exit(1);
    /* GCC, please shut up! */
    return 0;
}
