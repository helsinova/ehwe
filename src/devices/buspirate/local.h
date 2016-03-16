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
#ifndef buspirate_local_h
#define buspirate_local_h
/***************************************************************************
 * Module-local interface
 ***************************************************************************/
#include <log.h>
#include <inttypes.h>
#include <driver.h>

#define LOGV_IOERROR( X ) log_ioerror( X , LOG_LEVEL_VERBOSE )
#define LOGD_IOERROR( X ) log_ioerror( X , LOG_LEVEL_DEBUG )
#define LOGI_IOERROR( X ) log_ioerror( X , LOG_LEVEL_INFO )
#define LOGW_IOERROR( X ) log_ioerror( X , LOG_LEVEL_WARNING )
#define LOGE_IOERROR( X ) log_ioerror( X , LOG_LEVEL_ERROR )

/* electrical level */
typedef enum {
    POL_LOW = 0,
    POL_HIGH = 1
} pol_t;

/* pulse edges (note, LOGIC - not electrical) */
typedef enum {
    EDGE_FRONT = 0,
    EDGE_BACK = 1
} edge_t;

/* pulse phase */
typedef enum {
    PULSE_MIDDLE = 0,
    PULSE_END = 1
} phase_t;

/* pin electrical output type */
typedef enum {
    OPEN_DRAIN = 0,
    PUSH_PULL = 1
} pinout_t;

/* CMake built-ins*/
#define OFF 0
#define ON 1
/* End */

#define msleep( X ) \
	usleep( (X) * 1000 )

/* Commands in binary mode. Commands prefixed "ENTER" continues to further
 * submode. Note: Not all commands are defined here, only mode-changers (so
 * far). Full definition here:
 * http://dangerousprototypes.com/docs/Bitbang#Commands
 */
typedef enum {
    ENTER_RESET = 0x00,         /* Back to console mode */
    ENTER_SPI = 0x01,
    ENTER_I2C = 0x02,
    ENTER_UART = 0x03,
    ENTER_1WIRE = 0x04,
    ENTER_RAWWIRE = 0x05,
    ENTER_OPENOCD = 0x05,
    RESET_BUSPIRATE = 0x0F      /* Execute full reset cycle */
} bpcmd_raw_t;

/* Configuration commands while in SPI mode. Note, upper part of complete byte to fit
 * corresponding struct */
typedef enum {
    CONFIG_SPI_PEREPHERIALS = 0x04,
    CONFIG_SPI_SPEED = 0x05,
    CONFIG_SPI_BUS = 0x08
} bpconfigcmd_spi_t;

/* Enable (1) and disable (0) Bus Pirate peripherals and pins. */
struct confspi_pereph {
    union {
        struct {
#if defined(_BIT_FIELDS_HTOL)
            bpconfigcmd_spi_t cmd:4;    /* When used as cmd, 0100 = 0x04 */
            uint8_t power_on:1; /* Enable power on */
            uint8_t pullups:1;  /* Enable pull-up resistors */
            uint8_t aux:1;      /* Set AUX-pin */
            uint8_t cs_active:1;    /* CS pin state? */
#else
            uint8_t cs_active:1;
            uint8_t aux:1;
            uint8_t pullups:1;
            uint8_t power_on:1;
            bpconfigcmd_spi_t cmd:4;
#endif
        } __attribute__ ((packed));
        uint8_t raw;
    };
} __attribute__ ((packed));

typedef enum {
    SPISPEED_30kHz = 0x0,
    SPISPEED_125kHz = 0x1,
    SPISPEED_250kHz = 0x2,
    SPISPEED_1MHz = 0x3,
    SPISPEED_2MHz = 0x4,
    SPISPEED_2_6MHz = 0x5,
    SPISPEED_4MHz = 0x6,
    SPISPEED_8MHz = 0x7
} speed_t;

struct confspi_speed {
    union {
        struct {
#if defined(_BIT_FIELDS_HTOL)
            bpconfigcmd_spi_t cmd:5;    /* When used as cmd, 01100 = 0x05 */
            speed_t speed:3;
#else
            speed_t speed:3;
            bpconfigcmd_spi_t cmd:5;
#endif
        } __attribute__ ((packed));
        uint8_t raw;
    };

} __attribute__ ((packed));

struct confspi_bus {
    union {
        struct {
#if defined(_BIT_FIELDS_HTOL)
            bpconfigcmd_spi_t cmd:4;    /* When used as cmd, 1000 = 0x08 */
            pinout_t output_type:1; /* 0=HiZ, 1=3.3V */
            pol_t clk_pol_idle:1;   /* Clock polarity on idle: 0=low, 1=high */
            edge_t output_clk_edge:1;   /* MOSI on clock-edge: 0=front, 1=back */
            phase_t input_sample_end:1; /* MISO sample where: 0=middle, 1=end */
#else
            phase_t input_sample_end:1;
            edge_t output_clk_edge:1;
            pol_t clk_pol_idle:1;
            pinout_t output_type:1;
            bpconfigcmd_spi_t cmd:4;
#endif
        } __attribute__ ((packed));
        uint8_t raw;
    };

} __attribute__ ((packed));

struct config_SPI {
    struct confspi_pereph pereph;
    struct confspi_speed speed;
    struct confspi_bus bus;
};

/* Convenience-variable pre-set with build-system configuration */
extern struct config_SPI dflt_config_SPI;

/* Driver companion - NOTE: unique for each driver. Must NOT be public */
struct ddata {
    int fd;
    bpcmd_raw_t state;
    union {
        struct config_SPI spi;
    } config;
    struct driverAPI *odriver;  /* Owned by driver */
};

struct device;

void log_ioerror(int ecode, log_level llevel);
void empty_inbuff(int fd);
int rawMode_enter(struct device *);
int rawMode_toMode(struct device *, bpcmd_raw_t bpcmd);

/***************************************************************************
 * Main driver interfaces
 ***************************************************************************
 * SPI
 ***************************************************************************/
void bpspi_sendData(struct ddata *ddata, const uint8_t *data, int sz);

void bpspi_sendrecieveData(struct ddata *ddata, const uint8_t *outbuf,
                           int outsz, uint8_t *indata, int insz);
void bpspi_receiveData(struct ddata *ddata, uint8_t *data, int sz);
uint16_t bpspi_getStatus(struct ddata *ddata, uint16_t flags);
int bpspi_configure(struct ddata *ddata);
struct ddata *bpspi_newddata(struct device *device);
/***************************************************************************
 * I2C
 ***************************************************************************/
void bpi2c_sendData(struct ddata *ddata, const uint8_t *data, int sz);
void bpi2c_receiveData(struct ddata *ddata, uint8_t *data, int sz);
uint16_t bpi2c_getStatus(struct ddata *ddata, uint16_t flags);
int bpi2c_configure(struct ddata *ddata);
struct ddata *bpi2c_newddata(struct device *device);
/***************************************************************************
 * Configuration  interface
 ***************************************************************************
 * SPI
 ***************************************************************************/
config_etype_t bpspi_set_speed(int, struct ddata *);
config_etype_t bpspi_set_power_on(int, struct ddata *);
config_etype_t bpspi_set_pullups(int, struct ddata *);
config_etype_t bpspi_set_aux_on(int, struct ddata *);
config_etype_t bpspi_set_cs_active(int, struct ddata *);
config_etype_t bpspi_set_output_type(int, struct ddata *);
config_etype_t bpspi_set_clk_pol_idle(int, struct ddata *);
config_etype_t bpspi_set_output_clk_edge(int, struct ddata *);
config_etype_t bpspi_set_input_sample_end(int, struct ddata *);

config_etype_t bpspi_get_speed(int *, struct ddata *);
config_etype_t bpspi_get_power_on(int *, struct ddata *);
config_etype_t bpspi_get_pullups(int *, struct ddata *);
config_etype_t bpspi_get_aux_on(int *, struct ddata *);
config_etype_t bpspi_get_cs_active(int *, struct ddata *);
config_etype_t bpspi_get_output_type(int *, struct ddata *);
config_etype_t bpspi_get_clk_pol_idle(int *, struct ddata *);
config_etype_t bpspi_get_output_clk_edge(int *, struct ddata *);
config_etype_t bpspi_get_input_sample_end(int *, struct ddata *);
/***************************************************************************
 * I2C
 ***************************************************************************/
config_etype_t bpi2c_set_speed(int, struct ddata *);
config_etype_t bpi2c_set_power_on(int, struct ddata *);
config_etype_t bpi2c_set_pullups(int, struct ddata *);
config_etype_t bpi2c_set_aux_on(int, struct ddata *);
config_etype_t bpi2c_set_cs_active(int, struct ddata *);
config_etype_t bpi2c_set_output_type(int, struct ddata *);
config_etype_t bpi2c_set_clk_pol_idle(int, struct ddata *);
config_etype_t bpi2c_set_output_clk_edge(int, struct ddata *);
config_etype_t bpi2c_set_input_sample_end(int, struct ddata *);

config_etype_t bpi2c_get_speed(int *, struct ddata *);
config_etype_t bpi2c_get_power_on(int *, struct ddata *);
config_etype_t bpi2c_get_pullups(int *, struct ddata *);
config_etype_t bpi2c_get_aux_on(int *, struct ddata *);
config_etype_t bpi2c_get_cs_active(int *, struct ddata *);
config_etype_t bpi2c_get_output_type(int *, struct ddata *);
config_etype_t bpi2c_get_clk_pol_idle(int *, struct ddata *);
config_etype_t bpi2c_get_output_clk_edge(int *, struct ddata *);
config_etype_t bpi2c_get_input_sample_end(int *, struct ddata *);

#endif                          //buspirate_local_h
