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
              .cmd = CONFIG_SPI_SPEED,
              .speed = BUSPIRATE_SPI_DFLT_SPEED},
    .pereph = {
               .cmd = CONFIG_SPI_PEREPHERIALS,
               .power_on = BUSPIRATE_SPI_DFLT_PON,
               .pullups = BUSPIRATE_SPI_DFLT_ENABLE_PULLUPS,
               .aux = BUSPIRATE_SPI_DFLT_AUX_ON,
               .cs_active = BUSPIRATE_SPI_DFLT_CS_ACTIVE,
               },
    .bus = {
            .cmd = CONFIG_SPI_BUS,
            .output_type = BUSPIRATE_SPI_DFLT_OUTPUT_TYPE,
            .clk_pol_idle = BUSPIRATE_SPI_DFLT_CLK_IDLE_POLARITY,
            .output_clk_edge = BUSPIRATE_SPI_DFLT_CLK_EDGE,
            .input_sample_end = BUSPIRATE_SPI_DFLT_SAMPLE,
            },
};

/* Commands while in SPI mode. Note, upper part of complete byte to fit
 * corresponding struct */
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
void bpspi_sendData(struct ddata *ddata, const uint8_t *data, int sz)
{
    int ret;
    uint16_t nsz_send, nsz_receive;
    uint8_t tmp[8] = { 0 };

    nsz_send = htons(sz);
    nsz_receive = htons(0);
    ASSERT(sz < 4096);          /* Primitive handling for now */

    LOGD("BP: Interface %s sending %d bytes \n", __func__, sz);

    tmp[0] = 0;
    ASSURE_E(write(ddata->fd, (uint8_t[]) {
                   CMD_WR_RD}, 1) != -1, LOGE_IOERROR(errno));
    ASSURE_E(write(ddata->fd, &nsz_send, 2) != -1, LOGE_IOERROR(errno));
    ASSURE_E(write(ddata->fd, &nsz_receive, 2) != -1, LOGE_IOERROR(errno));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSERT(tmp[0] == 0x00);

    ASSURE_E((ret = write(ddata->fd, data, sz)) != -1, LOGE_IOERROR(errno));
    LOGD("BP: %d bytes written to device\n", ret);
}

void bpspi_receiveData(struct ddata *ddata, uint8_t *data, int sz)
{
    int ret;
    uint16_t nsz_send, nsz_receive;
    uint8_t tmp[8] = { 0 };

    nsz_send = htons(0);
    nsz_receive = htons(sz);
    ASSERT(sz < 4096);          /* Primitive handling for now */

    LOGD("BP: Interface %s receiving %d bytes \n", __func__, sz);

    tmp[0] = 0;
    ASSURE_E(write(ddata->fd, (uint8_t[]) {
                   CMD_WR_RD}, 1) != -1, LOGE_IOERROR(errno));
    ASSURE_E(write(ddata->fd, &nsz_send, 2) != -1, LOGE_IOERROR(errno));
    ASSURE_E(write(ddata->fd, &nsz_receive, 2) != -1, LOGE_IOERROR(errno));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSERT(tmp[0] == 0x00);

    ASSURE_E((ret = read(ddata->fd, data, sz)) != -1, LOGE_IOERROR(errno));
    LOGD("BP: %d bytes read from device\n", ret);
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
    ASSERT(tmp[0] == 0x00);
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
        return (struct ddata *)memcpy(ddata, device->driver->ddata,
                                      sizeof(struct ddata));

    memcpy(&ddata->config, &dflt_config_SPI, sizeof(struct config_SPI));
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
