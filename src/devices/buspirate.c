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

static regex_t preg;            /* Compiled regular expression for full
                                   device-string parsing */

#define REGEX_PATT \
	"(" BP_ROLES \
	"):(" INDEX \
	"):(" DEVICES \
	"):(" BP_CLKOWNER \
	"):(" FILENAME \
")"

#define REGEX_NSUB (5+1)

int buspirate_init()
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
 * Refined parsing of devstring to complete buspirate dev_struct
 *
 * */
int buspirate_parse(const char *devstr, struct device *device)
{
    int rc, i;
    char err_str[REXP_ESTRSZ];
    regmatch_t mtch_idxs[REGEX_NSUB];
    char *devstr_cpy = strdup(devstr);
    char *role_str;
    char *index_str;
    char *device_str;
    char *clkownr_str;
    char *filename_str;

    device->role = ROLE_INVALID;
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
        ASSURE_E(mtch_idxs[i].rm_so != -1, goto buspirate_parse_err);
        devstr_cpy[mtch_idxs[i].rm_eo] = 0;
    }

    role_str = &devstr_cpy[mtch_idxs[1].rm_so];
    index_str = &devstr_cpy[mtch_idxs[2].rm_so];
    device_str = &devstr_cpy[mtch_idxs[3].rm_so];
    clkownr_str = &devstr_cpy[mtch_idxs[4].rm_so];
    filename_str = &devstr_cpy[mtch_idxs[5].rm_so];

    LOGD("  Second level device-string parsing (by %s):\n", __func__);
    LOGD("    role=%s\n", role_str);
    LOGD("    index=%s\n", index_str);
    LOGD("    device=%s\n", device_str);
    LOGD("    clkownr=%s\n", clkownr_str);
    LOGD("    filename=%s\n", filename_str);

    ASSURE_E(strcasecmp(device_str, "bp") == 0, goto buspirate_parse_err);

    if (strcasecmp(role_str, "spi") == 0) {
        device->role = SPI;
    } else if (strcasecmp(role_str, "i2c") == 0) {
        device->role = I2C;
    } else {
        LOGE("Buspirate device driver can't handle role: %s\n", role_str);
        goto buspirate_parse_err;
    }

    device->index = atoi(index_str);
    device->devid = BUSPIRATE;

    if (strcasecmp(clkownr_str, "master") == 0) {
        device->buspirate.clckownr = MASTER;
    } else if (strcasecmp(clkownr_str, "slave") == 0) {
        device->buspirate.clckownr = SLAVE;
    } else {
        LOGE("Buspirate device driver can't handle clkownr: %s\n", clkownr_str);
        goto buspirate_parse_err;
    }

    /* Avoid need to strdup by using original which happens to terminate
     * correctly as well. Ignore const as this string belongs to
     * environment with process-long lifetime */
    device->buspirate.path = (char *)(&devstr[mtch_idxs[5].rm_so]);

    free(devstr_cpy);
    return 0;
buspirate_parse_err:
    free(devstr_cpy);
    return -1;
}
