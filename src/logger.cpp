#include <error.h>
#include <errno.h>
#include <stdarg.h>
#include "logger.h"

logger::logger(int l, FILE *f)
:level(l)
,fp(f)
{ }

logger::logger(int l, const char *f_name)
:level(l)
,fp(NULL){
    if(NULL == (fp = fopen(f_name, "a+")))
        error(-1, errno, "Unable to open file '%s'", f_name);
}

logger::~logger(){
    if (fp != stdin && fp != stdout && fp != stderr)
        if (0 != fclose(fp))
            error(-1, errno, "Unable to close logger!");
}

void logger::log(int l, const char *str, ...){
    va_list list;
    va_start(list, str);

    if(l > level)
        return;

    vfprintf(fp, str, list);
    fflush(fp);

    va_end(list);
}

int logger::get_level(){
    return level;
}

void logger::set_level(int new_level){
    level = new_level;
}

logger glob_log = logger(1, stdout);
