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
#ifndef stm32_h
#define stm32_h
#include <stdint.h>

struct stm32 {
    int dummy;
};

/* Forward declaration of 'struct interface' required to avoid mutual header
 * inclusion */
struct interface;

int stm32_init();
int stm32_init_interface(const struct device *device);

/* STM32F10x_StdPeriph_Lib_V3.5.0 API */

/* Stubbies */
enum {SPI1, SPI2, SPI3} SPIn;

/* Real API */
typedef enum {RESET = 0, SET = !RESET} FlagStatus, ITStatus;
typedef int SPI_TypeDef;

void SPI_I2S_SendData(SPI_TypeDef * SPIx, uint16_t Data);
uint16_t SPI_I2S_ReceiveData(SPI_TypeDef * SPIx);
FlagStatus SPI_I2S_GetFlagStatus(SPI_TypeDef * SPIx, uint16_t SPI_I2S_FLAG);

#endif                          //stm32_h
