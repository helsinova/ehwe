/***************************************************************************
 *   Copyright (C) 2016 by Michael Ambrus                                  *
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
#include <sys/types.h>
#include <regex.h>
#include <log.h>
#include "devices.h"
#include "buspirate.h"
#include <string.h>
#include <stdlib.h>
#include <assure.h>

struct buspirate buspirate;
static regex_t preg;            /* Compiled regular expression for generic
                                   part of device-string parsing */

#define REGEX_PATT \
	"^(" ROLES \
	"):(" INDEX \
	"):(" DEVICES \
	"):(" ANYTHING \
")"

#define REGEX_NSUB (4+1)

int devices_init()
{
    int rc;
    char err_str[REXP_ESTRSZ];
    static int is_init = 0;

    if (is_init) {
        LOGW("No need to run %s twice, CTOR _init has run it?\n", __func__);
        return 0;
    }
    is_init = 1;

    rc = regcomp(&preg, REGEX_PATT, REG_EXTENDED | REG_ICASE);
    if (rc) {
        regerror(rc, &preg, err_str, REXP_ESTRSZ);
        LOGE("Regexec compilation error: %s\n", err_str);
        return rc;
    }
    return 0;
}

/*
 * Takes devstr and makes initial parsing to detect which driver it should
 * send each description to for further refinement. Result is a struct
 * device,
 *
 * */
int devices_parse(const char *devstr, struct device *device)
{
    int rc, i;
    char err_str[REXP_ESTRSZ];
    regmatch_t mtch_idxs[REGEX_NSUB];
    char *devstr_cpy = strdup(devstr);
    char *role_str;
    char *index_str;
    char *device_str;

    device->devid = DEV_INVALID;

    rc = regexec(&preg, devstr_cpy, REGEX_NSUB, mtch_idxs, 0);
    if (rc) {
        regerror(rc, &preg, err_str, REXP_ESTRSZ);
        LOGE("Regexec match error: %s\n", err_str);
        free(devstr_cpy);
        return rc;
    }
    /* Add string terminators in substrings */
    for (i = 1; i < REGEX_NSUB; i++) {
        ASSURE_E(mtch_idxs[i].rm_so != -1, goto devices_parse_err);
        devstr_cpy[mtch_idxs[i].rm_eo] = 0;
    }

    role_str = &devstr_cpy[mtch_idxs[1].rm_so];
    index_str = &devstr_cpy[mtch_idxs[2].rm_so];
    device_str = &devstr_cpy[mtch_idxs[3].rm_so];

    LOGD("  First level device-string parsing:\n");
    LOGD("    role=%s\n", role_str);
    LOGD("    index=%s\n", index_str);
    LOGD("    device=%s\n", device_str);

#ifdef DEVICE_PARAPORT
    if (strcasecmp(device_str, "pp") == 0)
        rc = paraport_parse(devstr, device);
#endif
#ifdef DEVICE_BUSPIRATE
    if (strcasecmp(device_str, "bp") == 0)
        rc = buspirate_parse(devstr, device);
#endif

    free(devstr_cpy);
    return 0;
devices_parse_err:
    free(devstr_cpy);
    return -1;
}

int devices_init_device(const struct device *device)
{
    int rc = 0;
    switch (device->devid) {
#ifdef DEVICE_PARAPORT
        case PARAPORT:
            rc = paraport_init_device(device);
            break;
#endif
#ifdef DEVICE_BUSPIRATE
        case BUSPIRATE:
            rc = buspirate_init_device(device);
            break;
#endif
        default:
            LOGE("Unsupported device [%d]\n", device->devid);
    }
    return rc;
}

/***************************************************************************
 * INIT/FINI mechanism
 ***************************************************************************/
#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

void __init __devices_init(void)
{
    int rc;
#ifdef INITFINI_SHOW
    fprintf(stderr, ">>> Running module _init in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism ====\n");
#endif
    if ((rc = devices_init())) {
        fprintf(stderr, "Fatal error: devices_init() failed\n");
        exit(rc);
    }
}

void __fini __devices_fini(void)
{
#ifdef INITFINI_SHOW
    fprintf(stderr, ">>> Running module _fini in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism\n");
#endif
}
