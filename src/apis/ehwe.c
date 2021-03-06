/***************************************************************************
 *   Copyright (C) 2016 by Michael Ambrus                                  *
 *   ambrmi09@gmail.com                                                    *
 *   Copyright (C) 2018 by Michael Ambrus                                  *
 *   ambrmi09@helsinova.se                                                 *
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
/*
 * High level apis modeled after Nordic's API
 *
 * Note: For many cases you'd want a higher level of control and write your
 * own corresponding high-level functions
 */
#include <sys/types.h>
#include <liblog/log.h>
#include "apis.h"
#include "stm32.h"
#include <stm32f10x.h>
#include "adapters.h"
#include "driver.h"
#include <string.h>
#include <stdlib.h>
#include <liblog/assure.h>
#include "../adapters/buspirate/local.h"

#define DDATA( B ) (B->ddata)
#define DD( B ) (DDATA(B)->driver.i2c)
#define DEV( B ) (DD(B)->adapter)
#define WRITE_ADDR( A ) (A<<1)
#define READ_ADDR( A ) ((A<<1) | 0x01)

void i2c_write(I2C_TypeDef * bus, uint8_t adapter_addr, const uint8_t *buffer,
               int len, int send_stop)
{
    int ack;

    assert(adapter_addr < 0x80);
    assert(DEV(bus)->role == ROLE_I2C);

    /* Send START condition */
    DD(bus)->start(DDATA(bus));

    /* Send adapter address for write */
    ack = DD(bus)->sendByte(DDATA(bus), WRITE_ADDR(adapter_addr));
    assert(ack == 1);

    /* Send the rest - if there is any */
    if (len > 0) {
        assert(buffer != NULL);
        DD(bus)->sendData(DDATA(bus), buffer, len);
    }

    if (send_stop) {
        /* Close Communication */
        DD(bus)->stop(DDATA(bus));
    }
}

void i2c_read(I2C_TypeDef * bus, uint8_t adapter_addr, uint8_t *buffer, int len)
{
    int ack;

    assert(adapter_addr < 0x80);
    assert(DEV(bus)->role == ROLE_I2C);

    /* Send START condition */
    DD(bus)->start(DDATA(bus));

    /* Send IC address for read */
    ack = DD(bus)->sendByte(DDATA(bus), READ_ADDR(adapter_addr));
    assert(ack == 1);

    /* Read all but the last with auto ACK */
    DD(bus)->receiveData(DDATA(bus), buffer, len);

    /* Close Communication */
    DD(bus)->stop(DDATA(bus));
}

int ehwe_init_api(const struct adapter *adapter)
{
    return 0;
}
