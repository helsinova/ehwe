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
#ifndef interfaces_h
#define interfaces_h
#include <config.h>

struct device;

typedef enum {
    IN_UNDEFINED = 0,
    IN_INVALID = 1,             /* Invalid, unknown or parse failure */
#ifdef INTERFACE_STM32
    STM32 = 101,
#endif
} intrfc_t;

#ifdef INTERFACE_STM32
#  include "stm32.h"
#endif

struct interface {
    intrfc_t inid;
    int index;
    union {
#ifdef INTERFACE_STM32
        struct stm32 stm32;
#endif
    };
};

int interfaces_init();
int interfaces_init_interface(const struct device *device);

#endif                          //interfaces_h
