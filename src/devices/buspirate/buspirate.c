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
#include "config.h"
#include "buspirate_config.h"
#include "local.h"
#include <sys/types.h>
#include <regex.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <log.h>
#include <devices.h>
#include <driver.h>
#include <buspirate.h>
#include <string.h>
#include <stdlib.h>
#include <assure.h>

static regex_t preg;            /* Compiled regular expression for full
                                   device-string parsing */

#define REGEX_PATT \
	"^(" BP_ROLES \
	"):(" INDEX \
	"):(" DEVICES \
	"):(" BP_CLKOWNER \
	"):(" FILENAME \
")"

/* Driver companion - NOTE: unique for each driver. Must NOT be public */
struct ddata;

#define REGEX_NSUB (5+1)

/* Convenience variables: */
#ifdef BUSPIRATE_ENABLE_SPI
/*    SPI driver */
static struct driverAPI bpspi_driver = {
    .ddata = NULL,
    .sendData = bpspi_sendData,
    .receiveData = bpspi_receiveData,
    .getStatus = bpspi_getStatus
};
#endif
#ifdef BUSPIRATE_ENABLE_I2C
/*    I2C driver */
static struct driverAPI bpi2c_driver = {
    .ddata = NULL,
    .sendData = bpi2c_sendData,
    .receiveData = bpi2c_receiveData,
    .getStatus = bpi2c_getStatus
};
#endif

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
    device->buspirate = malloc(sizeof(struct buspirate));

    if (strcasecmp(clkownr_str, "master") == 0) {
        device->buspirate->clckownr = MASTER;
    } else if (strcasecmp(clkownr_str, "slave") == 0) {
        device->buspirate->clckownr = SLAVE;
    } else {
        LOGE("Buspirate device driver can't handle clkownr: %s\n", clkownr_str);
        goto buspirate_parse_err;
    }

    /* Avoid need to strdup by using original which happens to terminate
     * correctly as well. Ignore const as this string belongs to
     * environment with process-long lifetime */
    device->buspirate->name = (char *)(&devstr[mtch_idxs[5].rm_so]);

    free(devstr_cpy);
    return 0;
buspirate_parse_err:
    free(devstr_cpy);
    return -1;
}

int buspirate_init_device(struct device *device)
{
    struct driverAPI *driver;
    struct ddata *ddata;

    LOGI("BP: Initializing device ID [%d]\n", device->devid);

    ASSERT(driver = malloc(sizeof(struct driverAPI)));
    switch (device->role) {
#ifdef BUSPIRATE_ENABLE_SPI
        case SPI:
            memcpy(driver, &bpspi_driver, sizeof(struct driverAPI));
            break;
#endif
#ifdef BUSPIRATE_ENABLE_I2C
        case I2C:
            memcpy(driver, &bpi2c_driver, sizeof(struct driverAPI));
            break;
#endif
        default:
            LOGE("Role [%d] is not supported by Bus-Pirate\n", device->role);
            return -1;
    }

    ASSERT(ddata = malloc(sizeof(struct ddata)));
    ASSURE((ddata->fd =
            open(device->buspirate->name, O_RDWR | O_NONBLOCK)) != -1);

    empty_inbuff(ddata->fd);
    driver->ddata = ddata;
    driver->device = device;
    device->driver = driver;

    ASSURE(rawMode_enter(device) == 0);

    switch (device->role) {
#ifdef BUSPIRATE_ENABLE_SPI
        case SPI:
            ASSURE(rawMode_toMode(device, ENTER_SPI) == 0);
            break;
#endif
#ifdef BUSPIRATE_ENABLE_I2C
        case I2C:
            ASSURE(rawMode_toMode(device, ENTER_I2C) == 0);
            break;
#endif
        default:
            LOGE("Device BusPirate can't handle role %d (yet)\n", device->role);
            ASSURE("Bad device->role" == 0);
    }

    return 0;
}

int buspirate_deinit_device(struct device *device)
{
    struct driverAPI *driver = device->driver;
    struct ddata *ddata = driver->ddata;
    struct buspirate *buspirate = device->buspirate;

    LOGI("BP: Destroying device ID [%d]\n", device->devid);
    empty_inbuff(ddata->fd);
    ASSURE(rawMode_toMode(device, ENTER_RESET) == 0);
    msleep(1);
    empty_inbuff(ddata->fd);
    ASSURE(rawMode_toMode(device, RESET_BUSPIRATE) == 0);
    msleep(100);

    close(ddata->fd);
    free(ddata);
    free(driver);
    free(buspirate);

    device->driver = NULL;
    device->buspirate = NULL;
    return 0;
}

/***************************************************************************
 * INIT/FINI mechanism
 ***************************************************************************/
#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

void __init __buspirate_init(void)
{
    int rc;
#ifdef INITFINI_SHOW
    fprintf(stderr, ">>> Running module _init in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism ====\n");
#endif
    if ((rc = buspirate_init())) {
        fprintf(stderr, "Fatal error: buspirate_init() failed\n");
        exit(rc);
    }
}

void __fini __buspirate_fini(void)
{
#ifdef INITFINI_SHOW
    fprintf(stderr, ">>> Running module _fini in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism\n");
#endif
}
