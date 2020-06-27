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
#include <adapters.h>
#include <driver.h>
#include <string.h>
#include <stdlib.h>
#include <assure.h>
#include <arpa/inet.h>

struct config_SPI lxi_dflt_config_SPI = {
    .speed = {
              .raw = 0xD0,
              },
    .pereph = {
               .raw = 0xD1,
               },
    .bus = {
            .raw = 0xD2,
            },
};

/***************************************************************************
 * Main driver interface
 ***************************************************************************/
void lxispi_sendrecieveData(struct ddata *ddata, const uint8_t *obuf,
                            int osz, uint8_t *ibuf, int isz)
{
    ASSURE("Work in progress (WIP)" == NULL);
    char tmp_obuf[osz + isz];
    char *tmp_ibuf;

    tmp_ibuf = malloc(osz + isz);
    memcpy(tmp_obuf, obuf, osz + isz);

#ifndef NDEBUG
    memset(ibuf, 0, isz);
#endif

    LOGD("LXI: Interface %s sending-receiving %d,%d bytes \n", __func__, osz,
         isz);

    free(tmp_ibuf);
}

/* Ditto but with no CS toggling */
void lxispi_sendrecieveData_ncs(struct ddata *ddata, const uint8_t *obuf,
                                int osz, uint8_t *ibuf, int isz)
{
    ASSURE("Work in progress (WIP)" == NULL);
}

void lxispi_setCS(struct ddata *ddata, int state)
{
    ASSURE("Work in progress (WIP)" == NULL);
}

void lxispi_sendData(struct ddata *ddata, const uint8_t *data, int sz)
{
    ASSURE("Work in progress (WIP)" == NULL);
}

void lxispi_receiveData(struct ddata *ddata, uint8_t *data, int sz)
{
    lxispi_sendrecieveData(ddata, NULL, 0, data, sz);
}

uint16_t lxispi_getStatus(struct ddata *ddata, uint16_t flags)
{
    return flags;
}

int lxispi_configure(struct ddata *ddata)
{
    LOGW("LXI not supported function invoked");
    return 0;
}

/* Create a new adapter/driver-data object for external manipulation without
 * interfering with current one. If arg "adapter" is not NULL it will be a
 * copy of current, else it's will be pre-set with build-system defaults. */
struct ddata *lxispi_newddata(struct adapter *adapter)
{
    struct ddata *ddata;
    ASSERT(ddata = malloc(sizeof(struct ddata)));

    if (adapter != NULL)
        return (struct ddata *)memcpy(ddata, adapter->driver.any->ddata,
                                      sizeof(struct ddata));

    memcpy(&(ddata->config.spi), &lxi_dflt_config_SPI,
           sizeof(struct config_SPI));
    return ddata;
}

/***************************************************************************
 * Configuration  interface
 ***************************************************************************/
config_etype_t lxispi_set_speed(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_set_power_on(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_set_pullups(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_set_aux_on(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_set_cs_active(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_set_output_type(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_set_clk_pol_idle(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_set_output_clk_edge(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_set_input_sample_end(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_get_speed(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_get_power_on(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_get_pullups(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_get_aux_on(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_get_cs_active(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_get_output_type(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_get_clk_pol_idle(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_get_output_clk_edge(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxispi_get_input_sample_end(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}
