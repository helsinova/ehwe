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
/* Note about buffers needed consideration if used multithreaded or with
 * non-Nordic or EHWE high level API:s (i2c.h / i2c-dev.h):
 *
 * - Out-buffer is always deep-copied. I.e. it's consequently malloc and
 *   freed here.
 *
 * - In-buffer and byte result never is. I.e. it's up to the caller to
 *   handle release of memory.
 */
#include "config.h"
#include "lxi_config.h"
#include "local.h"
#include <sys/types.h>
#include <regex.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <log.h>
#include <adapters.h>
#include <driver.h>
#include <string.h>
#include <stdlib.h>
#include <assure.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#define OUT_MSG     0
#define IN_MSG      1

#define TO_LXI_STATE( F ) ((void(*) (void))(lxii2c_ ##F))
#define RW_ADDR( A ) (A>>1)     /* Convert (back) to 7-bit address */

struct config_I2C lxi_dflt_config_I2C = {
    .autoAck = ON,
    .speed = {
              .raw = 0xC1,
              },
    .pereph = {
               .raw = 0xC2,
               },
};

/* Dummy function to indicate no-session pending */
static void lxii2c_state_free(void)
{
}

#define AUTOACK (ddata->config.i2c.autoAck)
/***************************************************************************
 * Main driver interface
 ***************************************************************************/
void lxii2c_autoAck(struct ddata *ddata, int state)
{
    ASSURE("LXI not supported function invoked" == NULL);

    AUTOACK = state;
}

void
lxii2c_sendrecieveData(struct ddata *ddata, const uint8_t *obuf,
                       int osz, uint8_t *ibuf, int isz)
{
    ASSURE("Work in progress (WIP)" == NULL);
}

void lxii2c_stop(struct ddata *ddata)
{
    int i;

    if ((ddata->lxi_state.i2c.func_0 == TO_LXI_STATE(sendByte)) ||
        (ddata->lxi_state.i2c.func_0 == TO_LXI_STATE(sendData))) {
        /* Simple case. Just send is intended */
        ASSURE(ddata->lxi_state.i2c.msg[OUT_MSG].addr < 0x80);

        ddata->lxi_state.i2c.packets.msgs = ddata->lxi_state.i2c.msg;
        ddata->lxi_state.i2c.packets.nmsgs = 1;
    } else if ((ddata->lxi_state.i2c.func_0 == TO_LXI_STATE(receiveByte)) ||
               (ddata->lxi_state.i2c.func_0 == TO_LXI_STATE(receiveData))) {
        /* Receive is intended */
        ASSURE(ddata->lxi_state.i2c.msg[IN_MSG].addr < 0x80);
        ASSURE(ddata->lxi_state.i2c.msg[OUT_MSG].addr < 0x80);
        ASSURE(ddata->lxi_state.i2c.msg[IN_MSG].addr ==
               ddata->lxi_state.i2c.msg[OUT_MSG].addr);

        ddata->lxi_state.i2c.packets.msgs = ddata->lxi_state.i2c.msg;
        ddata->lxi_state.i2c.packets.nmsgs = 2;
    }

    /* Invoke i2c-session in kernel */
    ASSURE(ioctl(ddata->fd, I2C_RDWR, &ddata->lxi_state.i2c.packets) >= 0);

    /* Epilogue: Reset state */
    if (ddata->lxi_state.i2c.outbuf) {
        free(ddata->lxi_state.i2c.outbuf);
        ddata->lxi_state.i2c.outbuf = NULL;
    }
#ifdef NEVER
    /* Inbuf is deliberately not freed here as stop is invoked after get-data */
    if (ddata->lxi_state.i2c.inbuf) {
        free(ddata->lxi_state.i2c.inbuf);
        ddata->lxi_state.i2c.inbuf = NULL;
    }
#endif                          //NEVER
    for (i = 0; i < 1; i++) {
        ddata->lxi_state.i2c.msg[i].addr = 0xFF;
        ddata->lxi_state.i2c.msg[i].len = 0;
        ddata->lxi_state.i2c.msg[i].buf = NULL;
        ddata->lxi_state.i2c.msg[i].flags = 0;
    }

    ddata->lxi_state.i2c.func_0 = TO_LXI_STATE(state_free);
}

void lxii2c_start(struct ddata *ddata)
{
    if ((ddata->lxi_state.i2c.func_0 == TO_LXI_STATE(sendByte)) ||
        (ddata->lxi_state.i2c.func_0 == TO_LXI_STATE(sendData))) {
        /*I.e. if re-start, read is implied next. */
        ddata->lxi_state.i2c.msg[IN_MSG].addr =
            ddata->lxi_state.i2c.msg[OUT_MSG].addr;
    }

    ddata->lxi_state.i2c.func_0 = TO_LXI_STATE(start);
}

/* Send one byte */
int lxii2c_sendByte(struct ddata *ddata, uint8_t data)
{
    /* Read adapter-ID detection */
    if ((ddata->lxi_state.i2c.func_0 == TO_LXI_STATE(start))
        && (ddata->lxi_state.i2c.msg[OUT_MSG].addr != 0xFF)) {
        /* This is an adapter read address. Nothing to do as address is
           already put in place, except check that address is sane. */

        ASSURE((data & 0x01) && "Full-length read addresses must be odd");
        ASSURE(RW_ADDR(data) == ddata->lxi_state.i2c.msg[OUT_MSG].addr);

        ddata->lxi_state.i2c.func_0 = TO_LXI_STATE(sendByte);
        return 1;
    }

    if (ddata->lxi_state.i2c.func_0 == TO_LXI_STATE(sendByte)) {
        ASSURE(ddata->lxi_state.i2c.outbuf);
        ddata->lxi_state.i2c.msg[OUT_MSG].addr =
            RW_ADDR(ddata->lxi_state.i2c.outbuf[0]);
    }

    if (ddata->lxi_state.i2c.outbuf != NULL) {
        LOGW("Sending at bus before session complete drops previous out-buffer");
        free(ddata->lxi_state.i2c.outbuf);
    }

    ddata->lxi_state.i2c.outbuf = malloc(1);
    ddata->lxi_state.i2c.outbuf[0] = data;

    ddata->lxi_state.i2c.msg[OUT_MSG].flags = 0;
    ddata->lxi_state.i2c.msg[OUT_MSG].len = 1;
    ddata->lxi_state.i2c.msg[OUT_MSG].buf = ddata->lxi_state.i2c.outbuf;

    ddata->lxi_state.i2c.func_0 = TO_LXI_STATE(sendByte);
    /* We can't determine if address has been ACKed or not here so we'll fake
       it. Check is done on stop (i.e. sequence execution) instead. */
    return 1;
}

/* Send chunk */
void lxii2c_sendData(struct ddata *ddata, const uint8_t *data, int sz)
{
    if (ddata->lxi_state.i2c.func_0 == TO_LXI_STATE(sendByte)) {
        ASSURE(ddata->lxi_state.i2c.outbuf);
        ddata->lxi_state.i2c.msg[OUT_MSG].addr =
            RW_ADDR(ddata->lxi_state.i2c.outbuf[0]);
        free(ddata->lxi_state.i2c.outbuf);
        ddata->lxi_state.i2c.outbuf = NULL;
    }

    if (ddata->lxi_state.i2c.outbuf != NULL) {
        LOGW("Sending at bus before session complete drops previous out-buffer");
        free(ddata->lxi_state.i2c.outbuf);
    }

    ddata->lxi_state.i2c.outbuf = malloc(sz);
    memcpy(ddata->lxi_state.i2c.outbuf, data, sz);

    ddata->lxi_state.i2c.msg[OUT_MSG].flags = 0;
    ddata->lxi_state.i2c.msg[OUT_MSG].len = sz;
    ddata->lxi_state.i2c.msg[OUT_MSG].buf = ddata->lxi_state.i2c.outbuf;

    ddata->lxi_state.i2c.func_0 = TO_LXI_STATE(sendData);
}

/* Receive one byte */
void lxii2c_receiveByte(struct ddata *ddata, uint8_t *data)
{
    lxii2c_receiveData(ddata, data, 1);

    ddata->lxi_state.i2c.func_0 = TO_LXI_STATE(receiveByte);
}

/* Receive chunk */
void lxii2c_receiveData(struct ddata *ddata, uint8_t *data, int sz)
{
    /* Note: This risky as it depends/assumes back-end never OOB:s */
    ddata->lxi_state.i2c.inbuf = data;
    ASSURE(ddata->lxi_state.i2c.inbuf); /* Note: Non malloc pointers might slip
                                           thru. I.e. not fool-proof. */

    /* Sanity-check API sequence order */
    ASSURE(ddata->lxi_state.i2c.func_0 == TO_LXI_STATE(sendByte));

    /* All should be OK to proceed. */
    ddata->lxi_state.i2c.msg[IN_MSG].flags = I2C_M_RD;
    ddata->lxi_state.i2c.msg[IN_MSG].len = sz;
    ddata->lxi_state.i2c.msg[IN_MSG].buf = ddata->lxi_state.i2c.inbuf;

    ddata->lxi_state.i2c.func_0 = TO_LXI_STATE(receiveData);
}

uint16_t lxii2c_getStatus(struct ddata *ddata, uint16_t flags)
{
    return flags;
}

/* Init of ddata->linux (states) */
int lxii2c_configure(struct ddata *ddata)
{
    ddata->lxi_state.i2c.inbuf = NULL;
    ddata->lxi_state.i2c.outbuf = NULL;

    /* Set to invalid addresses */
    ddata->lxi_state.i2c.msg[IN_MSG].addr = 0xFF;
    ddata->lxi_state.i2c.msg[OUT_MSG].addr = 0xFF;

    /* Nothing to send/receive */
    ddata->lxi_state.i2c.msg[IN_MSG].len = 0;
    ddata->lxi_state.i2c.msg[OUT_MSG].len = 0;
    ddata->lxi_state.i2c.msg[IN_MSG].buf = NULL;
    ddata->lxi_state.i2c.msg[OUT_MSG].buf = NULL;

    /* Indicate no pending session */
    ddata->lxi_state.i2c.func_0 = TO_LXI_STATE(state_free);

    return 0;
}

/* Create a new adapter/driver-data object for external manipulation without
 * interfering with current one. If arg "adapter" is not NULL it will be a
 * copy of current, else it's will be pre-set with build-system defaults. */
struct ddata *lxii2c_newddata(struct adapter *adapter)
{
    struct ddata *ddata = NULL;

    ASSERT(ddata = malloc(sizeof(struct ddata)));

    if (adapter != NULL)
        return (struct ddata *)memcpy(ddata, adapter->driver.any->ddata,
                                      sizeof(struct ddata));

    memcpy(&(ddata->config.i2c), &lxi_dflt_config_I2C,
           sizeof(struct config_I2C));

    return ddata;
}

/***************************************************************************
 * Configuration  interface
 ***************************************************************************/
config_etype_t lxii2c_set_speed(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_set_power_on(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_set_pullups(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_set_aux_on(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_set_cs_active(int setval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_get_speed(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_get_power_on(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_get_pullups(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_get_aux_on(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}

config_etype_t lxii2c_get_cs_active(int *retval, struct ddata * dd)
{
    return E_UNKNOWN;
}
