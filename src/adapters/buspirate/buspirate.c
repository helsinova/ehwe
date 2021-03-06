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
#include "config.h"
#include "adapters_config.h"
#include "buspirate_config.h"
#include "local.h"
#include <sys/types.h>
#include <regex.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <liblog/log.h>
#include <adapters.h>
#include <driver.h>
#include <buspirate.h>
#include <string.h>
#include <stdlib.h>
#include <liblog/assure.h>
#include <stermio/stermio.h>

static regex_t preg;            /* Compiled regular expression for full
                                   adapter-string parsing */

#define REGEX_PATT \
  "^(" BP_ROLES \
  "):(" INDEX \
  "):(" ADAPTERS \
  "):(" BP_CLKOWNER \
  "):(" FILENAME \
")"

/* Driver companion - NOTE: unique for each driver. Must NOT be public */
struct ddata;

#define REGEX_NSUB (5+1)

/* Convenience variables: */
#ifdef BUSPIRATE_ENABLE_SPI
/*    SPI driver */
static struct driverAPI_spi bpspi_driver = {
    .ddata = NULL,
    .sendData = bpspi_sendData,
    .sendrecieveData = bpspi_sendrecieveData,
    .sendrecieveData_ncs = bpspi_sendrecieveData_ncs,
    .setCS = bpspi_setCS,
    .receiveData = bpspi_receiveData,
    .getStatus = bpspi_getStatus,
    .actuate_config = bpspi_configure,
    .newddata = bpspi_newddata,
    .config = {
               .set = {
                       .speed = bpspi_set_speed,
                       .power_on = bpspi_set_power_on,
                       .pullups = bpspi_set_pullups,
                       .aux_on = bpspi_set_aux_on,
                       .cs_active = bpspi_set_cs_active,
                       .output_type = bpspi_set_output_type,
                       .clk_pol_idle = bpspi_set_clk_pol_idle,
                       .output_clk_edge = bpspi_set_output_clk_edge,
                       .input_sample_end = bpspi_set_input_sample_end,
                       },
               .get = {
                       .speed = bpspi_get_speed,
                       .power_on = bpspi_get_power_on,
                       .pullups = bpspi_get_pullups,
                       .aux_on = bpspi_get_aux_on,
                       .cs_active = bpspi_get_cs_active,
                       .output_type = bpspi_get_output_type,
                       .clk_pol_idle = bpspi_get_clk_pol_idle,
                       .output_clk_edge = bpspi_get_output_clk_edge,
                       .input_sample_end = bpspi_get_input_sample_end,
                       },
               },
};
#endif
#ifdef BUSPIRATE_ENABLE_I2C
/*    I2C driver */
static struct driverAPI_i2c bpi2c_driver = {
    .ddata = NULL,
    .sendByte = bpi2c_sendByte,
    .receiveByte = bpi2c_receiveByte,
    .sendData = bpi2c_sendData,
    .receiveData = bpi2c_receiveData,
    .sendrecieveData = bpi2c_sendrecieveData,
    .start = bpi2c_start,
    .stop = bpi2c_stop,
    .autoAck = bpi2c_autoAck,
    .getStatus = bpi2c_getStatus,
    .actuate_config = bpi2c_configure,
    .newddata = bpi2c_newddata,
    .config = {
               .set = {
                       .speed = bpi2c_set_speed,
                       .power_on = bpi2c_set_power_on,
                       .pullups = bpi2c_set_pullups,
                       .aux_on = bpi2c_set_aux_on,
                       .cs_active = bpi2c_set_cs_active,
                       },
               .get = {
                       .speed = bpi2c_get_speed,
                       .power_on = bpi2c_get_power_on,
                       .pullups = bpi2c_get_pullups,
                       .aux_on = bpi2c_get_aux_on,
                       .cs_active = bpi2c_get_cs_active,
                       },
               },
};
#endif

int buspirate_init()
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
    ASSERT(sizeof(struct confspi_pereph) == 1);
    ASSERT(sizeof(struct confspi_speed) == 1);
    ASSERT(sizeof(struct confspi_bus) == 1);

    /* Testing bit-field structs for portability. Needed only once per
     * new compiler/target combo. */
#ifdef ENABLE_BITFIELD_TEST
#ifdef BUSPIRATE_ENABLE_SPI
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
#ifdef BUSPIRATE_ENABLE_I2C
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

    return 0;
}

/*
 * Refined parsing of adapterstring to complete buspirate adapter_struct
 *
 * */
int buspirate_parse(const char *adapterstr, struct adapter *adapter)
{
    int rc, i;
    char err_str[REXP_ESTRSZ];
    regmatch_t mtch_idxs[REGEX_NSUB];
    char *adapterstr_cpy = strdup(adapterstr);
    char *role_str;
    char *index_str;
    char *adapter_str;
    char *clkownr_str;
    char *filename_str;

    adapter->role = ROLE_INVALID;
    adapter->devid = DEV_INVALID;

    rc = regexec(&preg, adapterstr_cpy, REGEX_NSUB, mtch_idxs, 0);
    if (rc) {
        regerror(rc, &preg, err_str, REXP_ESTRSZ);
        LOGE("Regexec match error: %s\n", err_str);
        free(adapterstr_cpy);
        return rc;
    }
    /* Add string terminators in substrings */
    for (i = 1; i < REGEX_NSUB; i++) {
        ASSURE_E(mtch_idxs[i].rm_so != -1, goto buspirate_parse_err);
        adapterstr_cpy[mtch_idxs[i].rm_eo] = 0;
    }

    role_str = &adapterstr_cpy[mtch_idxs[1].rm_so];
    index_str = &adapterstr_cpy[mtch_idxs[2].rm_so];
    adapter_str = &adapterstr_cpy[mtch_idxs[3].rm_so];
    clkownr_str = &adapterstr_cpy[mtch_idxs[4].rm_so];
    filename_str = &adapterstr_cpy[mtch_idxs[5].rm_so];

    LOGD("  Second level adapter-string parsing (by %s):\n", __func__);
    LOGD("    role=%s\n", role_str);
    LOGD("    index=%s\n", index_str);
    LOGD("    adapter=%s\n", adapter_str);
    LOGD("    clkownr=%s\n", clkownr_str);
    LOGD("    filename=%s\n", filename_str);

    ASSURE_E(strcasecmp(adapter_str, "bp") == 0, goto buspirate_parse_err);

    if (strcasecmp(role_str, "spi") == 0) {
        adapter->role = ROLE_SPI;
    } else if (strcasecmp(role_str, "i2c") == 0) {
        adapter->role = ROLE_I2C;
    } else {
        LOGE("Buspirate adapter driver can't handle role: %s\n", role_str);
        goto buspirate_parse_err;
    }

    adapter->index = atoi(index_str);
    adapter->devid = BUSPIRATE;
    adapter->buspirate = malloc(sizeof(struct buspirate));

    if (strcasecmp(clkownr_str, "master") == 0) {
        adapter->buspirate->clckownr = MASTER;
    } else if (strcasecmp(clkownr_str, "slave") == 0) {
        adapter->buspirate->clckownr = SLAVE;
    } else {
        LOGE("Buspirate adapter driver can't handle clkownr: %s\n", clkownr_str);
        goto buspirate_parse_err;
    }

    /* Avoid need to strdup by using original which happens to terminate
     * correctly as well. Ignore const as this string belongs to
     * environment with process-long lifetime */
    adapter->buspirate->name = (char *)(&adapterstr[mtch_idxs[5].rm_so]);

    free(adapterstr_cpy);
    return 0;
buspirate_parse_err:
    free(adapterstr_cpy);
    return -1;
}

int buspirate_init_adapter(struct adapter *adapter)
{
    struct driverAPI_any *driver;
    struct ddata *ddata;

    LOGI("BP: Initializing adapter ID [%d]\n", adapter->devid);

    ASSERT(driver = malloc(sizeof(struct driverAPI_spi)));
    switch (adapter->role) {
#ifdef BUSPIRATE_ENABLE_SPI
        case ROLE_SPI:
            memcpy(driver, &bpspi_driver, sizeof(struct driverAPI_spi));
            ddata = bpspi_newddata(NULL);
            break;
#endif
#ifdef BUSPIRATE_ENABLE_I2C
        case ROLE_I2C:
            memcpy(driver, &bpi2c_driver, sizeof(struct driverAPI_spi));
            ddata = bpi2c_newddata(NULL);
            break;
#endif
        default:
            LOGE("Role [%d] is not supported by Bus-Pirate\n", adapter->role);
            return -1;
    }

    ASSURE((ddata->fd =
            open(adapter->buspirate->name, O_RDWR | O_NONBLOCK)) != -1);

#ifdef HAVE_POSIX_TERMIO
	/* Make sure terminal is "good" for BP wrt speed etc, but also LF:s and
	 * what-not:s */
    stio_bp_terminal(ddata->fd);
#endif

    empty_inbuff(ddata->fd);
    driver->ddata = ddata;
    driver->adapter = adapter;
    adapter->driver.any = driver;
    ddata->driver.any = driver;

    ASSURE(rawMode_enter(adapter) == 0);

    switch (adapter->role) {
#ifdef BUSPIRATE_ENABLE_SPI
        case ROLE_SPI:
            ASSURE(rawMode_toMode(adapter, ENTER_SPI) == 0);
            break;
#endif
#ifdef BUSPIRATE_ENABLE_I2C
        case ROLE_I2C:
            ASSURE(rawMode_toMode(adapter, ENTER_I2C) == 0);
            break;
#endif
        default:
            LOGE("Adapter BusPirate can't handle role %d (yet)\n", adapter->role);
            ASSURE("Bad adapter->role" == 0);
    }
    close(ddata->fd);
    ASSURE((ddata->fd = open(adapter->buspirate->name, O_RDWR)) != -1);
#ifdef HAVE_POSIX_TERMIO
    stio_bp_raw(ddata->fd);
#endif

    LOGI("Adapter [%s] is now state-initialized and re-opened blocking r/w\n",
         adapter->buspirate->name);
    //setserial_bp(ddata->fd);

    /* Configure the adapter to a known state as the state can't be read */
    switch (adapter->role) {
#ifdef BUSPIRATE_ENABLE_SPI
        case ROLE_SPI:
            ASSURE(bpspi_configure(ddata) == 0);
            break;
#endif
#ifdef BUSPIRATE_ENABLE_I2C
        case ROLE_I2C:
            ASSURE(bpi2c_configure(ddata) == 0);
            break;
#endif
        default:
            LOGE("Adapter BusPirate can't handle role %d (yet)\n", adapter->role);
            ASSURE("Bad adapter->role" == 0);
    }

    return 0;
}

int buspirate_deinit_adapter(struct adapter *adapter)
{
    struct driverAPI_any *driver = adapter->driver.any;
    struct ddata *ddata = driver->ddata;
    struct buspirate *buspirate = adapter->buspirate;

    close(ddata->fd);
    ASSURE((ddata->fd =
            open(adapter->buspirate->name, O_RDWR | O_NONBLOCK)) != -1);
#ifdef HAVE_POSIX_TERMIO
    stio_bp_terminal(ddata->fd);
#endif

    LOGD("Adapter [%s] re-opened non-blocking r/w\n", adapter->buspirate->name);

    LOGI("BP: Destroying adapter ID [%d]\n", adapter->devid);
    empty_inbuff(ddata->fd);
    ASSURE(rawMode_toMode(adapter, ENTER_RESET) == 0);
    msleep(1);
    empty_inbuff(ddata->fd);
    ASSURE(rawMode_toMode(adapter, RESET_BUSPIRATE) == 0);
    msleep(100);

    close(ddata->fd);
    free(ddata);
    free(driver);
    free(buspirate);

    adapter->driver.any = NULL;
    adapter->buspirate = NULL;
    return 0;
}

/***************************************************************************
 * INIT/FINI mechanism
 ***************************************************************************/
#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

void __init __buspirate_init(void)
{
    int rc;
#ifdef ENABLE_INITFINI_SHOWEXEC
    fprintf(stderr, ">>> Running module _init in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism ====\n");
#endif
    if ((rc = buspirate_init())) {
        fprintf(stderr, "Fatal error: buspirate_init() failed\n");
        exit(rc);
    }
}

void __fini __buspirate_fini(void)
{
#ifdef ENABLE_INITFINI_SHOWEXEC
    fprintf(stderr, ">>> Running module _fini in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism\n");
#endif
}
