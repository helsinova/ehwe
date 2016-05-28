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
#include <arpa/inet.h>

struct config_SPI dflt_config_SPI = {
    .speed = {
              .cmd = SPICMD_CONFIG_SPEED,
              .speed = BUSPIRATE_SPI_DFLT_SPEED},
    .pereph = {
               .cmd = SPICMD_CONFIG_PEREPHERIALS,
               .power_on = BUSPIRATE_SPI_DFLT_PON,
               .pullups = BUSPIRATE_SPI_DFLT_ENABLE_PULLUPS,
               .aux = BUSPIRATE_SPI_DFLT_AUX_ON,
               .cs_active = BUSPIRATE_SPI_DFLT_CS_START_LEVEL,
               },
    .bus = {
            .cmd = SPICMD_CONFIG_BUS,
            .output_type = BUSPIRATE_SPI_DFLT_OUTPUT_TYPE,
            .clk_pol_idle = BUSPIRATE_SPI_DFLT_CLK_IDLE_POLARITY,
            .output_clk_edge = BUSPIRATE_SPI_DFLT_CLK_EDGE,
            .input_sample_end = BUSPIRATE_SPI_DFLT_SAMPLE,
            },
};

/* Commands while in SPI mode. */
typedef enum {
    CMD_CS = 0x02,              /* Toggle CS */
    CMD_WR_RD = 0x04,           /* Write then read up to 4096 bytes in each
                                   direction */
    CMD_WR_RD_NOCS = 0x05,      /* Same as CMD_WR_RD but without automatic
                                   toggling of CS */
    CMD_SNIFF = 0x0C,
    CMD_BULK = 0x10,
} bpcmd_spi_t;

/***************************************************************************
 * Main driver interface
 ***************************************************************************/
void bpspi_sendrecieveData(struct ddata *ddata, const uint8_t *obuf,
                           int osz, uint8_t *ibuf, int isz)
{
    int ret;
    uint16_t nsz_send, nsz_receive;
    uint8_t tmp[8] = { 0 };

    nsz_send = htons(osz);
    nsz_receive = htons(isz);
    ASSERT((osz < 4096) && (osz >= 0)); /* Primitive handling for now (TBD) */
    ASSERT(isz < 4096);

#ifndef NDEBUG
    memset(ibuf, 0, isz);
#endif

    LOGD("BP: Interface %s sending-receiving %d,%d bytes \n", __func__, osz,
         isz);

    //tmp[0] = 0;
    ASSURE_E(write(ddata->fd, (uint8_t[]) {
                   CMD_WR_RD}, 1) != -1, LOGE_IOERROR(errno));
    ASSURE_E(write(ddata->fd, &nsz_send, 2) != -1, LOGE_IOERROR(errno));
    ASSURE_E(write(ddata->fd, &nsz_receive, 2) != -1, LOGE_IOERROR(errno));
    //We need another solution for reading that can
    //timeout if blocked-on-read (TBD)
    //ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    //ASSERT(tmp[0] == 0x00);

    ASSURE_E((ret = write(ddata->fd, obuf, osz)) >= -1, LOGE_IOERROR(errno));
    LOGD("BP: %d bytes written to device\n", ret);
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    //n_sent=ntohs(*(int16_t*)(tmp));
    //LOGD("BP: %d bytes written SPI\n", n_sent);
    ASSERT(tmp[0] == 0x01);

    if (isz > 0) {
        ASSURE_E((ret = read(ddata->fd, ibuf, isz)) != -1, LOGE_IOERROR(errno));
        LOGD("BP: %d bytes read from device\n", ret);
    }
}

void bpspi_sendrecieveData_ncs(struct ddata *ddata, const uint8_t *obuf,
                               int osz, uint8_t *ibuf, int isz)
{
    int ret;
    uint16_t nsz_send, nsz_receive;
    uint8_t tmp[8] = { 0 };

    nsz_send = htons(osz);
    nsz_receive = htons(isz);
    ASSERT((osz < 4096) && (osz >= 0)); /* Primitive handling for now (TBD) */
    ASSERT(isz < 4096);

#ifndef NDEBUG
    memset(ibuf, 0, isz);
#endif

    LOGD("BP: Interface %s sending-receiving %d,%d bytes (NO CS)\n", __func__,
         osz, isz);

    ASSURE_E(write(ddata->fd, (uint8_t[]) {
                   CMD_WR_RD_NOCS}, 1) != -1, LOGE_IOERROR(errno));
    ASSURE_E(write(ddata->fd, &nsz_send, 2) != -1, LOGE_IOERROR(errno));
    ASSURE_E(write(ddata->fd, &nsz_receive, 2) != -1, LOGE_IOERROR(errno));

    ASSURE_E((ret = write(ddata->fd, obuf, osz)) >= -1, LOGE_IOERROR(errno));
    LOGD("BP: %d bytes written to device\n", ret);
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSERT(tmp[0] == 0x01);

    if (isz > 0) {
        ASSURE_E((ret = read(ddata->fd, ibuf, isz)) != -1, LOGE_IOERROR(errno));
        LOGD("BP: %d bytes read from device\n", ret);
    }
}

void bpspi_setCS(struct ddata *ddata, int state)
{
    uint8_t tmp[8] = { 0 };
    uint8_t mstate = state;

/*
 * This is not the flag for inverted polarity. Kept in code for future
 * reference.

    if (!ddata->config.spi.pereph.cs_active) {
        mstate = mstate ^ 0x01;
    }
*/
    ASSERT((mstate == 0) || (mstate == 1));
    mstate &= 0x01;

    tmp[0] = CMD_CS | mstate;
    LOGD("BP: Interface %s sets CS to: (0x%02X)\n", __func__, mstate, tmp[0]);

    ASSURE_E(write(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    memset(tmp, 0, sizeof(tmp));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSERT(tmp[0] == 0x01);

}

void bpspi_sendData(struct ddata *ddata, const uint8_t *data, int sz)
{
    bpspi_sendrecieveData(ddata, data, sz, NULL, 0);
}

void bpspi_receiveData(struct ddata *ddata, uint8_t *data, int sz)
{
    bpspi_sendrecieveData(ddata, NULL, 0, data, sz);
}

uint16_t bpspi_getStatus(struct ddata *ddata, uint16_t flags)
{
    return flags;
}

int bpspi_configure(struct ddata *ddata)
{
    int ret;
    uint8_t tmp[8] = { 0 };

    struct confspi_pereph *pereph = &(ddata->config.spi.pereph);
    struct confspi_speed *speed = &(ddata->config.spi.speed);
    struct confspi_bus *bus = &(ddata->config.spi.bus);

    tmp[0] = 0;
    ASSURE_E((ret =
              write(ddata->fd, speed, sizeof(struct confspi_speed))) != -1,
             LOGE_IOERROR(errno));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSERT(tmp[0] == 0x01);
    memcpy(&ddata->config.spi.speed, speed, sizeof(struct confspi_speed));

    tmp[0] = 0;
    ASSURE_E((ret =
              write(ddata->fd, bus, sizeof(struct confspi_bus))) != -1,
             LOGE_IOERROR(errno));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSERT(tmp[0] == 0x01);
    memcpy(&ddata->config.spi.bus, bus, sizeof(struct confspi_bus));

    tmp[0] = 0;
    ASSURE_E((ret =
              write(ddata->fd, pereph, sizeof(struct confspi_pereph))) != -1,
             LOGE_IOERROR(errno));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSERT(tmp[0] == 0x01);
    memcpy(&ddata->config.spi.pereph, pereph, sizeof(struct confspi_pereph));

    return 0;
}

/* Create a new device/driver-data object for external manipulation without
 * interfering with current one. If arg "device" is not NULL it will be a
 * copy of current, else it's will be pre-set with build-system defaults. */
struct ddata *bpspi_newddata(struct device *device)
{
    struct ddata *ddata;
    ASSERT(ddata = malloc(sizeof(struct ddata)));

    if (device != NULL)
        return (struct ddata *)memcpy(ddata, device->driver.any->ddata,
                                      sizeof(struct ddata));

    memcpy(&(ddata->config.spi), &dflt_config_SPI, sizeof(struct config_SPI));
    return ddata;
}

/***************************************************************************
 * Configuration  interface
 ***************************************************************************/
config_etype_t bpspi_set_speed(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_set_power_on(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_set_pullups(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_set_aux_on(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_set_cs_active(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_set_output_type(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_set_clk_pol_idle(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_set_output_clk_edge(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_set_input_sample_end(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_get_speed(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_get_power_on(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_get_pullups(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_get_aux_on(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_get_cs_active(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_get_output_type(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_get_clk_pol_idle(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_get_output_clk_edge(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpspi_get_input_sample_end(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}
