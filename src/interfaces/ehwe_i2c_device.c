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
/*
 * Device level i2c, i.e. chip level, abstraction
 *
 * This is a high abstraction level which builds on Nordics i2c API
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <assert.h>

#include <ehwe.h>
#include <ehwe_i2c_device.h>

/* Forward declaration of layout for device registers.
   Layout differs from IC to IC and is thus unknown to this
   abstraction-level
 */
struct devregs;

struct i2c_device_struct {
    /* HW-bus: I2C0-I2Cn */
    I2C_TypeDef *bus;

    /* 7-bit write address - read-address implicit */
    uint8_t addr;

    /* this-pointer */
    struct i2c_device_struct *self;

    /* Copy of IC:s all registers or NULL if not in sync. Handling of this
     * member is up to the device-driver and can be optional. */
    struct i2c_device_devregs *reg;
};

/* Creates a i2c-device instance, and returns handle to it. */
i2c_device_hndl i2c_device_open(I2C_TypeDef * bus, uint8_t addr)
{
    struct i2c_device_struct *i2c_device;

    i2c_device = malloc(sizeof(struct i2c_device_struct));

    /* Set this-pointer */
    i2c_device->self = i2c_device;

    /* bus-id for this instance. */
    i2c_device->bus = bus;

    /* Chip adress (7-bit form) */
    i2c_device->addr = addr;

    /* Register map, uninitialized */
    i2c_device->reg = NULL;

    return i2c_device;
}

/* Destroy i2c-device instance */
void i2c_device_close(i2c_device_hndl i2c_device)
{
    assert(i2c_device != NULL && "Error: Bad i2c-device descriptor");

    if (i2c_device->reg != NULL)
        free(i2c_device->reg);

    i2c_device->self = NULL;
    i2c_device->addr = 0xF7;    /* 8-bit magic number fo debugging memory leaks */
    i2c_device->reg = NULL;

    free(i2c_device);
}

void i2c_device_read_bytes(i2c_device_hndl i2c_device, uint8_t reg,
                           uint8_t *buf, uint8_t count)
{

    assert(i2c_device != NULL && "Error: Bad i2c-device descriptor");

    /* Send which register to start access, omit STOP */
    i2c_write(i2c_device->bus, i2c_device->addr, (uint8_t[]) {
              reg}, 1, 0);

    i2c_read(i2c_device->bus, i2c_device->addr, buf, count);
}

void i2c_device_write_bytes(i2c_device_hndl i2c_device, uint8_t reg,
                            uint8_t *buf, uint8_t count)
{

    assert(i2c_device != NULL && "Error: Bad i2c-device descriptor");

    /* Send which register to start access, omit STOP */
    i2c_write(i2c_device->bus, i2c_device->addr, (uint8_t[]) {
              reg}, 1, 0);

    i2c_write(i2c_device->bus, i2c_device->addr, buf, count, 1);
}

uint8_t i2c_device_read_uint8(i2c_device_hndl i2c_device, uint8_t reg)
{
    uint8_t val = 0;

    assert(i2c_device != NULL && "Error: Bad i2c-device descriptor");

    /* Send which register to access, omit STOP */
    i2c_write(i2c_device->bus, i2c_device->addr, (uint8_t[]) {
              reg}, 1, 0);

    i2c_read(i2c_device->bus, i2c_device->addr, &val, sizeof(val));

    return val;
}

uint16_t i2c_device_read_uint16(i2c_device_hndl i2c_device, uint8_t reg)
{
    uint16_t val = 0;
    uint8_t buf[2] = { 0 };

    assert(i2c_device != NULL && "Error: Bad i2c-device descriptor");

    /* Send which register to access, omit STOP */
    i2c_write(i2c_device->bus, i2c_device->addr, (uint8_t[]) {
              reg}, 1, 0);

    i2c_read(i2c_device->bus, i2c_device->addr, buf, sizeof(val));
    val = *(uint16_t *)buf;

    return val;
}

uint32_t i2c_device_read_uint32(i2c_device_hndl i2c_device, uint8_t reg)
{
    uint32_t val = 0;
    uint8_t buf[4] = { 0 };

    assert(i2c_device != NULL && "Error: Bad i2c-device descriptor");

    /* Send which register to access, omit STOP */
    i2c_write(i2c_device->bus, i2c_device->addr, (uint8_t[]) {
              reg}, 1, 0);

    i2c_read(i2c_device->bus, i2c_device->addr, buf, sizeof(val));
    val = *(uint32_t *)buf;

    return val;
}

void i2c_device_write_uint8(i2c_device_hndl i2c_device, uint8_t reg,
                            uint8_t val)
{
    assert(i2c_device != NULL && "Error: Bad i2c-device descriptor");

    /* Send which register to access directly followed by value */
    i2c_write(i2c_device->bus, i2c_device->addr, (uint8_t[]) {
              reg, val}, sizeof(val) + 1, 1);
}

void i2c_device_write_uint16(i2c_device_hndl i2c_device, uint8_t reg,
                             uint16_t val)
{
    uint8_t buf[sizeof(val) + 1];
    buf[0] = reg;
    *(uint16_t *)(&buf[1]) = val;

    assert(i2c_device != NULL && "Error: Bad i2c-device descriptor");

    assert(sizeof(buf) == 3);

    i2c_write(i2c_device->bus, i2c_device->addr, buf, sizeof(val) + 1, 1);
}

void i2c_device_write_uint32(i2c_device_hndl i2c_device, uint8_t reg,
                             uint32_t val)
{
    uint8_t buf[sizeof(val) + 1];
    buf[0] = reg;
    *(uint32_t *)(&buf[1]) = val;

    assert(i2c_device != NULL && "Error: Bad i2c-device descriptor");

    assert(sizeof(buf) == 5);

    i2c_write(i2c_device->bus, i2c_device->addr, buf, sizeof(val) + 1, 1);
}

/* Does nothing but is needed for linker not to optimize away functions */
int i2c_device_init_interface(const struct device *device)
{
    return 0;
}
