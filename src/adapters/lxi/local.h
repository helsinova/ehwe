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
#ifndef lxi_local_h
#define lxi_local_h
/***************************************************************************
 * Module-local api
 ***************************************************************************/
#include <liblog/log.h>
#include <inttypes.h>
#include <driver.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define LOGV_IOERROR( X ) log_ioerror( X , LOG_LEVEL_VERBOSE )
#define LOGD_IOERROR( X ) log_ioerror( X , LOG_LEVEL_DEBUG )
#define LOGI_IOERROR( X ) log_ioerror( X , LOG_LEVEL_INFO )
#define LOGW_IOERROR( X ) log_ioerror( X , LOG_LEVEL_WARNING )
#define LOGE_IOERROR( X ) log_ioerror( X , LOG_LEVEL_ERROR )

/* CMake built-ins*/
#define OFF 0
#define ON 1
/* End */

#define msleep( X ) \
    usleep( (X) * 1000 )

/* SPI HW-api behaviour & GPIO config. Enable (1) and disable (0) */
struct confspi_pereph {
    union {
        uint8_t raw;
    };
} __attribute__ ((packed));

/* SPI HW-api behaviour & GPIO config. Enable (1) and disable (0) */
struct confi2c_pereph {
    union {
        uint8_t raw;
    };
} __attribute__ ((packed));

struct confspi_speed {
    union {
        uint8_t raw;
    };

} __attribute__ ((packed));

struct confi2c_speed {
    union {
        uint8_t raw;
    };

} __attribute__ ((packed));

struct confspi_bus {
    union {
        uint8_t raw;
    };

} __attribute__ ((packed));

struct config_SPI {
    struct confspi_pereph pereph;
    struct confspi_speed speed;
    struct confspi_bus bus;
};

struct config_I2C {
    int autoAck;
    struct confi2c_pereph pereph;
    struct confi2c_speed speed;
};

/* As Linux API has no ability to control bus-details, we need to wait with
 * execution while package is built up. Final executor is the function:
 * lxii2c_stop
 */
struct lxi_i2c {
    unsigned char *inbuf, *outbuf;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg msg[2];

    void (*func_0) (void);      /* Just invoked function is used as state.
                                   Dummy function lxii2c_state_free is used as
                                   indication that no session is pending
                                   or initiated yet */
};

/* TBD */
struct lxi_spi {
    int dummy;
};

/* Convenience-variable pre-set with build-system configuration */
extern struct config_SPI lxi_dflt_config_SPI;

/* Driver companion - NOTE: unique for each driver. Must NOT be public */
struct ddata {
    int fd;
    union {
        struct config_I2C i2c;
        struct config_SPI spi;
    } config;
    /* Owned by driver */
    union {
        struct driverAPI_any *any;
        struct driverAPI_spi *spi;
        struct driverAPI_i2c *i2c;
    } driver;

    /* Linux driver accompanying state variables */
    union {
        struct lxi_spi spi;
        struct lxi_i2c i2c;
    } lxi_state;

};

struct adapter;

/***************************************************************************
 * Main driver apis
 ***************************************************************************
 * SPI
 ***************************************************************************/
void lxispi_setCS(struct ddata *ddata, int state);
void lxispi_receiveData(struct ddata *ddata, uint8_t *data, int sz);
void lxispi_sendData(struct ddata *ddata, const uint8_t *data, int sz);
uint16_t lxispi_getStatus(struct ddata *ddata, uint16_t flags);
int lxispi_configure(struct ddata *ddata);
struct ddata *lxispi_newddata(struct adapter *adapter);
/* High level */
void lxispi_sendrecieveData(struct ddata *ddata, const uint8_t *outbuf,
                            int outsz, uint8_t *indata, int insz);
void lxispi_sendrecieveData_ncs(struct ddata *ddata, const uint8_t *outbuf,
                                int outsz, uint8_t *indata, int insz);
/***************************************************************************
 * I2C
 ***************************************************************************/
void lxii2c_start(struct ddata *ddata);
void lxii2c_stop(struct ddata *ddata);
void lxii2c_autoAck(struct ddata *ddata, int state);
void lxii2c_receiveByte(struct ddata *ddata, uint8_t *data);
int lxii2c_sendByte(struct ddata *ddata, uint8_t data);
void lxii2c_receiveData(struct ddata *ddata, uint8_t *data, int sz);
void lxii2c_sendData(struct ddata *ddata, const uint8_t *data, int sz);
uint16_t lxii2c_getStatus(struct ddata *ddata, uint16_t flags);
int lxii2c_configure(struct ddata *ddata);
struct ddata *lxii2c_newddata(struct adapter *adapter);
/* High level */
void lxii2c_sendrecieveData(struct ddata *ddata, const uint8_t *outbuf,
                            int outsz, uint8_t *indata, int insz);
/***************************************************************************
 * Configuration  api
 ***************************************************************************
 * SPI
 ***************************************************************************/
config_etype_t lxispi_set_speed(int, struct ddata *);
config_etype_t lxispi_set_power_on(int, struct ddata *);
config_etype_t lxispi_set_pullups(int, struct ddata *);
config_etype_t lxispi_set_aux_on(int, struct ddata *);
config_etype_t lxispi_set_cs_active(int, struct ddata *);
config_etype_t lxispi_set_output_type(int, struct ddata *);
config_etype_t lxispi_set_clk_pol_idle(int, struct ddata *);
config_etype_t lxispi_set_output_clk_edge(int, struct ddata *);
config_etype_t lxispi_set_input_sample_end(int, struct ddata *);

config_etype_t lxispi_get_speed(int *, struct ddata *);
config_etype_t lxispi_get_power_on(int *, struct ddata *);
config_etype_t lxispi_get_pullups(int *, struct ddata *);
config_etype_t lxispi_get_aux_on(int *, struct ddata *);
config_etype_t lxispi_get_cs_active(int *, struct ddata *);
config_etype_t lxispi_get_output_type(int *, struct ddata *);
config_etype_t lxispi_get_clk_pol_idle(int *, struct ddata *);
config_etype_t lxispi_get_output_clk_edge(int *, struct ddata *);
config_etype_t lxispi_get_input_sample_end(int *, struct ddata *);
/***************************************************************************
 * I2C
 ***************************************************************************/
config_etype_t lxii2c_set_speed(int, struct ddata *);
config_etype_t lxii2c_set_power_on(int, struct ddata *);
config_etype_t lxii2c_set_pullups(int, struct ddata *);
config_etype_t lxii2c_set_aux_on(int, struct ddata *);
config_etype_t lxii2c_set_cs_active(int, struct ddata *);

config_etype_t lxii2c_get_speed(int *, struct ddata *);
config_etype_t lxii2c_get_power_on(int *, struct ddata *);
config_etype_t lxii2c_get_pullups(int *, struct ddata *);
config_etype_t lxii2c_get_aux_on(int *, struct ddata *);
config_etype_t lxii2c_get_cs_active(int *, struct ddata *);

#endif                          //lxi_local_h
