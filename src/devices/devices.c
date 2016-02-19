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
#include <sys/types.h>
#include <regex.h>
#include <log.h>
#include "devices.h"
#include <stdio.h>

static regex_t preg;            /* Compiled regular expression for generic
                                   part of device-string parsing */

#define DEVSTR_PATT \
	"(" ROLES "):(" "[0-9]" "):(" DEVICES "):(" DIRECTIONS "):(.*)"

int devices_init()
{
    int rc;
    char err_str[80];
    static int is_init = 0;

    if (is_init) {
        LOGW("No need to run %s twice, CTOR _init has run it?", __func__);
        return 0;
    }
    is_init = 1;

    rc = regcomp(&preg, DEVSTR_PATT, REG_EXTENDED);
    if (rc) {
        regerror(rc, &preg, err_str, 80);
        LOGE("Regexec compilation error: %s", err_str);
        return rc;
    }
    return 0;
}

/*
 * Takes devstr and makes initial parsing to detect which driver it should
 * send each description to for further refinement. Result is a struct
 * device,
 *
 * */
int devices_parse(const char *devstr, struct device *device)
{
    return 0;
}
