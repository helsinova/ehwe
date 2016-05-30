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

/* Driver abstract type common to all drivers */
struct driverAPI_any {
    /*------------ Data -----------*/
    struct device *odevice;     /* Belongs to this device */
    struct ddata *ddata;        /* Device specific Driver-Data */
};

struct configAPI_spi {
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

struct driverAPI_spi {
    /*------------ Data -----------*/
    struct device *odevice;     /* Belongs to this device */
    struct ddata *ddata;        /* Device specific Driver-Data */

    /*---------- Methods ----------*/
    void (*sendData) (struct ddata * ddata, const uint8_t *data, int sz);
    void (*receiveData) (struct ddata * ddata, uint8_t *data, int sz);

    /* First send then directly receive data. Toggle CS in start and
       beginning of operation.
     */
    void (*sendrecieveData) (struct ddata * ddata, const uint8_t *outbuf,
                             int outsz, uint8_t *indata, int insz);

    /* As above but don't toggle CS */
    void (*sendrecieveData_ncs) (struct ddata * ddata, const uint8_t *outbuf,
                                 int outsz, uint8_t *indata, int insz);

    /* Sets state of CS signal. Note: state is logical value, not electrical
     */
    void (*setCS) (struct ddata * ddata, int state);

    uint16_t (*getStatus) (struct ddata * ddata, uint16_t);
    int (*actuate_config) (struct ddata * ddata);   /* Actuate configuration */

    /* Allocate and return a pointer with a copy* of driver specific
       driver-data unless device is NULL, in which case content is built-in
       defaults
     */
    struct ddata *(*newddata) (struct device * device);

    /* Standardized functions for configuring device and/or driver.
       Functions may be NULL or only partly filled-in depending if driver
       allows run-rime configuration or not. Behavior, such as in order what
       should be done, is also mandated by implementation
     */
    struct configAPI_spi config;
};

struct configAPI_i2c {
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
    } set;
    struct {
        config_etype_t(*speed) (int *, struct ddata *);
        config_etype_t(*power_on) (int *, struct ddata *);
        config_etype_t(*pullups) (int *, struct ddata *);
        config_etype_t(*aux_on) (int *, struct ddata *);
        config_etype_t(*cs_active) (int *, struct ddata *);
    } get;
};

struct driverAPI_i2c {
    /*------------ Data -----------*/
    struct device *odevice;     /* Belongs to this device */
    struct ddata *ddata;        /* Device specific Driver-Data */

    /*---------- Methods ----------*/
    void (*receiveByte) (struct ddata * ddata, uint8_t *data);
    void (*sendByte) (struct ddata * ddata, uint8_t data);
    void (*sendData) (struct ddata * ddata, const uint8_t *data, int sz);
    void (*receiveData) (struct ddata * ddata, uint8_t *data, int sz);

    /* First send then directly receive data. Toggle CS in start and
       beginning of operation.
     */
    void (*sendrecieveData) (struct ddata * ddata, const uint8_t *outbuf,
                             int outsz, uint8_t *indata, int insz);

    /* Send bus-start/-stop condition
     */
    void (*start) (struct ddata * ddata);
    void (*stop) (struct ddata * ddata);

    /* If each should be automatically ACKed or not
     */
    void (*autoAck) (struct ddata * ddata, int state);

    uint16_t (*getStatus) (struct ddata * ddata, uint16_t);
    int (*actuate_config) (struct ddata * ddata);   /* Actuate configuration */

    /* Allocate and return a pointer with a copy* of driver specific
       driver-data unless device is NULL, in which case content is built-in
       defaults
     */
    struct ddata *(*newddata) (struct device * device);

    /* Standardized functions for configuring device and/or driver.
       Functions may be NULL or only partly filled-in depending if driver
       allows run-rime configuration or not. Behavior, such as in order what
       should be done, is also mandated by implementation
     */
    struct configAPI_i2c config;
};

#endif                          /* driver_h */
