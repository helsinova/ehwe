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
#include <config.h>

#define REXP_ESTRSZ 80

// Some generic regex-i patterns
#define INDEX "[0-9]+"
#define ANYTHING ".*"
#define FILENAME "[[:alnum:] _/,.:-]*"

typedef enum {
    ROLE_UNDEFINED = 0,
    ROLE_INVALID = 1,           /* Invalid, unknown or parse failure */
    SPI,
    DSPI,
    QSPI,
    I2C,
    CAN
} role_t;

// Valid regex-i patterns for roles
#define ROLES "SPI|DSPI|QSPI|I2C|CAN"

typedef enum {
    DEV_UNDEFINED = 0,
    DEV_INVALID = 1,            /* Invalid, unknown or parse failure */
#ifdef DEVICE_PARAPORT
    PARAPORT = 100,
#endif
#ifdef DEVICE_BUSPIRATE
    BUSPIRATE = 101,
#endif
} devid_t;

// Valid regex-i patterns for devices
#define DEVICES "PP|BP"

// Valid regex-i patterns for "direction"
#define DIRECTIONS "MASTER|SLAVE"

#ifdef DEVICE_PARAPORT
#  include "paraport.h"
#endif
#ifdef DEVICE_BUSPIRATE
#  include "buspirate.h"
#endif

struct device {
    devid_t devid;
    role_t role;
    int index;
    union {
#ifdef DEVICE_PARAPORT
        struct paraport paraport;
#endif
#ifdef DEVICE_BUSPIRATE
        struct buspirate buspirate;
#endif
    };
};

int devices_init();
int devices_parse(const char *devstr, struct device *device);
int devices_init_device(const struct device *device);

#endif                          //devices_h
