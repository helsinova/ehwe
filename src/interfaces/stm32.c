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
#include <log.h>
#include "interfaces.h"
#include "stm32.h"
#include <stm32f10x.h>
#include "devices.h"
#include "driver.h"
#include <string.h>
#include <stdlib.h>
#include <assure.h>

SPI_TypeDef *SPI_stm32_drv[MAX_SPI_DRIVERS];

static void nod_sendData(const uint8_t *data, int sz);
static void nod_receiveData(uint8_t *data, int sz);
static uint16_t nod_getStatus();

struct driverAPI nodriverAPI = {
    .sendData = nod_sendData,
    .receiveData = nod_receiveData,
    .getStatus = nod_getStatus
};

/***************************************************************************
 * Module stuff                                                            *
 ***************************************************************************/
int stm32_init()
{
    int i;
    static int is_init = 0;

    if (is_init) {
        LOGW("No need to run %s twice, CTOR _init has run it?\n", __func__);
        return 0;
    }
    is_init = 1;
    for (i = 0; i < MAX_SPI_DRIVERS; i++) {
        SPI_stm32_drv[i] = &nodriverAPI;
    }

    return 0;
}

int stm32_init_interface(const struct device *device)
{
    LOGW("Unfinished function [%s] (TBD) for device ID [%d]\n", __func__,
         device->devid);
    return 0;
}

/***************************************************************************
 * STM32F10x_StdPeriph_Lib_V3.5.0 API                                      *
 ***************************************************************************/

/**
  * @brief  Transmits a Data through the SPIx/I2Sx peripheral.
  * @param  SPIx: where x can be
  *   - 1, 2 or 3 in SPI mode
  *   - 2 or 3 in I2S mode
  * @param  Data : Data to be transmitted.
  * @retval None
  */
void SPI_I2S_SendData(SPI_TypeDef * SPIx, uint16_t Data)
{
    uint8_t ldata = Data;       /*Intentional truncation to 8-bit */
    SPIx->sendData(&ldata, 1);
}

/**
  * @brief  Returns the most recent received data by the SPIx/I2Sx peripheral.
  * @param  SPIx: where x can be
  *   - 1, 2 or 3 in SPI mode
  *   - 2 or 3 in I2S mode
  * @retval The value of the received data.
  */
uint16_t SPI_I2S_ReceiveData(SPI_TypeDef * SPIx)
{

    return 0;
}

/**
  * @brief  Checks whether the specified SPI/I2S flag is set or not.
  * @param  SPIx: where x can be
  *   - 1, 2 or 3 in SPI mode
  *   - 2 or 3 in I2S mode
  * @param  SPI_I2S_FLAG: specifies the SPI/I2S flag to check.
  *   This parameter can be one of the following values:
  *     @arg SPI_I2S_FLAG_TXE: Transmit buffer empty flag.
  *     @arg SPI_I2S_FLAG_RXNE: Receive buffer not empty flag.
  *     @arg SPI_I2S_FLAG_BSY: Busy flag.
  *     @arg SPI_I2S_FLAG_OVR: Overrun flag.
  *     @arg SPI_FLAG_MODF: Mode Fault flag.
  *     @arg SPI_FLAG_CRCERR: CRC Error flag.
  *     @arg I2S_FLAG_UDR: Underrun Error flag.
  *     @arg I2S_FLAG_CHSIDE: Channel Side flag.
  * @retval The new state of SPI_I2S_FLAG (SET or RESET).
  */
FlagStatus SPI_I2S_GetFlagStatus(SPI_TypeDef * SPIx, uint16_t SPI_I2S_FLAG)
{
    FlagStatus bitstatus = SET;
    return bitstatus;
}

/***************************************************************************
 * No-driver stubs                                                         *
 ***************************************************************************/
static void nod_sendData(const uint8_t *data, int sz)
{
    int i;
    char cbuf[512] = { '\0' };

    LOGD("Interface-stub %s sending %d bytes \n", __func__, sz);
    for (i = 0; i < sz; i++) {
        if (data[i] > 31)
            sprintf(cbuf, "0x%02X %c,", data[i], data[i]);
        else
            sprintf(cbuf, "0x%02X %c,", data[i], " ");
    }
    LOGD("%s\n", cbuf);
}

static void nod_receiveData(uint8_t *data, int sz)
{
    LOGE("Interface %s is not supposed to run\n", __func__);
}

static uint16_t nod_getStatus()
{
    LOGE("Interface %s is not supposed to run\n", __func__);
    return -1;
}

/***************************************************************************
 *   INIT/FINI CTOR/DTOR mechanism                                         *
 ***************************************************************************/
#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

void __init __stm32_init(void)
{
    int rc;
#ifdef INITFINI_SHOW
    fprintf(stderr, ">>> Running module _init in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism ====\n");
#endif
    if ((rc = stm32_init())) {
        fprintf(stderr, "Fatal error: stm32_init() failed\n");
        exit(rc);
    }
}

void __fini __stm32_fini(void)
{
#ifdef INITFINI_SHOW
    fprintf(stderr, ">>> Running module _fini in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism\n");
#endif
}
