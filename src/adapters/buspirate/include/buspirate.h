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
#ifndef buspirate_h
#define buspirate_h
/***************************************************************************
 * Public interface
 ***************************************************************************/

struct buspirate {
    clkownr_t clckownr;
    char *name;                 /* Local hosts adapter name/path */
};

/* Valid regex-i role patterns for buspirate */
#define BP_ROLES "SPI|I2C"

/* Valid regex-i clock-owner patterns for buspirate */
#define BP_CLKOWNER "MASTER|SLAVE"

/* Forward declaration of 'struct adapter' required to avoid mutual header
 * inclusion */
struct adapter;

int buspirate_init();
int buspirate_parse(const char *adapterstr, struct adapter *adapter);
int buspirate_init_adapter(struct adapter *adapter);
int buspirate_deinit_adapter(struct adapter *adapter);

#endif                          //buspirate_h
