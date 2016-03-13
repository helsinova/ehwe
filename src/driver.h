/***************************************************************************
 *   Copyright (C) 2015 by Michael Ambrus                                  *
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
#ifndef driver_h
#define driver_h

#include <stdint.h>

struct device;
struct ddata;

typedef enum {
    E_UNKNOWN = -1,
    CONFIG_DRIVER_OK = 0,
    E_WRONG_ORDER,
    E_BAD_ROLE,
    E_BAD_VALUE
} config_etype_t;

struct configAPI_SPI {
    /* If second argument is NULL: access and affect directly (if permitted
     * by driver). */
    /* If second argument given: access/modify (struct ddata *) that is
     * effectuated upon call to configure */
    struct {
        config_etype_t(*speed) (int, struct ddata *);
        config_etype_t(*power_on) (int, struct ddata *);
        config_etype_t(*pullups) (int, struct ddata *);
        config_etype_t(*aux_on) (int, struct ddata *);
        config_etype_t(*cs_active) (int, struct ddata *);
        config_etype_t(*output_type) (int, struct ddata *);
        config_etype_t(*clk_pol_idle) (int, struct ddata *);
        config_etype_t(*output_clk_edge) (int, struct ddata *);
        config_etype_t(*input_sample_end) (int, struct ddata *);
    } set;
    struct {
        config_etype_t(*speed) (int *, struct ddata *);
        config_etype_t(*power_on) (int *, struct ddata *);
        config_etype_t(*pullups) (int *, struct ddata *);
        config_etype_t(*aux_on) (int *, struct ddata *);
        config_etype_t(*cs_active) (int *, struct ddata *);
        config_etype_t(*output_type) (int *, struct ddata *);
        config_etype_t(*clk_pol_idle) (int *, struct ddata *);
        config_etype_t(*output_clk_edge) (int *, struct ddata *);
        config_etype_t(*input_sample_end) (int *, struct ddata *);
    } get;
};

struct driverAPI {
    struct device *device;      /* Belongs to this device */
    struct ddata *ddata;        /* Device specific driver-data */
    struct device *odevice;     /* Owned by device */
    void (*sendData) (const uint8_t *data, int sz);
    void (*receiveData) (uint8_t *data, int sz);
    uint16_t (*getStatus) (uint16_t);
    int (*configure) (struct ddata * ddata);    /* Actuate configuration */

    /* Allocate and return pointer *a copy* to driver specific driver-data */
    struct ddata *(*newddata) (struct device);

    /* Standardized functions for configuring
       device and/or driver. May be NULL or only partly
       filled-in depending if driver allows
       run-rime configuration or not. Behavior,
       such as in order what should be done, is
       also mandated by implementation
     */

    union {
        struct configAPI_SPI cSPI;
    };
};

#endif                          /* driver_h */
