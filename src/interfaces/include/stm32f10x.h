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
#ifndef stm32f10x_h
#define stm32f10x_h
#include <stdint.h>

/* STM32F10x_StdPeriph_Lib_V3.5.0 API */

/* Glue
 * Note: may be fewer depending on MPU variant.
 * TBD: SPI and I2C are mutually exclusive. I.e. shared resources. Implement
 * logic to handle that sum does not exceed shared number of shared
 * resources */
#define MAX_SPI_INTERFACES 3
#define MAX_I2C_INTERFACES 3

struct driverAPI_spi;
struct driverAPI_i2c;
typedef struct driverAPI_spi SPI_TypeDef;
typedef struct driverAPI_i2c I2C_TypeDef;

extern SPI_TypeDef *SPI_stm32_drv[MAX_SPI_INTERFACES];
extern I2C_TypeDef *I2C_stm32_drv[MAX_I2C_INTERFACES];

#define SPI1 (SPI_stm32_drv[0])
#define SPI2 (SPI_stm32_drv[1])
#define SPI3 (SPI_stm32_drv[2])

#define I2C1 (I2C_stm32_drv[0])
#define I2C2 (I2C_stm32_drv[1])
#define I2C3 (I2C_stm32_drv[2])

/* Real API */

#define SPI_I2S_FLAG_RXNE               ((uint16_t)0x0001)
#define SPI_I2S_FLAG_TXE                ((uint16_t)0x0002)
#define I2S_FLAG_CHSIDE                 ((uint16_t)0x0004)
#define I2S_FLAG_UDR                    ((uint16_t)0x0008)
#define SPI_FLAG_CRCERR                 ((uint16_t)0x0010)
#define SPI_FLAG_MODF                   ((uint16_t)0x0020)
#define SPI_I2S_FLAG_OVR                ((uint16_t)0x0040)
#define SPI_I2S_FLAG_BSY                ((uint16_t)0x0080)

#define  I2C_Direction_Transmitter      ((uint8_t)0x00)
#define  I2C_Direction_Receiver         ((uint8_t)0x01)

#define I2C_FLAG_DUALF                  ((uint32_t)0x00800000)
#define I2C_FLAG_SMBHOST                ((uint32_t)0x00400000)
#define I2C_FLAG_SMBDEFAULT             ((uint32_t)0x00200000)
#define I2C_FLAG_GENCALL                ((uint32_t)0x00100000)
#define I2C_FLAG_TRA                    ((uint32_t)0x00040000)
#define I2C_FLAG_BUSY                   ((uint32_t)0x00020000)
#define I2C_FLAG_MSL                    ((uint32_t)0x00010000)

#define I2C_EVENT_MASTER_MODE_SELECT                      ((uint32_t)0x00030001)
#define I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED        ((uint32_t)0x00070082)
#define I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED           ((uint32_t)0x00030002)
#define I2C_EVENT_MASTER_MODE_ADDRESS10                   ((uint32_t)0x00030008)
#define I2C_EVENT_MASTER_BYTE_RECEIVED                    ((uint32_t)0x00030040)
#define I2C_EVENT_MASTER_BYTE_TRANSMITTING                ((uint32_t)0x00070080)
#define I2C_EVENT_MASTER_BYTE_TRANSMITTED                 ((uint32_t)0x00070084)
#define I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED          ((uint32_t)0x00020002)
#define I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED       ((uint32_t)0x00060082)
#define I2C_EVENT_SLAVE_RECEIVER_SECONDADDRESS_MATCHED    ((uint32_t)0x00820000)
#define I2C_EVENT_SLAVE_TRANSMITTER_SECONDADDRESS_MATCHED ((uint32_t)0x00860080)
#define I2C_EVENT_SLAVE_GENERALCALLADDRESS_MATCHED        ((uint32_t)0x00120000)
#define I2C_EVENT_SLAVE_BYTE_RECEIVED                     ((uint32_t)0x00020040)
#define I2C_EVENT_SLAVE_STOP_DETECTED                     ((uint32_t)0x00000010)
#define I2C_EVENT_SLAVE_BYTE_TRANSMITTED                  ((uint32_t)0x00060084)
#define I2C_EVENT_SLAVE_BYTE_TRANSMITTING                 ((uint32_t)0x00060080)
#define I2C_EVENT_SLAVE_ACK_FAILURE                       ((uint32_t)0x00000400)

#define IS_I2C_EVENT(EVENT) ( \
    ((EVENT) == I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED) || \
    ((EVENT) == I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED) || \
    ((EVENT) == I2C_EVENT_SLAVE_TRANSMITTER_SECONDADDRESS_MATCHED) || \
    ((EVENT) == I2C_EVENT_SLAVE_RECEIVER_SECONDADDRESS_MATCHED) || \
    ((EVENT) == I2C_EVENT_SLAVE_GENERALCALLADDRESS_MATCHED) || \
    ((EVENT) == I2C_EVENT_SLAVE_BYTE_RECEIVED) || \
    ((EVENT) == (I2C_EVENT_SLAVE_BYTE_RECEIVED | I2C_FLAG_DUALF)) || \
    ((EVENT) == (I2C_EVENT_SLAVE_BYTE_RECEIVED | I2C_FLAG_GENCALL)) || \
    ((EVENT) == I2C_EVENT_SLAVE_BYTE_TRANSMITTED) || \
    ((EVENT) == (I2C_EVENT_SLAVE_BYTE_TRANSMITTED | I2C_FLAG_DUALF)) || \
    ((EVENT) == (I2C_EVENT_SLAVE_BYTE_TRANSMITTED | I2C_FLAG_GENCALL)) || \
    ((EVENT) == I2C_EVENT_SLAVE_STOP_DETECTED) || \
    ((EVENT) == I2C_EVENT_MASTER_MODE_SELECT) || \
    ((EVENT) == I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) || \
    ((EVENT) == I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) || \
    ((EVENT) == I2C_EVENT_MASTER_BYTE_RECEIVED) || \
    ((EVENT) == I2C_EVENT_MASTER_BYTE_TRANSMITTED) || \
    ((EVENT) == I2C_EVENT_MASTER_BYTE_TRANSMITTING) || \
    ((EVENT) == I2C_EVENT_MASTER_MODE_ADDRESS10) || \
    ((EVENT) == I2C_EVENT_SLAVE_ACK_FAILURE) \
)

typedef enum { RESET = 0, SET = !RESET } FlagStatus, ITStatus;
typedef enum { ERROR = 0, SUCCESS = !ERROR } ErrorStatus;
typedef enum { DISABLE = 0, ENABLE = !DISABLE } FunctionalState;

/* SPI / I2S */
void SPI_I2S_SendData(SPI_TypeDef * SPIx, uint16_t Data);
uint16_t SPI_I2S_ReceiveData(SPI_TypeDef * SPIx);
FlagStatus SPI_I2S_GetFlagStatus(SPI_TypeDef * SPIx, uint16_t SPI_I2S_FLAG);

/* I2C */
void I2C_GenerateSTART(I2C_TypeDef * I2Cx, FunctionalState NewState);
void I2C_GenerateSTOP(I2C_TypeDef * I2Cx, FunctionalState NewState);
void I2C_AcknowledgeConfig(I2C_TypeDef * I2Cx, FunctionalState NewState);
void I2C_Send7bitAddress(I2C_TypeDef * I2Cx, uint8_t Address,
                         uint8_t I2C_Direction);
void I2C_SendData(I2C_TypeDef * I2Cx, uint8_t Data);
uint8_t I2C_ReceiveData(I2C_TypeDef * I2Cx);
ErrorStatus I2C_CheckEvent(I2C_TypeDef * I2Cx, uint32_t I2C_EVENT);
void I2C_ClearFlag(I2C_TypeDef * I2Cx, uint32_t I2C_FLAG);
FlagStatus I2C_GetFlagStatus(I2C_TypeDef * I2Cx, uint32_t I2C_FLAG);

/***************************************************************************
 * These don't exist in stdperf but are added for testing ehwe             *
 ***************************************************************************/
void SPI_I2S_SendData_ncs(SPI_TypeDef * SPIx, uint16_t Data);
uint16_t SPI_I2S_ReceiveData_ncs(SPI_TypeDef * SPIx);

void SPI_I2S_SendReceiveData(SPI_TypeDef * SPIx, const uint8_t *obuffer,
                             int osz, uint8_t *ibuffer, int isz);
void SPI_I2S_SendDataArray(SPI_TypeDef * SPIx, const uint8_t *buffer, int sz);
void SPI_I2S_SendDataArray_ncs(SPI_TypeDef * SPIx, const uint8_t *buffer,
                               int sz);
void SPI_I2S_ReceiveDataArray(SPI_TypeDef * SPIx, uint8_t *buffer, int sz);
void SPI_I2S_ReceiveDataArray_ncs(SPI_TypeDef * SPIx, uint8_t *buffer, int sz);
/* Special case. This exists in stdperf but is a GPIO. Needs refined API
 * (TBD)*/
void SPI_I2S_SetCS(SPI_TypeDef * SPIx, int state);

#endif                          //stm32f10x_h
