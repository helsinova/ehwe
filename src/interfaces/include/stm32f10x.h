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

/* Glue */
#define MAX_SPI_INTERFACES 3
#define MAX_I2C_INTERFACES 3
struct driverAPI;
typedef struct driverAPI SPI_TypeDef;

extern SPI_TypeDef *SPI_stm32_drv[MAX_SPI_INTERFACES];
#define SPI1 (SPI_stm32_drv[0])
#define SPI2 (SPI_stm32_drv[1])
#define SPI3 (SPI_stm32_drv[2])

/* Real API */

#define SPI_I2S_FLAG_RXNE               ((uint16_t)0x0001)
#define SPI_I2S_FLAG_TXE                ((uint16_t)0x0002)
#define I2S_FLAG_CHSIDE                 ((uint16_t)0x0004)
#define I2S_FLAG_UDR                    ((uint16_t)0x0008)
#define SPI_FLAG_CRCERR                 ((uint16_t)0x0010)
#define SPI_FLAG_MODF                   ((uint16_t)0x0020)
#define SPI_I2S_FLAG_OVR                ((uint16_t)0x0040)
#define SPI_I2S_FLAG_BSY                ((uint16_t)0x0080)

typedef enum { RESET = 0, SET = !RESET } FlagStatus, ITStatus;

void SPI_I2S_SendData(SPI_TypeDef * SPIx, uint16_t Data);
uint16_t SPI_I2S_ReceiveData(SPI_TypeDef * SPIx);
FlagStatus SPI_I2S_GetFlagStatus(SPI_TypeDef * SPIx, uint16_t SPI_I2S_FLAG);

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
