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

struct config_I2C dflt_config_I2C = {
    .speed = {
              .cmd = I2CCMD_CONFIG_SPEED,
              .speed = BUSPIRATE_I2C_DFLT_SPEED},
    .pereph = {
               .cmd = SPICMD_CONFIG_PEREPHERIALS,
               .power_on = BUSPIRATE_I2C_DFLT_PON,
               .pullups = BUSPIRATE_I2C_DFLT_ENABLE_PULLUPS,
               .aux = BUSPIRATE_I2C_DFLT_AUX_ON,
               .cs_active = BUSPIRATE_I2C_DFLT_CS_START_LEVEL,
               },
};

/* Commands while in I2C mode. */
typedef enum {
    CMD_START_BIT = 0x02,
    CMD_I2C_STOP_BIT = 0x03,
    CMD_I2C_READ_BYTE = 0x04,
    CMD_ACK_BIT = 0x06,
    CMD_NACK_BIT = 0x07,
    CMD_WRITE_THEN_READ = 0x08,
    CMD_START_BUS_SNIFFER = 0x0F,
    CMD_BULK = 0X10
} bpcmd_i2c_t;

/***************************************************************************
 * Main driver interface
 ***************************************************************************/
void bpi2c_sendrecieveData(struct ddata *ddata, const uint8_t *obuf,
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
	/*
    ASSURE_E(write(ddata->fd, (uint8_t[]) {
                   CMD_WR_RD}, 1) != -1, LOGE_IOERROR(errno));
	*/
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

void bpi2c_sendrecieveData_ncs(struct ddata *ddata, const uint8_t *obuf,
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

	/*
    ASSURE_E(write(ddata->fd, (uint8_t[]) {
                   CMD_WR_RD_NOCS}, 1) != -1, LOGE_IOERROR(errno));
	*/
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

void bpi2c_setCS(struct ddata *ddata, int state)
{
    uint8_t tmp[8] = { 0 };
    uint8_t mstate = state;

/*
 * This is not the flag for inverted polarity. Kept in code for future
 * reference.

    if (!ddata->config.i2c.pereph.cs_active) {
        mstate = mstate ^ 0x01;
    }
*/
    ASSERT((mstate == 0) || (mstate == 1));
    mstate &= 0x01;

    /*
	tmp[0] = CMD_CS | mstate;
	*/
    LOGD("BP: Interface %s sets CS to: (0x%02X)\n", __func__, mstate, tmp[0]);

    ASSURE_E(write(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    memset(tmp, 0, sizeof(tmp));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSERT(tmp[0] == 0x01);

}

void bpi2c_sendData(struct ddata *ddata, const uint8_t *data, int sz)
{
    int i;
    char cbuf[512] = { '\0' };

    LOGW("BP: Interface %s sending %d bytes \n", __func__, sz);
    for (i = 0; i < sz; i++) {
        if (data[i] > 31)
            sprintf(cbuf, "0x%02X %c,", data[i], data[i]);
        else
            sprintf(cbuf, "0x%02X %s,", data[i], " ");
    }
    LOGW("BP: %s\n", cbuf);
}

void bpi2c_receiveData(struct ddata *ddata, uint8_t *data, int sz)
{
}

uint16_t bpi2c_getStatus(struct ddata *ddata, uint16_t flags)
{
    return flags;
}

int bpi2c_configure(struct ddata *ddata)
{
    int ret;
    uint8_t tmp[8] = { 0 };

    struct confi2c_pereph *pereph = &(ddata->config.i2c.pereph);
    struct confi2c_speed *speed = &(ddata->config.i2c.speed);

    tmp[0] = 0;
    ASSURE_E((ret =
              write(ddata->fd, speed, sizeof(struct confi2c_speed))) != -1,
             LOGE_IOERROR(errno));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSERT(tmp[0] == 0x01);
    memcpy(&ddata->config.i2c.speed, speed, sizeof(struct confi2c_speed));

    tmp[0] = 0;
    ASSURE_E((ret =
              write(ddata->fd, pereph, sizeof(struct confi2c_pereph))) != -1,
             LOGE_IOERROR(errno));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSERT(tmp[0] == 0x01);
    memcpy(&ddata->config.i2c.pereph, pereph, sizeof(struct confi2c_pereph));

    return 0;

}

/* Create a new device/driver-data object for external manipulation without
 * interfering with current one. If arg "device" is not NULL it will be a
 * copy of current, else it's will be pre-set with build-system defaults. */
struct ddata *bpi2c_newddata(struct device *device)
{
    struct ddata *ddata;
    ASSERT(ddata = malloc(sizeof(struct ddata)));

    if (device != NULL)
        return (struct ddata *)memcpy(ddata, device->driver->ddata,
                                      sizeof(struct ddata));

    memcpy(&(ddata->config.i2c), &dflt_config_I2C, sizeof(struct config_I2C));
    return ddata;
}

/***************************************************************************
 * Configuration  interface
 ***************************************************************************/
config_etype_t bpi2c_set_speed(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_set_power_on(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_set_pullups(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_set_aux_on(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_set_cs_active(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_set_output_type(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_set_clk_pol_idle(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_set_output_clk_edge(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_set_input_sample_end(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_get_speed(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_get_power_on(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_get_pullups(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_get_aux_on(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_get_cs_active(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_get_output_type(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_get_clk_pol_idle(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_get_output_clk_edge(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t bpi2c_get_input_sample_end(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}
