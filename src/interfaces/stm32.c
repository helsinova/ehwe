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

SPI_TypeDef *SPI_stm32_drv[MAX_SPI_INTERFACES];

static void nod_sendData(struct ddata *ddata, const uint8_t *data, int sz);
static void nod_receiveData(struct ddata *ddata, uint8_t *data, int sz);
static uint16_t nod_getStatus(struct ddata *ddata, uint16_t);
static void nod_sendrecieveData(struct ddata *ddata, const uint8_t *outbuf,
                                int outsz, uint8_t *indata, int insz);
static void nod_sendrecieveData_ncs(struct ddata *ddata, const uint8_t *outbuf,
                                    int outsz, uint8_t *indata, int insz);
static void nod_setCS(struct ddata *ddata, int state);

static struct driverAPI nodriverAPI = {
    .ddata = NULL,
    .sendData = nod_sendData,
    .receiveData = nod_receiveData,
    .getStatus = nod_getStatus,
    .sendrecieveData = nod_sendrecieveData,
    .sendrecieveData_ncs = nod_sendrecieveData_ncs,
    .setCS = nod_setCS
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
    for (i = 0; i < MAX_SPI_INTERFACES; i++) {
        SPI_stm32_drv[i] = &nodriverAPI;
    }

    return 0;
}

int stm32_init_interface(const struct device *device)
{
    int if_init = 0;

    switch (device->devid) {
#ifdef DEVICE_PARAPORT
        case PARAPORT:
            ASSERT("PARAPORT is TBD" == NULL);
            if_init = 1;
            break;
#endif
#ifdef DEVICE_BUSPIRATE
        case BUSPIRATE:
            switch (device->role) {
                case SPI:
                    ASSERT(device->index > 0
                           && device->index <= MAX_SPI_INTERFACES);
                    break;
                case I2C:
                    ASSERT(device->index > 0
                           && device->index <= MAX_I2C_INTERFACES);
                    break;
                default:
                    ASSERT("Role not supported for BUSPIRATE driver" == NULL);
            }

            SPI_stm32_drv[device->index - 1] = device->driver;
            if_init = 1;
            break;
#endif
        default:
            LOGE("Unsupported device [%d] in [%s]\n", device->devid, __func__);

    }
    ASSERT(if_init);
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
    struct ddata *ddata = SPIx->ddata;

    SPIx->sendData(ddata, &ldata, 1);
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
    uint8_t ldata;
    struct ddata *ddata = SPIx->ddata;

    SPIx->receiveData(ddata, &ldata, 1);
    return ldata;
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
    struct ddata *ddata = SPIx->ddata;
    FlagStatus bitstatus = SPIx->getStatus(ddata, SPI_I2S_FLAG);
    return bitstatus;
}

/***************************************************************************
 * These don't exist in stdperf but are added for testing ehwe             *
 ***************************************************************************/
void SPI_I2S_SendReceiveData(SPI_TypeDef * SPIx, const uint8_t *obuffer,
                             int osz, uint8_t *ibuffer, int isz)
{
    struct ddata *ddata = SPIx->ddata;
    SPIx->sendrecieveData(ddata, obuffer, osz, ibuffer, isz);

}

void SPI_I2S_SendReceiveData_ncs(SPI_TypeDef * SPIx, const uint8_t *obuffer,
                                 int osz, uint8_t *ibuffer, int isz)
{
    struct ddata *ddata = SPIx->ddata;
    SPIx->sendrecieveData_ncs(ddata, obuffer, osz, ibuffer, isz);

}

void SPI_I2S_SendDataArray(SPI_TypeDef * SPIx, const uint8_t *buffer, int sz)
{
    struct ddata *ddata = SPIx->ddata;
    SPIx->sendrecieveData(ddata, buffer, sz, NULL, 0);
}

void SPI_I2S_SendDataArray_ncs(SPI_TypeDef * SPIx, const uint8_t *buffer,
                               int sz)
{
    struct ddata *ddata = SPIx->ddata;
    SPIx->sendrecieveData_ncs(ddata, buffer, sz, NULL, 0);
}

void SPI_I2S_ReceiveDataArray(SPI_TypeDef * SPIx, uint8_t *buffer, int sz)
{
    struct ddata *ddata = SPIx->ddata;
    SPIx->sendrecieveData(ddata, NULL, 0, buffer, sz);
}

void SPI_I2S_ReceiveDataArray_ncs(SPI_TypeDef * SPIx, uint8_t *buffer, int sz)
{
    struct ddata *ddata = SPIx->ddata;
    SPIx->sendrecieveData_ncs(ddata, NULL, 0, buffer, sz);
}

void SPI_I2S_SetCS(SPI_TypeDef * SPIx, int state)
{
    struct ddata *ddata = SPIx->ddata;
    SPIx->setCS(ddata, state);
}

void SPI_I2S_SendData_ncs(SPI_TypeDef * SPIx, uint16_t Data)
{
    uint8_t ldata = Data;       /*Intentional truncation to 8-bit */
    struct ddata *ddata = SPIx->ddata;

    SPIx->sendrecieveData_ncs(ddata, &ldata, 1, NULL, 0);
}

uint16_t SPI_I2S_ReceiveData_ncs(SPI_TypeDef * SPIx)
{
    uint8_t ldata;
    struct ddata *ddata = SPIx->ddata;

    SPIx->sendrecieveData_ncs(ddata, NULL, 0, &ldata, 1);
    return ldata;
}

/***************************************************************************
 * No-driver stubs                                                         *
 ***************************************************************************/
static void nod_sendData(struct ddata *ddata, const uint8_t *data, int sz)
{
    int i;
    char cbuf[512] = { '\0' };

    LOGW("Interface-stub %s sending %d bytes \n", __func__, sz);
    for (i = 0; i < sz; i++) {
        if (data[i] > 31)
            sprintf(cbuf, "0x%02X %c,", data[i], data[i]);
        else
            sprintf(cbuf, "0x%02X %s,", data[i], " ");
    }
    LOGW("%s\n", cbuf);
}

static void nod_sendData_ncs(struct ddata *ddata, const uint8_t *data, int sz)
{
    int i;
    char cbuf[512] = { '\0' };

    LOGW("Interface-stub %s sending %d bytes (NO CS) \n", __func__, sz);
    for (i = 0; i < sz; i++) {
        if (data[i] > 31)
            sprintf(cbuf, "0x%02X %c,", data[i], data[i]);
        else
            sprintf(cbuf, "0x%02X %s,", data[i], " ");
    }
    LOGW("%s\n", cbuf);
}

static void nod_receiveData(struct ddata *ddata, uint8_t *data, int sz)
{
    LOGW("Interface %s is not supposed to run\n", __func__);
}

static void nod_setCS(struct ddata *ddata, int state)
{
    LOGW("Stubbed [%s] sets CS to: \n", __func__, state);
}

static uint16_t nod_getStatus(struct ddata *ddata, uint16_t flags)
{
    LOGW("Interface-stub %s quired about flags 0x02X% bytes \n", __func__,
         flags);
    return -1;
}

static void nod_sendrecieveData(struct ddata *ddata, const uint8_t *outbuf,
                                int outsz, uint8_t *indata, int insz)
{

    LOGW("Interface-stub %s sending 0x02X% bytes, receiving  0x02X% bytes\n",
         __func__, outsz, insz);
}

static void nod_sendrecieveData_ncs(struct ddata *ddata, const uint8_t *outbuf,
                                    int outsz, uint8_t *indata, int insz)
{

    LOGW("Interface-stub %s (NO CS) sending 0x02X% bytes, "
         "receiving  0x02X% bytes\n", __func__, outsz, insz);
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
