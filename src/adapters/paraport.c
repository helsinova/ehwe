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
#include "adapters.h"
#include "paraport.h"

/*
 * Refined parsing of adapterstring to complete paraport adapter_struct
 *
 * */
int paraport_parse(const char *adapterstr, struct adapter *adapter)
{
    /* Parse the first part to be able to get the vitals out */

    LOGE("Unfinished function [%s] (TBD) for adapter ID [%d]\n", __func__,
         adapter->devid);
    return -1;
}

int paraport_init_adapter(struct adapter *adapter)
{
    LOGE("Unfinished function [%s] (TBD) for adapter ID [%d]\n", __func__,
         adapter->devid);
    return -1;
}

int paraport_deinit_adapter(struct adapter *adapter)
{
    LOGE("Unfinished function [%s] (TBD) for adapter ID [%d]\n", __func__,
         adapter->devid);
    return -1;
}

/***************************************************************************
 * INIT/FINI mechanism
 ***************************************************************************/
#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

void __init __paraport_init(void)
{
#ifdef ENABLE_INITFINI_SHOWEXEC
    fprintf(stderr, ">>> Running module _init in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism ====\n");
#endif
}

void __fini __paraport_fini(void)
{
#ifdef ENABLE_INITFINI_SHOWEXEC
    fprintf(stderr, ">>> Running module _fini in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism\n");
#endif
}
