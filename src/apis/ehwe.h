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
#ifndef ehwe_h
#define ehwe_h
#include <stdint.h>
#include <config.h>
#include <stm32f10x.h>

struct adapter;

int ehwe_init_api(const struct adapter *adapter);
void i2c_write(I2C_TypeDef * bus, uint8_t adapter_addr, const uint8_t *buffer,
               int len, int send_stop);
void i2c_read(I2C_TypeDef * bus, uint8_t adapter_addr, uint8_t *buffer, int len);

#endif                          //ehwe_h
