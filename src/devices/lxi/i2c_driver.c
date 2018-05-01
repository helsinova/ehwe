/***************************************************************************
 *   Copyright (C) 2018 by Michael Ambrus                                  *
 *   michael@helsinova.se                                                  *
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
#include "lxi_config.h"
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
#include <string.h>
#include <stdlib.h>
#include <assure.h>
#include <arpa/inet.h>

struct config_I2C lxi_dflt_config_I2C = {
    .autoAck = ON,
    .speed = {
              .raw = 0xC1,
              },
    .pereph = {
               .raw = 0xC2,
               },
};

#define AUTOACK (ddata->config.i2c.autoAck)
/***************************************************************************
 * Main driver interface
 ***************************************************************************/
void
lxii2c_sendrecieveData(struct ddata *ddata, const uint8_t *obuf,
                       int osz, uint8_t *ibuf, int isz)
{
    ASSURE("Work in progress (WIP)" == NULL);
}

void lxii2c_start(struct ddata *ddata)
{
    ASSURE("LXI not supported function invoked" == NULL);
}

void lxii2c_stop(struct ddata *ddata)
{
    ASSURE("LXI not supported function invoked" == NULL);
}

void lxii2c_autoAck(struct ddata *ddata, int state)
{
    ASSURE("LXI not supported function invoked" == NULL);

    AUTOACK = state;
}

void lxii2c_receiveByte(struct ddata *ddata, uint8_t *data)
{
    ASSURE("LXI not supported function invoked" == NULL);
}

int lxii2c_sendByte(struct ddata *ddata, uint8_t data)
{
    ASSURE("LXI not supported function invoked" == NULL);
    return 0;
}

void lxii2c_sendData(struct ddata *ddata, const uint8_t *data, int sz)
{
    ASSURE("LXI not supported function invoked" == NULL);
}

void lxii2c_receiveData(struct ddata *ddata, uint8_t *data, int sz)
{
    ASSURE("Work in progress (WIP)" == NULL);
}

uint16_t lxii2c_getStatus(struct ddata *ddata, uint16_t flags)
{
    return flags;
}

int lxii2c_configure(struct ddata *ddata)
{
    LOGW("LXI not supported function invoked");
    return 0;
}

/* Create a new device/driver-data object for external manipulation without
 * interfering with current one. If arg "device" is not NULL it will be a
 * copy of current, else it's will be pre-set with build-system defaults. */
struct ddata *lxii2c_newddata(struct device *device)
{
    struct ddata *ddata = NULL;

    ASSERT(ddata = malloc(sizeof(struct ddata)));

    if (device != NULL)
        return (struct ddata *)memcpy(ddata, device->driver.any->ddata,
                                      sizeof(struct ddata));

    memcpy(&(ddata->config.i2c), &lxi_dflt_config_I2C,
           sizeof(struct config_I2C));

    return ddata;
}

/***************************************************************************
 * Configuration  interface
 ***************************************************************************/
config_etype_t lxii2c_set_speed(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_set_power_on(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_set_pullups(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_set_aux_on(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_set_cs_active(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_get_speed(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_get_power_on(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_get_pullups(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_get_aux_on(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_get_cs_active(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}
