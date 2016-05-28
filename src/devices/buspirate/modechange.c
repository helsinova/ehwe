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
#include <devices.h>
#include <driver.h>
#include <buspirate.h>
#include <string.h>
#include <stdlib.h>
#include <assure.h>
#include "local.h"

/* Lookup-table: Expected replies for command */
struct cmdrply_s {
    bpcmd_raw_t command;
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

#define TBL_LEN (sizeof(cmdrply) / sizeof(struct cmdrply_s))
#define BUF_SZ 100
#define STATE_RETRIES 10
#define US_BPCMD_RESPONSE_TIME 725
                                /* Time in uS for BusPirate to process a
                                 * command. Machine constant.*/
#define US_CHAR_TIME 100        /* Time in uS to propagate one character
                                 * over the serial device. Note: Baud-rate
                                 * dependent. TBD: detect baud-rate to
                                 * release this dependency, current constant
                                 * assumes 9K6 bps
                                 */
#define MAX_ONGOING_CHARS 6     /* Number of character possibly coming */
#define US_DELAY_RETRY (MAX_ONGOING_CHARS * US_CHAR_TIME)
                                /* Must be long enough to allow any ongoing
                                 * replies to fully reach UART registers,
                                 * including possible multiple strings. If
                                 * Baud-rate > 115kb make sure this doesn't
                                 * evaluate to 0.
                                 */

static int read_2err(int fd, char *rbuff, int len);
static char *expected_rply(int cmd);
static unsigned int lookup_cmd(char *rply);

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

/* Empty (i.e. flush) any remaining stdlib-level in-buffer characters. This
   could be necessary when BP and host are not in sync, i.e. reply from a
   previous command is already in in-buffer (resulting in that current
   response will be the pending previous one).

  NOTE: **ONLY** to be used when device is in non-blocking. Add check for
  this (TBD)
 */
void empty_inbuff(int fd)
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
    LOGE("BusPirate drive doesn't know what response to expect for cmd %d",
         cmd);
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

/***************************************************************************
 * Bus-pirate device functions
 * Inspired from:
 * http://dangerousprototypes.com/docs/Bus_Pirate:_Entering_binary_mode
 ***************************************************************************/
void log_ioerror(int ecode, log_level llevel)
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

/* Enter binary mode from normal console-mode */
int rawMode_enter(struct device *device)
{
    int ret, slen, i, corr_cmd;
    char tmp[BUF_SZ] = { '\0' };
    char *expRply = NULL;
    int done = 0;
    int tries = 0;
    struct ddata *ddata = device->driver.any->ddata;
    int *fd = &ddata->fd;

    LOGI("BusPirate entering binary mode...\n");
    expRply = expected_rply(ENTER_RESET);
    slen = strlen(expRply);

    if (*fd == -1) {
        LOGE("Device isn't open\n");
        return -1;
    }

    /* Do the first 20 writes without listening for any reply. This is
     * according to BusPirate protocol spec V1.
     */
    for (i = 0; i < 20; i++) {
        tmp[0] = ENTER_RESET;
        LOGD("Sending 0x%02X to port\n", tmp[0]);
        usleep(US_CHAR_TIME * (slen + 2) + US_CHAR_TIME +
               US_BPCMD_RESPONSE_TIME);
        ASSURE_E((ret = write(*fd, tmp, 1)) != -1, LOGE_IOERROR(errno));
    }

    /*loop up to 25 more times, send 0x00 each time and pause briefly for a reply (BBIO1) */
    while (!done) {
        tmp[0] = 0x00;
        LOGD("Sending 0x%02X to port\n", tmp[0]);
        ASSURE_E((ret = write(*fd, tmp, 1)) != -1, LOGE_IOERROR(errno));
        tries++;
        LOGD("tries: %i Ret %i\n", tries, ret);
        usleep(US_CHAR_TIME * (slen + 2) + US_CHAR_TIME +
               US_BPCMD_RESPONSE_TIME);
        ASSURE_E((ret = read_2err(*fd, tmp, 5)) != -1, LOGE_IOERROR(errno));
        if (ret != 5 && tries > 22) {
            corr_cmd = lookup_cmd(tmp);
            LOGE("Buspirate did not respond correctly in function "
                 "[%s] (%i,%i)\n", __func__, ret, tries);
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

int rawMode_toMode(struct device *device, bpcmd_raw_t bpcmd)
{
    int ret, slen, corr_cmd, tries = 0;
    char tmp[BUF_SZ] = { '\0' };
    char *expRply = NULL;
    struct ddata *ddata = device->driver.any->ddata;
    int *fd = &ddata->fd;

    LOGI("BusPirate entering mode %d\n", bpcmd);
    expRply = expected_rply(bpcmd);
    slen = strlen(expRply);

    usleep(US_DELAY_RETRY);
    empty_inbuff(*fd);
    for (tries = 0; tries < STATE_RETRIES; tries++) {
        memset(tmp, 0, BUF_SZ);
        tmp[0] = bpcmd;
        LOGD("Sending 0x%02X to port. Expecting response %s\n", tmp[0],
             expRply);
        ASSURE_E((ret = write(*fd, tmp, 1)) != -1, LOGE_IOERROR(errno));
        usleep(US_CHAR_TIME * (slen + 2) + US_CHAR_TIME +
               US_BPCMD_RESPONSE_TIME);
        ASSURE_E((ret = read_2err(*fd, tmp, slen)) != -1, LOGE_IOERROR(errno));

        if ((ret == slen) && (strncmp(tmp, expRply, slen) == 0)) {
            ddata->state = bpcmd;
            empty_inbuff(*fd);
            return 0;
        }
        usleep(US_DELAY_RETRY);
        empty_inbuff(*fd);
        LOGE("Retry (%d) due to Buspirate unexpected response to "
             "mode-change: (%i,%s).\n", tries + 1, ret, tmp);
    }

    corr_cmd = lookup_cmd(tmp);
    LOGE("Buspirate did not respond correctly in function [%s] (%i,%i)\n",
         __func__, ret, tries);
    LOGE("  Command requested: 0x%02X\n", bpcmd);
    LOGE("  Reply matches:     0x%02X\n", corr_cmd);
    return -1;
}
