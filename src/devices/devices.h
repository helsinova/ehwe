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
#ifndef devices_h
#define devices_h
#ifdef PARAPORT
#include "paraport.h"
#endif
#ifdef BUSPIRATE
#include "buspirate.h"
#endif

typedef enum {
    SPI,
    DSPI,
    QSPI,
    I2C,
    CAN
} role_t;

// Valid patterns for roles
#define _ROLES "SPI|DSPI|QSPI|I2C|CAN"
#define _roles "spi|dspi|qspi|i2c|can"
#define ROLES _ROLES "|" _roles

typedef enum {
    DEV_UNDEFINED = 0,
    DEV_INVALID = 1,            /* Invalid, unknown or parse failure */
#ifdef PARAPORT
    PARAPORT = 100,
#endif
#ifdef BUSPIRATE
    BUSPIRATE = 101,
#endif
} devid_t;

// Valid patterns for devices
#define _DEVICES "PP|BP"
#define _devices "pp|bp"
#define DEVICES _DEVICES "|" _devices

typedef enum {
    DIR_UNDEFINED = 0,
    DIR_INVALID = 1,            /* Invalid, unknown or parse failure */
    MASTER = 100,
    SLAVE = 101
} dir_t;

// Valid patterns for "direction"
#define _DIRECTIONS "MASTER|SLAVE"
#define _directions "master|slave"
#define DIRECTIONS _DIRECTIONS "|" _directions

struct device {
    devid_t devid;
    role_t role;
    int number;
    dir_t direction;
    union {
#ifdef PARAPORT
        struct paraport paraport;
#endif
#ifdef BUSPIRATE
        struct buspirate buspirate;
#endif
    };
};

int devices_init();
int devices_parse(const char *devstr, struct device *device);

#endif                          //devices_h
