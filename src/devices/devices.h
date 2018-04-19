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
    ROLE_SPI,
    ROLE_DSPI,
    ROLE_QSPI,
    ROLE_I2C,
    ROLE_CAN
} role_t;

// Valid regex-i patterns for roles
#define ROLES "SPI|DSPI|QSPI|I2C|CAN"

typedef enum {
    DEV_UNDEFINED = 0,
    DEV_INVALID = 1,            /* Invalid, unknown or parse failure */
    PARAPORT = 100,
    BUSPIRATE = 101,
} devid_t;

/* Note: Not all devices can have variations in clock-owners */
typedef enum {
    CLKOWNR_UNDEFINED = 0,
    CLKOWNR_INVALID = 1,        /* Invalid, unknown or parse failure */
    MASTER = 100,
    SLAVE = 101
} clkownr_t;

// Valid regex-i patterns for devices
#define DEVICES "PP|BP"

// Valid regex-i patterns for "direction"
#define DIRECTIONS "MASTER|SLAVE"

struct driverAPI_spi;
struct driverAPI_i2c;
struct driverAPI_any;

/* Forward declared Device specific substructures */
struct paraport;
struct buspirate;

struct device {
    devid_t devid;
    role_t role;
    int index;
    union {
        struct paraport *paraport;
        struct buspirate *buspirate;
    };
    union {
        struct driverAPI_any *any;
        struct driverAPI_spi *spi;
        struct driverAPI_i2c *i2c;
    } driver;
};

int devices_init();
int devices_parse(const char *devstr, struct device *device);
int devices_init_device(struct device *device);
int devices_deinit_device(struct device *device);

#endif                          //devices_h
