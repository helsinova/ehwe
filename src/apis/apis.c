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
#include <string.h>
#include <stdlib.h>
#include <assure.h>
#include <config.h>
#include "apis.h"
#include "stm32.h"
#include "ehwe.h"
#include "ehwe_i2c_device.h"
#include "adapters.h"

int apis_init()
{
    static int is_init = 0;

    if (is_init) {
        LOGW("No need to run %s twice, CTOR _init has run it?\n", __func__);
        return 0;
    }
    is_init = 1;

    return 0;
}

int apis_init_api(const struct adapter *adapter)
{
    int rc = 0;

#ifdef ENABLE_API_HIGH_LVL
    ASSURE_E((rc =
              ehwe_init_api(adapter)) == 0,
             goto apis_init_api_err);
    ASSURE_E((rc =
              i2c_device_init_api(adapter)) == 0,
             goto apis_init_api_err);
#endif

#ifdef ENABLE_API_STM32
    ASSURE_E((rc =
              stm32_init_api(adapter)) == 0,
             goto apis_init_api_err);
#endif

    return 0;

apis_init_api_err:
    LOGE("Failed initialize api\n");
    return rc;
}

/***************************************************************************
 *   INIT/FINI CTOR/DTOR mechanism                                         *
 ***************************************************************************/
#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

void __init __apis_init(void)
{
    int rc;
#ifdef ENABLE_INITFINI_SHOWEXEC
    fprintf(stderr, ">>> Running module _init in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism ====\n");
#endif
    if ((rc = apis_init())) {
        fprintf(stderr, "Fatal error: apis_init() failed\n");
        exit(rc);
    }
}

void __fini __apis_fini(void)
{
#ifdef ENABLE_INITFINI_SHOWEXEC
    fprintf(stderr, ">>> Running module _fini in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism\n");
#endif
}
