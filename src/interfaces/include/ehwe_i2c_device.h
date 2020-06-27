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
#ifndef ehwe_i2c_device_h
#define ehwe_i2c_device_h

#include <stdint.h>

/* Not strictly true as we will use Nordics higher-level API.
 * Include is a work-around for EHWE not separating them well enough. This
 * will change and might require a change of the include (TBD) */
#include <stm32f10x.h>

struct adapter;

/* Module initialization */
int i2c_device_init_interface(const struct adapter *device);

/* Forward declaration - hide unneeded details*/
struct i2c_device_struct;
typedef struct i2c_device_struct *i2c_device_hndl;

/* Open/close - Creates/destroys a i2c-device instance */
i2c_device_hndl i2c_device_open(I2C_TypeDef * bus, uint8_t addr);
void i2c_device_close(i2c_device_hndl i2c_device);

I2C_TypeDef *i2c_device_bus(i2c_device_hndl i2c_device);
uint8_t i2c_device_addr(i2c_device_hndl i2c_device);

/* Adapter access for byte-chunks */
void i2c_device_read_bytes(i2c_device_hndl i2c_device, uint8_t reg,
                           uint8_t *buf, uint8_t count);
void i2c_device_write_bytes(i2c_device_hndl i2c_device, uint8_t reg,
                            uint8_t *buf, uint8_t count);

/* Register-level access functions for 8,16 & 32-bit access */
uint8_t i2c_device_read_uint8(i2c_device_hndl, uint8_t reg);
uint16_t i2c_device_read_uint16(i2c_device_hndl, uint8_t reg);
uint32_t i2c_device_read_uint32(i2c_device_hndl, uint8_t reg);
void i2c_device_write_uint8(i2c_device_hndl, uint8_t reg, uint8_t val);
void i2c_device_write_uint16(i2c_device_hndl, uint8_t reg, uint16_t val);
void i2c_device_write_uint32(i2c_device_hndl, uint8_t reg, uint32_t val);

#endif                          //ehwe_i2c_device_h
