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
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <log.h>
#include "devices.h"
#include "driver.h"
#include "buspirate.h"
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

/* Commands in binary mode. Commands prefixed "ENTER" continues to further
 * submode. Not all commands are defines. Full definition here:
 * http://dangerousprototypes.com/docs/Bitbang#Commands
 */
typedef enum {
    ENTER_RESET = 0x00,         /* Back to console mode */
    ENTER_SPI = 0x01,
    ENTER_I2C = 0x02,
    ENTER_UART = 0x03,
    ENTER_1WIRE = 0x04,
    ENTER_RAWWIRE = 0x05,
    ENTER_OPENOCD = 0x05,
    RESET_BUSPIRATE = 0x0F      /* Execute full reset cycle */
} bpcmd_t;

/* Lookup-table: Expected replies for command */
struct cmdrply_s {
    bpcmd_t command;
    const char *reply;
};

struct cmdrply_s cmdrply[] = {
    {ENTER_RESET, "BBIO0"},
    {ENTER_SPI, "SPI1"},
    {ENTER_I2C, "I2C1"},
    {ENTER_UART, "ART1"},
    {ENTER_1WIRE, "1W01"},
    {ENTER_RAWWIRE, "RAW1"},
/*  {ENTER_OPENOCD, ""},       Unknown (TBD) */
    {RESET_BUSPIRATE, (char[]){'\1', '\0'}}
};

/* Driver companion - NOTE: unique for each driver. Must NOT be public */
struct ddata {
    int fd;
    bpcmd_t state;
};

#define REGEX_NSUB (5+1)

static void bpspi_sendData(const uint8_t *data, int sz);
static void bpspi_receiveData(uint8_t *data, int sz);
static uint16_t bpspi_getStatus(uint16_t flags);

/* Convenience variable: Bus-pirate provides one API. Variants communicated
 * through ddata */
static struct driverAPI bpspi_driver = {
    .ddata = NULL,
    .sendData = bpspi_sendData,
    .receiveData = bpspi_receiveData,
    .getStatus = bpspi_getStatus
};

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
    device->buspirate.name = (char *)(&devstr[mtch_idxs[5].rm_so]);

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
    memcpy(driver, &bpspi_driver, sizeof(struct driverAPI));

    ASSERT(ddata = malloc(sizeof(struct ddata)));
    ASSURE((ddata->fd = open(device->buspirate.name, O_RDWR)) != -1);

    driver->ddata = ddata;
    device->driver = driver;

    return 0;
}

int buspirate_deinit_device(struct device *device)
{
    struct driverAPI *driver = device->driver;
    struct ddata *ddata = driver->ddata;

    LOGI("BP: Destroying device ID [%d]\n", device->devid);
    ddata = device->driver->ddata;

    close(ddata->fd);
    free(ddata);
    free(driver);

    device->driver = NULL;
    return 0;
}

/***************************************************************************
 * Bus-pirate device functions
 * Inspired from:
 * http://dangerousprototypes.com/docs/Bus_Pirate:_Entering_binary_mode
 ***************************************************************************/

/* Enter binary mode from normal console-mode */
static void rawMode_enter(int fd)
{
    int ret;
    char tmp[100] = { '\0' };
    int done = 0;
    int tries = 0;

    LOGI("BusPirate entering binary mode...\n");

    /* check that the passed serial port exists and is working */
    if (fd == -1) {             /*added because the fd has already returned null */
        printf("Port does not exist!");
        return;

    }
    /*loop up to 25 times, send 0x00 each time and pause briefly for a reply (BBIO1) */
    while (!done) {
        tmp[0] = 0x00;
        LOGD("Sending 0X%X to port\n", tmp[0]);
        write(fd, tmp, 1);
        tries++;
        LOGD("tries: %i Ret %i\n", tries, ret);
        usleep(1);
        ret = read(fd, tmp, 5);
        if (ret != 5 && tries > 20) {
            LOGE("Buspirate did not respond correctly (%i,%i) \n", ret, tries);
            exit(-1);
        } else if (strncmp(tmp, "BBIO1", 5) == 0) {
            done = 1;
        }
        if (tries > 25) {
            LOGE("Buspirate: Too many tries in serial read! -exiting \n");
            LOGE("Buspirate: - chip not detected, or not readable/writable\n");
            exit(-1);
        }
    }

}

static void rawMode_toMode(int fd, bpcmd_t bpcmd)
{
    //int ret;
    //char tmp[100] = {'\0'};

/*
    tmp[0] = bbmode;            //the mode to select
    //printf("Sending 0X%X to port\n",tmp[0]);
    serial_write(fd, tmp, 1);
    usleep(1);
    ret = serial_read(fd, tmp, 4);

    if ((ret == 4) && (strncmp(tmp, "SPI1", 4) == 0)) { //check the reply

    } else {
        fprintf(stderr, "Buspirate did not respond correctly :( %i \n", ret);
        exit(-1);
    }
*/
}

/***************************************************************************
 * Driver interface
 ***************************************************************************/
static void bpspi_sendData(const uint8_t *data, int sz)
{
}

static void bpspi_receiveData(uint8_t *data, int sz)
{
}

static uint16_t bpspi_getStatus(uint16_t flags)
{
    return flags;
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
