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

/***************************************************************************
 * Driver interface
 ***************************************************************************/
void bpspi_sendData(const uint8_t *data, int sz)
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

void bpspi_receiveData(uint8_t *data, int sz)
{
}

uint16_t bpspi_getStatus(uint16_t flags)
{
    return flags;
}

int bpspi_config(struct ddata *ddata)
{
    int ret;
    uint8_t tmp[8] = { 0 };

    struct confspi_pereph *pereph = &(ddata->config.spi.pereph);
    struct confspi_speed *speed = &(ddata->config.spi.speed);
    struct confspi_bus *bus = &(ddata->config.spi.bus);

    ASSURE_E((ret =
              write(ddata->fd, speed, sizeof(struct confspi_speed))) != -1,
             LOGE_IOERROR(errno));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSURE(tmp[0] == 0x00);
    tmp[0] = 0;
    ASSURE_E((ret =
              write(ddata->fd, bus, sizeof(struct confspi_bus))) != -1,
             LOGE_IOERROR(errno));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSURE(tmp[0] == 0x01);
    tmp[0] = 0;
    ASSURE_E((ret =
              write(ddata->fd, pereph, sizeof(struct confspi_pereph))) != -1,
             LOGE_IOERROR(errno));
    ASSURE_E(read(ddata->fd, tmp, 1) != -1, LOGE_IOERROR(errno));
    ASSURE(tmp[0] == 0x01);
    tmp[0] = 0;
    return 0;
}
