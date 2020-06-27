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
#include <liblog/assure.h>
#include <liblog/log.h>
#include "adapters.h"
#include <string.h>
#include <stdlib.h>
#include "adapters_config.h"

#ifdef ADAPTER_PARAPORT
#include <paraport.h>
#endif

#ifdef ADAPTER_BUSPIRATE
#include <buspirate.h>
#endif

#ifdef ADAPTER_LXI
#include <lxi.h>
#endif

static regex_t preg;            /* Compiled regular expression for generic
                                   part of adapter-string parsing */

#define REGEX_PATT \
	"^(" ROLES \
	"):(" INDEX \
	"):(" ADAPTERS \
	"):(" ANYTHING \
")"

#define REGEX_NSUB (4+1)        /* Number of parse-groups (see REGEXP_PATT) */

int adapters_init()
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
    return 0;
}

/*
 * Takes adapterstr and makes initial parsing to detect which driver it should
 * send each description to for further refinement. Result is a struct
 * adapter,
 *
 * */
int adapters_parse(const char *adapterstr, struct adapter *adapter)
{
    int rc = 0, i;
    char err_str[REXP_ESTRSZ];
    regmatch_t mtch_idxs[REGEX_NSUB];
    char *adapterstr_cpy = strdup(adapterstr);
    char *role_str;
    char *index_str;
    char *adapter_str;

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
        ASSURE_E(mtch_idxs[i].rm_so != -1, goto adapters_parse_err);
        adapterstr_cpy[mtch_idxs[i].rm_eo] = 0;
    }

    role_str = &adapterstr_cpy[mtch_idxs[1].rm_so];
    index_str = &adapterstr_cpy[mtch_idxs[2].rm_so];
    adapter_str = &adapterstr_cpy[mtch_idxs[3].rm_so];

    LOGD("  First level adapter-string parsing:\n");
    LOGD("    role=%s\n", role_str);
    LOGD("    index=%s\n", index_str);
    LOGD("    adapter=%s\n", adapter_str);

#ifdef ADAPTER_PARAPORT
    if (strcasecmp(adapter_str, "pp") == 0)
        ASSURE_E((rc =
                  paraport_parse(adapterstr, adapter)) == 0, goto adapters_parse_err);
#endif
#ifdef ADAPTER_BUSPIRATE
    if (strcasecmp(adapter_str, "bp") == 0)
        ASSURE_E((rc =
                  buspirate_parse(adapterstr, adapter)) == 0,
                 goto adapters_parse_err);
#endif
#ifdef ADAPTER_LXI
    if (strcasecmp(adapter_str, "lxi") == 0)
        ASSURE_E((rc =
                  lxi_parse(adapterstr, adapter)) == 0,
                 goto adapters_parse_err);
#endif

    free(adapterstr_cpy);
    return rc;
adapters_parse_err:
    free(adapterstr_cpy);
    return -1;
}

/*
 * For each adapter-driver:
 * - Compiles regexp for further matching
 * - Initialize DD-global variables */
int adapters_init_adapter(struct adapter *adapter)
{
    int rc = 0;

    ASSURE(adapter);
    LOGD("{%d,%d,%d}\n", adapter->devid, adapter->role, adapter->index);
    switch (adapter->devid) {
#ifdef ADAPTER_PARAPORT
        case PARAPORT:
            rc = paraport_init_adapter(adapter);
            break;
#endif
#ifdef ADAPTER_BUSPIRATE
        case BUSPIRATE:
            rc = buspirate_init_adapter(adapter);
            break;
#endif
#ifdef ADAPTER_LXI
        case LXI:
            rc = lxi_init_adapter(adapter);
            break;
#endif
        default:
            LOGE("Unsupported adapter [%d] in [%s]\n", adapter->devid, __func__);
    }

    return rc;
}

int adapters_deinit_adapter(struct adapter *adapter)
{
    int rc = 0;

    ASSERT(adapter);
    ASSERT(adapter->driver.any);
    LOGD("{%d,%d,%d}\n", adapter->devid, adapter->role, adapter->index);
    switch (adapter->devid) {
#ifdef ADAPTER_PARAPORT
        case PARAPORT:
            rc = paraport_deinit_adapter(adapter);
            break;
#endif
#ifdef ADAPTER_BUSPIRATE
        case BUSPIRATE:
            rc = buspirate_deinit_adapter(adapter);
            break;
#endif
#ifdef ADAPTER_LXI
        case LXI:
            rc = lxi_deinit_adapter(adapter);
            break;
#endif
        default:
            LOGE("Unsupported adapter [%d] in [%s]\n", adapter->devid, __func__);
    }
    return rc;
}

/***************************************************************************
 * INIT/FINI mechanism
 ***************************************************************************/
#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

void __init __adapters_init(void)
{
    int rc;
#ifdef ENABLE_INITFINI_SHOWEXEC
    fprintf(stderr, ">>> Running module _init in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism ====\n");
#endif
    if ((rc = adapters_init())) {
        fprintf(stderr, "Fatal error: adapters_init() failed\n");
        exit(rc);
    }
}

void __fini __adapters_fini(void)
{
#ifdef ENABLE_INITFINI_SHOWEXEC
    fprintf(stderr, ">>> Running module _fini in [" __FILE__ "]\n"
            ">>> using CTORS/DTORS mechanism\n");
#endif
}
