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
#include <devices.h>

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
    int rc;
    LOGI("\"ehwe\" version v%s \n", VERSION);

    /* Storage for device-specification strings */
    ASSURE((rc = mlist_opencreate(sizeof(char *), NULL, &opts.dev_strs)) == 0);

    opts_init();
    /* /begin/ zeroing of opt vars */
    /* /end/   zeroing of opt vars */

    ASSURE_E((rc = opts_parse(argc, argv, &opts)) >= 0, goto err);
    LOGI("Parsed %d options.\n", rc);
    ASSURE_E(opts_check(&opts) == OPT_OK, goto err);
    LOGI("Option passed rule-check OK\n", rc);

    /* Storage for device-specification strings */
    ASSURE((rc =
            mlist_opencreate(sizeof(struct device), NULL, &ehwe.devices)) == 0);

    /* Convert device strings to devices */
    LOGD("List of device definition strings:\n");
#undef LDATA
#define LDATA char *
    ITERATE(opts.dev_strs) {
        struct device device;

        LOGD("  %s\n", CDATA(opts.dev_strs));
        devices_parse(CDATA(opts.dev_strs), &device);
        /* OK to add stack-variable as deep-copy to new location occurs. */
        ASSURE(mlist_add_last(ehwe.devices, &device));
    }
#undef LDATA

    /* Close storage of device-specification strings */
    ASSURE((rc = mlist_close(opts.dev_strs)) == 0);

    LOGD("Initialize device:\n");
#undef LDATA
#define LDATA struct device
    ITERATE(ehwe.devices) {
        LOGD("  %d\n", CDATA(ehwe.devices).devid);
        devices_init_device(CREF(ehwe.devices));
    }
#undef LDATA

    /* Close storage of device-list. OK as all of payload is also freed,
     * as long struct device does not contain any 2:nd depth-level heap
     * variable. */
    ASSURE((rc = mlist_close(ehwe.devices)) == 0);

    ehwe_exit(0);
err:
    LOGE("Current rc: %d\n", rc);
    ehwe_exit(1);
    /* GCC, please shut up! */
    return 0;
}
