#include <stdio.h>
#include "doc.h"
#include "cmd_help.h"

static char ehwe_doc[] = CLI_CMD_HELPTEXT_ehwe;

void doc_print()
{
    printf("%s\n", ehwe_doc);
}
