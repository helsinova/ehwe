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
#include <errno.h>
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
 * submode. Note: Not all commands are defined here, only mode-changers (so
 * far). Full definition here:
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
    const char *rply;
};

struct cmdrply_s cmdrply[] = {
    {ENTER_RESET, "BBIO1"},     /*<- Note: protocol-version specific */
    {ENTER_SPI, "SPI1"},
    {ENTER_I2C, "I2C1"},
    {ENTER_UART, "ART1"},
    {ENTER_1WIRE, "1W01"},
    {ENTER_RAWWIRE, "RAW1"},
/*  {ENTER_OPENOCD, ""},       Unknown (TBD) */
    {RESET_BUSPIRATE, (char[]){'\1', '\0'}}
};

#define msleep( X ) \
	usleep( (X) * 1000 )
#define TBL_LEN (sizeof(cmdrply) / sizeof(struct cmdrply_s))
#define BUF_SZ 100
#define STATE_RETRIES 10
#define US_CHAR_TIME 100        /* Time in uS to propagate one character over
								 * the serial device. Note: Baud-rate
								 * dependent  
                                 */
#define MAX_ONGOING_CHARS 6     /* Number of character possibly coming */
#define US_DELAY_RETRY (MAX_ONGOING_CHARS * US_CHAR_TIME)
                                /* Must be long enough to allow any ongoing
                                 * replies to fully reach UART registers, 
                                 * including possible multiple strings. If
								 * Baud-rate > 115kb make sure this doesn't
								 * evaluate to 0.
                                 */

/* Driver companion - NOTE: unique for each driver. Must NOT be public */
struct ddata {
    int fd;
    bpcmd_t state;
};

#define REGEX_NSUB (5+1)

static void bpspi_sendData(const uint8_t *data, int sz);
static void bpspi_receiveData(uint8_t *data, int sz);
static uint16_t bpspi_getStatus(uint16_t flags);

static void log_ioerror(int ecode, log_level llevel);
static int rawMode_enter(struct device *);
static int rawMode_toMode(struct device *, bpcmd_t bpcmd);
static void empty_inbuff(int fd);
static int read_2err(int fd, char *rbuff, int len);
static char *expected_rply(int cmd);
static unsigned int lookup_cmd(char *rply);

#define LOGV_IOERROR( X ) log_ioerror( X , LOG_LEVEL_VERBOSE )
#define LOGD_IOERROR( X ) log_ioerror( X , LOG_LEVEL_DEBUG )
#define LOGI_IOERROR( X ) log_ioerror( X , LOG_LEVEL_INFO )
#define LOGW_IOERROR( X ) log_ioerror( X , LOG_LEVEL_WARNING )
#define LOGE_IOERROR( X ) log_ioerror( X , LOG_LEVEL_ERROR )

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
    ASSURE((ddata->fd =
            open(device->buspirate.name, O_RDWR | O_NONBLOCK)) != -1);

    empty_inbuff(ddata->fd);
    driver->ddata = ddata;
    device->driver = driver;

    ASSURE(rawMode_enter(device) == 0);

    switch (device->role) {
        case SPI:
            ASSURE(rawMode_toMode(device, ENTER_SPI) == 0);
            break;
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

    device->driver = NULL;
    return 0;
}

/***************************************************************************
 * Bus-pirate device functions
 * Inspired from:
 * http://dangerousprototypes.com/docs/Bus_Pirate:_Entering_binary_mode
 ***************************************************************************/
static void log_ioerror(int ecode, log_level llevel)
{
    char buff[120];

    strerror_r(ecode, buff, 120);

    switch (llevel) {
        case LOG_LEVEL_VERBOSE:
            LOGV("File I/O error: %s\n", buff);
            break;
        case LOG_LEVEL_DEBUG:
            LOGD("File I/O error: %s\n", buff);
            break;
        case LOG_LEVEL_INFO:
            LOGI("File I/O error: %s\n", buff);
            break;
        case LOG_LEVEL_WARNING:
            LOGW("File I/O error: %s\n", buff);
            break;
        case LOG_LEVEL_ERROR:
            LOGE("File I/O error: %s\n", buff);
            break;
        default:
            LOGE("File I/O error: %s\n", buff);
    }

}

/* Reads fd one character at a time until error occurs */
static int read_2err(int fd, char *rbuff, int len)
{
    int rc, m_errno, idx = 0;
    char tmp[5];
    log_level llevel;

    do {
        rc = read(fd, tmp, 1);
        m_errno = errno;
        if (rc != -1)
            rbuff[idx++] = tmp[0];
    } while ((rc != -1) && (idx < len));

    llevel = get_loglevel_assure();
    set_loglevel_assure(LOG_LEVEL_DEBUG);
    ASSURE_E(idx > 0, LOGD_IOERROR(m_errno));
    set_loglevel_assure(llevel);

    if (idx > 0)
        return idx;
    if (m_errno != EAGAIN)
        return rc;
    return 0;
}

static void empty_inbuff(int fd)
{
    int ret, tries = 0;
    char tmp[3];
    do {
        ret = read_2err(fd, tmp, 1);
        if ((ret == -1) && (errno != EAGAIN))
            LOGE_IOERROR(errno);
        tries++;
    } while (ret > 0);
}

static char *expected_rply(int cmd)
{
    int i;
    for (i = 0; i < TBL_LEN; i++) {
        if (cmdrply[i].command == cmd)
            return (char *)cmdrply[i].rply;
    }
    return NULL;
}

static unsigned int lookup_cmd(char *rply)
{
    int i;
    for (i = 0; i < TBL_LEN; i++) {
        if (strcmp(cmdrply[i].rply, rply) == 0)
            return cmdrply[i].command;
    }
    return 0xDEAD;
}

/* Enter binary mode from normal console-mode */
static int rawMode_enter(struct device *device)
{
    int ret, i, corr_cmd;
    char tmp[BUF_SZ] = { '\0' };
    int done = 0;
    int tries = 0;
    struct ddata *ddata = device->driver->ddata;
    int *fd = &ddata->fd;

    LOGI("BusPirate entering binary mode...\n");

    if (*fd == -1) {
        LOGE("Device isn't open\n");
        return -1;
    }

    /* Do the first 20 writes without listening for any reply. This is
     * according to BusPirate protocol spec V1.
     */
    for (i = 0; i < 20; i++) {
        tmp[0] = 0x00;
        LOGD("Sending 0x%02X to port\n", tmp[0]);
        usleep(US_CHAR_TIME);
        ASSURE_E((ret = write(*fd, tmp, 1)) != -1, LOGE_IOERROR(errno));
    }

    /*loop up to 25 more times, send 0x00 each time and pause briefly for a reply (BBIO1) */
    while (!done) {
        tmp[0] = 0x00;
        LOGD("Sending 0x%02X to port\n", tmp[0]);
        ASSURE_E((ret = write(*fd, tmp, 1)) != -1, LOGE_IOERROR(errno));
        tries++;
        LOGD("tries: %i Ret %i\n", tries, ret);
        usleep(US_CHAR_TIME);
        ASSURE_E((ret = read_2err(*fd, tmp, 5)) != -1, LOGE_IOERROR(errno));
        if (ret != 5 && tries > 22) {
            corr_cmd = lookup_cmd(tmp);
            LOGE("Buspirate did not respond correctly in function [%s] (%i,%i)\n", __func__, ret, tries);
            LOGE("  Command requested: 0x%02X\n", 0x00);
            LOGE("  Reply matches:     0x%02X\n", corr_cmd);
            return -1;
        } else if (strncmp(tmp, "BBIO1", 5) == 0) {
            /* Empty any remains in buffer */
            empty_inbuff(*fd);
            done = 1;
        }
        if (tries > 25) {
            LOGE("Buspirate: Too many tries in serial read! -exiting \n");
            LOGE("Buspirate: - chip not detected, or not readable/writable\n");
            return -1;
        }
        usleep(US_DELAY_RETRY);
        empty_inbuff(*fd);
    }

    ddata->state = ENTER_RESET;
    return 0;
}

static int rawMode_toMode(struct device *device, bpcmd_t bpcmd)
{
    int ret, i, slen, corr_cmd, tries = 0;
    char tmp[BUF_SZ] = { '\0' };
    char *expRply = NULL;
    struct ddata *ddata = device->driver->ddata;
    int *fd = &ddata->fd;

    LOGI("BusPirate entering mode %d\n", bpcmd);

    for (i = 0; i < TBL_LEN; i++) {
        if (cmdrply[i].command == bpcmd)
            expRply = (char *)cmdrply[i].rply;
    }
    expRply = expected_rply(bpcmd);
    if (expRply == NULL) {
        LOGE("BusPirate drive doesn't know what response to expect for cmd %d",
             bpcmd);
        return -1;
    }

    for (tries = 0; tries < STATE_RETRIES; tries++) {
        memset(tmp, 0, BUF_SZ);
        tmp[0] = bpcmd;
        LOGD("Sending 0x%02X to port. Expecting response %s\n", tmp[0],
             expRply);
        ASSURE_E((ret = write(*fd, tmp, 1)) != -1, LOGE_IOERROR(errno));
        msleep(1);
        slen = strlen(expRply);
        ASSURE_E((ret = read_2err(*fd, tmp, slen)) != -1, LOGE_IOERROR(errno));

        if ((ret == strlen(expRply)) && (strncmp(tmp, expRply, slen) == 0)) {
            ddata->state = bpcmd;
            empty_inbuff(*fd);
            return 0;
        }
        usleep(US_DELAY_RETRY);
        empty_inbuff(*fd);
        LOGD("Retry (%d) due to Buspirate unexpected response to "
             "mode-change: (%i,%s) \n", tries + 1, ret, tmp);
    }

    corr_cmd = lookup_cmd(tmp);
    LOGE("Buspirate did not respond correctly in function [%s] (%i,%i)\n",
         __func__, ret, tries);
    LOGE("  Command requested: 0x%02X\n", bpcmd);
    LOGE("  Reply matches:     0x%02X\n", corr_cmd);
    return -1;
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
