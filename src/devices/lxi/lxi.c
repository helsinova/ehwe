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
#include "config.h"
#include "devices_config.h"
#include "lxi_config.h"
#include <sys/types.h>
#include <regex.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <log.h>
#include <devices.h>
#include <driver.h>
#include <lxi.h>
#include <string.h>
#include <stdlib.h>
#include <assure.h>
#include "local.h"

static regex_t preg;            /* Compiled regular expression for full
                                   device-string parsing */

/* Valid regex-i role patterns for lxi */
#define LXI_ROLES "SPI|I2C"

/* Valid regex-i clock-owner patterns for lxi */
#define LXI_CLKOWNER "MASTER|SLAVE"

#define REGEX_PATT \
  "^(" LXI_ROLES \
  "):(" INDEX \
  "):(" DEVICES \
  "):(" LXI_CLKOWNER \
  "):(" FILENAME \
")"

/* Driver companion - NOTE: unique for each driver. Must NOT be public */
struct ddata;

#define REGEX_NSUB (5+1)

/* Convenience variables: */
#ifdef LXI_ENABLE_SPI
/*    SPI driver */
static struct driverAPI_spi lxispi_driver = {
    .ddata = NULL,
    .sendData = NULL,           //lxispi_sendData,
    .sendrecieveData = lxispi_sendrecieveData,
    .sendrecieveData_ncs = NULL,    // lxispi_sendrecieveData_ncs,
    .setCS = NULL,              // lxispi_setCS,
    .receiveData = NULL,        // lxispi_receiveData,
    .getStatus = NULL,          // lxispi_getStatus,
    .actuate_config = NULL,     // lxispi_configure,
    .newddata = NULL,           // lxispi_newddata,
    .config = {
               .set = {
                       .speed = NULL,   // lxispi_set_speed,
                       .power_on = NULL,    // lxispi_set_power_on,
                       .pullups = NULL, // lxispi_set_pullups,
                       .aux_on = NULL,  // lxispi_set_aux_on,
                       .cs_active = NULL,   // lxispi_set_cs_active,
                       .output_type = NULL, // lxispi_set_output_type,
                       .clk_pol_idle = NULL,    // lxispi_set_clk_pol_idle,
                       .output_clk_edge = NULL, // lxispi_set_output_clk_edge,
                       .input_sample_end = NULL,    // lxispi_set_input_sample_end,
                       },
               .get = {
                       .speed = NULL,   // lxispi_get_speed,
                       .power_on = NULL,    // lxispi_get_power_on,
                       .pullups = NULL, // lxispi_get_pullups,
                       .aux_on = NULL,  // lxispi_get_aux_on,
                       .cs_active = NULL,   //lxispi_get_cs_active,
                       .output_type = NULL, // lxispi_get_output_type,
                       .clk_pol_idle = NULL,    // lxispi_get_clk_pol_idle,
                       .output_clk_edge = NULL, // lxispi_get_output_clk_edge,
                       .input_sample_end = NULL,    // lxispi_get_input_sample_end,
                       },
               },
};
#endif
#ifdef LXI_ENABLE_I2C
/*    I2C driver */
static struct driverAPI_i2c lxii2c_driver = {
    .ddata = NULL,
    .sendByte = lxii2c_sendByte,
    .receiveByte = NULL,        // lxii2c_receiveByte,
    .sendData = lxii2c_sendData,
    .receiveData = lxii2c_receiveData,
    .sendrecieveData = NULL,    // lxii2c_sendrecieveData,
    .start = lxii2c_start,
    .stop = lxii2c_stop,
    .autoAck = lxii2c_autoAck,
    .getStatus = NULL,          // lxii2c_getStatus,
    .actuate_config = NULL,     // lxii2c_configure,
    .newddata = lxii2c_newddata,
    .config = {
               .set = {
                       .speed = NULL,   // lxii2c_set_speed,
                       .power_on = NULL,    // lxii2c_set_power_on,
                       .pullups = NULL, // lxii2c_set_pullups,
                       .aux_on = NULL,  // lxii2c_set_aux_on,
                       .cs_active = NULL,   // lxii2c_set_cs_active,
                       },
               .get = {
                       .speed = NULL,   // lxii2c_get_speed,
                       .power_on = NULL,    // lxii2c_get_power_on,
                       .pullups = NULL, // lxii2c_get_pullups,
                       .aux_on = NULL,  // lxii2c_get_aux_on,
                       .cs_active = NULL,   // lxii2c_get_cs_active,
                       },
               },
};
#endif

/* CTOR-type code-init */
int lxi_init()
{
    int rc;
    char err_str[REXP_ESTRSZ];
    static int is_init = 0;

    if (is_init) {
        LOGW("No need to run %s twice, CTOR _init has run it?\n", __func__);
        return 0;
    }
    is_init = 1;

    rc = regcomp(&preg, REGEX_PATT, REG_EXTENDED | REG_ICASE);
    if (rc) {
        regerror(rc, &preg, err_str, REXP_ESTRSZ);
        LOGE("Regexec compilation error: %s\n", err_str);
        return rc;
    }
#if NOT_NOW
    ASSERT(sizeof(struct confspi_pereph) == 1);
    ASSERT(sizeof(struct confspi_speed) == 1);
    ASSERT(sizeof(struct confspi_bus) == 1);

    /* Testing bit-field structs for portability. Needed only once per
     * new compiler/target combo. */
#ifdef ENABLE_BITFIELD_TEST
#ifdef LXI_ENABLE_SPI
    {
        volatile struct confspi_pereph pereph = {
            .cmd = SPICMD_CONFIG_PEREPHERIALS,
            .power_on = 1,
            .pullups = 1,
            .aux = 0,
            .cs_active = 0
        };
        LOGW("Testing confspi_pereph bitfields: cmd=%d power_on=%d pullups=%d "
             "aux=%d cs_active=%d (raw=0x%02X)\n", pereph.cmd, pereph.power_on,
             pereph.pullups, pereph.aux, pereph.cs_active, pereph.raw);
        ASSERT(pereph.raw == 0x4C);

        struct confspi_speed speed = {
            .cmd = SPICMD_CONFIG_SPEED,
            .speed = SPISPEED_1MHz
        };
        LOGW("Testing confspi_speed bitfields: cmd=%d speed=%d (raw=0x%02X)\n",
             speed.cmd, speed.speed, speed.raw);
        ASSERT(speed.raw == 0x63);

        struct confspi_bus bus = {
            .cmd = SPICMD_CONFIG_BUS,
            .output_type = 1,
            .clk_pol_idle = 0,
            .output_clk_edge = 1,
            .input_sample_end = 0
        };

        LOGW("Tasting confspi_bus bitfields: cmd=%d output_type=%d "
             "clk_pol_idle=%d output_clk_edge=%d input_sample_end=%d "
             "(raw=0x%02X)\n", bus.cmd, bus.output_type, bus.clk_pol_idle,
             bus.output_clk_edge, bus.input_sample_end, bus.raw);
        ASSERT(bus.raw == 0x8A);
    }
#endif
#ifdef LXI_ENABLE_I2C
    {
        volatile struct confi2c_pereph pereph = {
            .cmd = I2CCMD_CONFIG_PEREPHERIALS,
            .power_on = 0,
            .pullups = 1,
            .aux = 1,
            .cs_active = 0
        };
        LOGW("Testing confi2c_pereph bitfields: cmd=%d power_on=%d pullups=%d "
             "aux=%d cs_active=%d (raw=0x%02X)\n", pereph.cmd, pereph.power_on,
             pereph.pullups, pereph.aux, pereph.cs_active, pereph.raw);
        ASSERT(pereph.raw == 0x46);

        struct confi2c_speed speed = {
            .cmd = I2CCMD_CONFIG_SPEED,
            .speed = I2CSPEED_100kHz
        };
        LOGW("Testing confi2c_speed bitfields: cmd=%d speed=%d (raw=0x%02X)\n",
             speed.cmd, speed.speed, speed.raw);
        ASSERT(speed.raw == 0x62);
    }
#endif
#endif

#endif                          //NOT_NOW
    return 0;
}

/*
 * Refined parsing of devstring to complete lxi dev_struct
 *
 * */
int lxi_parse(const char *devstr, struct device *device)
{
    int rc, i;
    char err_str[REXP_ESTRSZ];
    regmatch_t mtch_idxs[REGEX_NSUB];
    char *devstr_cpy = strdup(devstr);
    char *role_str;
    char *index_str;
    char *device_str;
    char *clkownr_str;
    char *filename_str;

    device->role = ROLE_INVALID;
    device->devid = DEV_INVALID;

    rc = regexec(&preg, devstr_cpy, REGEX_NSUB, mtch_idxs, 0);
    if (rc) {
        regerror(rc, &preg, err_str, REXP_ESTRSZ);
        LOGE("Regexec match error: %s\n", err_str);
        free(devstr_cpy);
        return rc;
    }
    /* Add string terminators in substrings */
    for (i = 1; i < REGEX_NSUB; i++) {
        ASSURE_E(mtch_idxs[i].rm_so != -1, goto lxi_parse_err);
        devstr_cpy[mtch_idxs[i].rm_eo] = 0;
    }

    role_str = &devstr_cpy[mtch_idxs[1].rm_so];
    index_str = &devstr_cpy[mtch_idxs[2].rm_so];
    device_str = &devstr_cpy[mtch_idxs[3].rm_so];
    clkownr_str = &devstr_cpy[mtch_idxs[4].rm_so];
    filename_str = &devstr_cpy[mtch_idxs[5].rm_so];

    LOGD("  Second level device-string parsing (by %s):\n", __func__);
    LOGD("    role=%s\n", role_str);
    LOGD("    index=%s\n", index_str);
    LOGD("    device=%s\n", device_str);
    LOGD("    clkownr=%s\n", clkownr_str);
    LOGD("    filename=%s\n", filename_str);

    ASSURE_E(strcasecmp(device_str, "lxi") == 0, goto lxi_parse_err);

    if (strcasecmp(role_str, "spi") == 0) {
        device->role = ROLE_SPI;
    } else if (strcasecmp(role_str, "i2c") == 0) {
        device->role = ROLE_I2C;
    } else {
        LOGE("LXI device driver can't handle role: %s\n", role_str);
        goto lxi_parse_err;
    }

    device->index = atoi(index_str);
    device->devid = LXI;
    device->lxi = malloc(sizeof(struct lxi));

    if (strcasecmp(clkownr_str, "master") == 0) {
        device->lxi->clckownr = MASTER;
    } else if (strcasecmp(clkownr_str, "slave") == 0) {
        device->lxi->clckownr = SLAVE;
    } else {
        LOGE("LXI device driver can't handle clkownr: %s\n", clkownr_str);
        goto lxi_parse_err;
    }

    /* Avoid need to strdup by using original which happens to terminate
     * correctly as well. Ignore const as this string belongs to
     * environment with process-long lifetime */
    device->lxi->filename = (char *)(&devstr[mtch_idxs[5].rm_so]);

    free(devstr_cpy);
    return 0;
lxi_parse_err:
    free(devstr_cpy);
    return -1;
}

int lxi_init_device(struct device *device)
{
    struct driverAPI_any *driver;
    struct ddata *ddata;

    LOGI("LXI: Initializing device ID [%d]\n", device->devid);

    ASSERT(driver = malloc(sizeof(struct driverAPI_spi)));
    switch (device->role) {
#ifdef LXI_ENABLE_SPI
        case ROLE_SPI:
            memcpy(driver, &lxispi_driver, sizeof(struct driverAPI_spi));
            ddata = lxispi_newddata(NULL);
            break;
#endif
#ifdef LXI_ENABLE_I2C
        case ROLE_I2C:
            memcpy(driver, &lxii2c_driver, sizeof(struct driverAPI_spi));
            ddata = lxii2c_newddata(NULL);
            break;
#endif
        default:
            LOGE("Role [%d] is not supported by LXI\n", device->role);
            return -1;
    }
    ASSURE((ddata->fd = open(device->lxi->filename, O_RDWR)) != -1);

    driver->ddata = ddata;
    driver->device = device;
    device->driver.any = driver;
    ddata->driver.any = driver;

    /* Configure the device to a known state as the state can't be read */
    switch (device->role) {
#ifdef LXI_ENABLE_SPI
        case ROLE_SPI:
            ASSURE(lxispi_configure(ddata) == 0);
            break;
#endif
#ifdef LXI_ENABLE_I2C
        case ROLE_I2C:
            ASSURE(lxii2c_configure(ddata) == 0);
            break;
#endif
        default:
            LOGE("Device LXI can't handle role %d (yet)\n", device->role);
            ASSURE("Bad device->role" == 0);
    }

    return 0;
}

int lxi_deinit_device(struct device *device)
{
    struct driverAPI_any *driver = device->driver.any;
    struct ddata *ddata = driver->ddata;
    struct lxi *lxi = device->lxi;

    close(ddata->fd);
    free(ddata);
    free(driver);
    free(lxi);

    device->driver.any = NULL;
    device->lxi = NULL;

    return 0;
}

/***************************************************************************
 * INIT/FINI mechanism
 ***************************************************************************/
#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

void __init __lxi_init(void)
{
    int rc;
#ifdef ENABLE_INITFINI_SHOWEXEC
    fprintf(stderr, ">>> Running module _init in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism ====\n");
#endif
    if ((rc = lxi_init())) {
        fprintf(stderr, "Fatal error: lxi_init() failed\n");
        exit(rc);
    }
}

void __fini __lxi_fini(void)
{
#ifdef ENABLE_INITFINI_SHOWEXEC
    fprintf(stderr, ">>> Running module _fini in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism\n");
#endif
}
