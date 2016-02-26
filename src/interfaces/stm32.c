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
#include "interfaces.h"
#include "stm32.h"
#include "devices.h"
#include <string.h>
#include <stdlib.h>
#include <assure.h>

int stm32_init()
{
    static int is_init = 0;

    if (is_init) {
        LOGW("No need to run %s twice, CTOR _init has run it?\n", __func__);
        return 0;
    }
    is_init = 1;

    return 0;
}

int stm32_init_interface(const struct device *device)
{
    LOGW("Unfinished function [%s] (TBD) for device ID [%d]\n", __func__,device->devid);
    return 0;
}

/***************************************************************************
 *   INIT/FINI CTOR/DTOR mechanism                                         *
 ***************************************************************************/
#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

void __init __stm32_init(void)
{
    int rc;
#ifdef INITFINI_SHOW
    fprintf(stderr, ">>> Running module _init in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism ====\n");
#endif
    if ((rc = stm32_init())) {
        fprintf(stderr, "Fatal error: stm32_init() failed\n");
        exit(rc);
    }
}

void __fini __stm32_fini(void)
{
#ifdef INITFINI_SHOW
    fprintf(stderr, ">>> Running module _fini in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism\n");
#endif
}
