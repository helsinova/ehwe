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

#define LOGV_IOERROR( X ) log_ioerror( X , LOG_LEVEL_VERBOSE )
#define LOGD_IOERROR( X ) log_ioerror( X , LOG_LEVEL_DEBUG )
#define LOGI_IOERROR( X ) log_ioerror( X , LOG_LEVEL_INFO )
#define LOGW_IOERROR( X ) log_ioerror( X , LOG_LEVEL_WARNING )
#define LOGE_IOERROR( X ) log_ioerror( X , LOG_LEVEL_ERROR )

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
} bpcmd_t;

/* Driver companion - NOTE: unique for each driver. Must NOT be public */
struct ddata {
    int fd;
    bpcmd_t state;
};

struct device;

void log_ioerror(int ecode, log_level llevel);
void empty_inbuff(int fd);
int rawMode_enter(struct device *);
int rawMode_toMode(struct device *, bpcmd_t bpcmd);

/***************************************************************************
 * Driver interfaces
 ***************************************************************************/
void bpspi_sendData(const uint8_t *data, int sz);
void bpspi_receiveData(uint8_t *data, int sz);
uint16_t bpspi_getStatus(uint16_t flags);

void bpi2c_sendData(const uint8_t *data, int sz);
void bpi2c_receiveData(uint8_t *data, int sz);
uint16_t bpi2c_getStatus(uint16_t flags);

#endif                          //buspirate_local_h
