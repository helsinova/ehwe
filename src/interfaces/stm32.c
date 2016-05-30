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
I2C_TypeDef *I2C_stm32_drv[MAX_I2C_INTERFACES];

static void nod_sendData(struct ddata *ddata, const uint8_t *data, int sz);
static void nod_receiveData(struct ddata *ddata, uint8_t *data, int sz);
static uint16_t nod_getStatus(struct ddata *ddata, uint16_t);
static void nod_sendrecieveData(struct ddata *ddata, const uint8_t *outbuf,
                                int outsz, uint8_t *indata, int insz);
static void nod_sendrecieveData_ncs(struct ddata *ddata, const uint8_t *outbuf,
                                    int outsz, uint8_t *indata, int insz);
static void nod_setCS(struct ddata *ddata, int state);

static struct driverAPI_spi nodriverAPI_spi = {
    .ddata = NULL,
    .sendData = nod_sendData,
    .receiveData = nod_receiveData,
    .getStatus = nod_getStatus,
    .sendrecieveData = nod_sendrecieveData,
    .sendrecieveData_ncs = nod_sendrecieveData_ncs,
    .setCS = nod_setCS
};

static struct driverAPI_i2c nodriverAPI_i2c = {
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
        SPI_stm32_drv[i] = &nodriverAPI_spi;
    }
    for (i = 0; i < MAX_I2C_INTERFACES; i++) {
        I2C_stm32_drv[i] = &nodriverAPI_i2c;
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
                    SPI_stm32_drv[device->index - 1] = device->driver.spi;
                    break;
                case I2C:
                    ASSERT(device->index > 0
                           && device->index <= MAX_I2C_INTERFACES);
                    I2C_stm32_drv[device->index - 1] = device->driver.i2c;
                    break;
                default:
                    ASSERT("Role not supported for BUSPIRATE driver" == NULL);
            }

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
/*--------------------------------------------------------------------------
 * SPI or I2S (slave mode I2C) API
 *-------------------------------------------------------------------------*/

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

/*--------------------------------------------------------------------------
 * I2C API
 *-------------------------------------------------------------------------*/
/**
  * @brief  Generates I2Cx communication START condition.
  * @param  I2Cx: where x can be 1 or 2 to select the I2C peripheral.
  * @param  NewState: new state of the I2C START condition generation.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None.
  */
void I2C_GenerateSTART(I2C_TypeDef * I2Cx, FunctionalState NewState)
{
}

/**
  * @brief  Generates I2Cx communication STOP condition.
  * @param  I2Cx: where x can be 1 or 2 to select the I2C peripheral.
  * @param  NewState: new state of the I2C STOP condition generation.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None.
  */
void I2C_GenerateSTOP(I2C_TypeDef * I2Cx, FunctionalState NewState)
{
}

/**
  * @brief  Enables or disables the specified I2C acknowledge feature.
  * @param  I2Cx: where x can be 1 or 2 to select the I2C peripheral.
  * @param  NewState: new state of the I2C Acknowledgement.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None.
  */
void I2C_AcknowledgeConfig(I2C_TypeDef * I2Cx, FunctionalState NewState)
{
}

/**
  * @brief  Transmits the address byte to select the slave device.
  * @param  I2Cx: where x can be 1 or 2 to select the I2C peripheral.
  * @param  Address: specifies the slave address which will be transmitted
  * @param  I2C_Direction: specifies whether the I2C device will be a
  *   Transmitter or a Receiver. This parameter can be one of the following values
  *     @arg I2C_Direction_Transmitter: Transmitter mode
  *     @arg I2C_Direction_Receiver: Receiver mode
  * @retval None.
  */
void I2C_Send7bitAddress(I2C_TypeDef * I2Cx, uint8_t Address,
                         uint8_t I2C_Direction)
{
}

/**
  * @brief  Sends a data byte through the I2Cx peripheral.
  * @param  I2Cx: where x can be 1 or 2 to select the I2C peripheral.
  * @param  Data: Byte to be transmitted..
  * @retval None
  */
void I2C_SendData(I2C_TypeDef * I2Cx, uint8_t Data)
{
}

/**
  * @brief  Returns the most recent received data by the I2Cx peripheral.
  * @param  I2Cx: where x can be 1 or 2 to select the I2C peripheral.
  * @retval The value of the received data.
  */
uint8_t I2C_ReceiveData(I2C_TypeDef * I2Cx)
{
    return 0;
}

/**
  * @brief  Checks whether the last I2Cx Event is equal to the one passed
  *   as parameter.
  * @param  I2Cx: where x can be 1 or 2 to select the I2C peripheral.
  * @param  I2C_EVENT: specifies the event to be checked.
  *   This parameter can be one of the following values:
  *     @arg I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED           : EV1
  *     @arg I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED              : EV1
  *     @arg I2C_EVENT_SLAVE_TRANSMITTER_SECONDADDRESS_MATCHED     : EV1
  *     @arg I2C_EVENT_SLAVE_RECEIVER_SECONDADDRESS_MATCHED        : EV1
  *     @arg I2C_EVENT_SLAVE_GENERALCALLADDRESS_MATCHED            : EV1
  *     @arg I2C_EVENT_SLAVE_BYTE_RECEIVED                         : EV2
  *     @arg (I2C_EVENT_SLAVE_BYTE_RECEIVED | I2C_FLAG_DUALF)      : EV2
  *     @arg (I2C_EVENT_SLAVE_BYTE_RECEIVED | I2C_FLAG_GENCALL)    : EV2
  *     @arg I2C_EVENT_SLAVE_BYTE_TRANSMITTED                      : EV3
  *     @arg (I2C_EVENT_SLAVE_BYTE_TRANSMITTED | I2C_FLAG_DUALF)   : EV3
  *     @arg (I2C_EVENT_SLAVE_BYTE_TRANSMITTED | I2C_FLAG_GENCALL) : EV3
  *     @arg I2C_EVENT_SLAVE_ACK_FAILURE                           : EV3_2
  *     @arg I2C_EVENT_SLAVE_STOP_DETECTED                         : EV4
  *     @arg I2C_EVENT_MASTER_MODE_SELECT                          : EV5
  *     @arg I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED            : EV6
  *     @arg I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED               : EV6
  *     @arg I2C_EVENT_MASTER_BYTE_RECEIVED                        : EV7
  *     @arg I2C_EVENT_MASTER_BYTE_TRANSMITTING                    : EV8
  *     @arg I2C_EVENT_MASTER_BYTE_TRANSMITTED                     : EV8_2
  *     @arg I2C_EVENT_MASTER_MODE_ADDRESS10                       : EV9
  *
  * @note: For detailed description of Events, please refer to section
  *    I2C_Events in stm32f10x_i2c.h file.
  *
  * @retval An ErrorStatus enumeration value:
  * - SUCCESS: Last event is equal to the I2C_EVENT
  * - ERROR: Last event is different from the I2C_EVENT
  */
ErrorStatus I2C_CheckEvent(I2C_TypeDef * I2Cx, uint32_t I2C_EVENT)
{
    return SUCCESS;
}

/**
  * @brief  Clears the I2Cx's pending flags.
  * @param  I2Cx: where x can be 1 or 2 to select the I2C peripheral.
  * @param  I2C_FLAG: specifies the flag to clear.
  *   This parameter can be any combination of the following values:
  *     @arg I2C_FLAG_SMBALERT: SMBus Alert flag
  *     @arg I2C_FLAG_TIMEOUT: Timeout or Tlow error flag
  *     @arg I2C_FLAG_PECERR: PEC error in reception flag
  *     @arg I2C_FLAG_OVR: Overrun/Underrun flag (Slave mode)
  *     @arg I2C_FLAG_AF: Acknowledge failure flag
  *     @arg I2C_FLAG_ARLO: Arbitration lost flag (Master mode)
  *     @arg I2C_FLAG_BERR: Bus error flag
  *
  * @note
  *   - STOPF (STOP detection) is cleared by software sequence: a read operation
  *     to I2C_SR1 register (I2C_GetFlagStatus()) followed by a write operation
  *     to I2C_CR1 register (I2C_Cmd() to re-enable the I2C peripheral).
  *   - ADD10 (10-bit header sent) is cleared by software sequence: a read
  *     operation to I2C_SR1 (I2C_GetFlagStatus()) followed by writing the
  *     second byte of the address in DR register.
  *   - BTF (Byte Transfer Finished) is cleared by software sequence: a read
  *     operation to I2C_SR1 register (I2C_GetFlagStatus()) followed by a
  *     read/write to I2C_DR register (I2C_SendData()).
  *   - ADDR (Address sent) is cleared by software sequence: a read operation to
  *     I2C_SR1 register (I2C_GetFlagStatus()) followed by a read operation to
  *     I2C_SR2 register ((void)(I2Cx->SR2)).
  *   - SB (Start Bit) is cleared software sequence: a read operation to I2C_SR1
  *     register (I2C_GetFlagStatus()) followed by a write operation to I2C_DR
  *     register  (I2C_SendData()).
  * @retval None
  */
void I2C_ClearFlag(I2C_TypeDef * I2Cx, uint32_t I2C_FLAG)
{
}

/**
  * @brief  Checks whether the specified I2C flag is set or not.
  * @param  I2Cx: where x can be 1 or 2 to select the I2C peripheral.
  * @param  I2C_FLAG: specifies the flag to check.
  *   This parameter can be one of the following values:
  *     @arg I2C_FLAG_DUALF: Dual flag (Slave mode)
  *     @arg I2C_FLAG_SMBHOST: SMBus host header (Slave mode)
  *     @arg I2C_FLAG_SMBDEFAULT: SMBus default header (Slave mode)
  *     @arg I2C_FLAG_GENCALL: General call header flag (Slave mode)
  *     @arg I2C_FLAG_TRA: Transmitter/Receiver flag
  *     @arg I2C_FLAG_BUSY: Bus busy flag
  *     @arg I2C_FLAG_MSL: Master/Slave flag
  *     @arg I2C_FLAG_SMBALERT: SMBus Alert flag
  *     @arg I2C_FLAG_TIMEOUT: Timeout or Tlow error flag
  *     @arg I2C_FLAG_PECERR: PEC error in reception flag
  *     @arg I2C_FLAG_OVR: Overrun/Underrun flag (Slave mode)
  *     @arg I2C_FLAG_AF: Acknowledge failure flag
  *     @arg I2C_FLAG_ARLO: Arbitration lost flag (Master mode)
  *     @arg I2C_FLAG_BERR: Bus error flag
  *     @arg I2C_FLAG_TXE: Data register empty flag (Transmitter)
  *     @arg I2C_FLAG_RXNE: Data register not empty (Receiver) flag
  *     @arg I2C_FLAG_STOPF: Stop detection flag (Slave mode)
  *     @arg I2C_FLAG_ADD10: 10-bit header sent flag (Master mode)
  *     @arg I2C_FLAG_BTF: Byte transfer finished flag
  *     @arg I2C_FLAG_ADDR: Address sent flag (Master mode) "ADSL"
  *   Address matched flag (Slave mode)"ENDA"
  *     @arg I2C_FLAG_SB: Start bit flag (Master mode)
  * @retval The new state of I2C_FLAG (SET or RESET).
  */
FlagStatus I2C_GetFlagStatus(I2C_TypeDef * I2Cx, uint32_t I2C_FLAG)
{
    return RESET;
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
#ifdef ENABLE_INITFINI_SHOWEXEC
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
#ifdef ENABLE_INITFINI_SHOWEXEC
    fprintf(stderr, ">>> Running module _fini in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism\n");
#endif
}
