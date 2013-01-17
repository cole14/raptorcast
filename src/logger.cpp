#include <error.h>
#include <errno.h>
#include <stdarg.h>
#include "logger.h"

//Global logger instance
logger glob_log = logger(1, stdout);

/* Constructor from open file descriptor */
logger::logger(int l, FILE *f)
:level(l)
,fp(f)
{ }

/* Constructor from filename */
logger::logger(int l, const char *f_name)
:level(l)
,fp(NULL){
    if(NULL == (fp = fopen(f_name, "a+")))
        error(-1, errno, "Unable to open file '%s'", f_name);
}

/* Destructor */
logger::~logger(){
    if (fp != stdin && fp != stdout && fp != stderr)
        if (0 != fclose(fp))
            error(-1, errno, "Unable to close logger!");
}

/* Basic log method */
void logger::log(int l, const char *str, ...){
    va_list list;
    va_start(list, str);

    if(l > level)
        return;

    vfprintf(fp, str, list);
    fflush(fp);

    va_end(list);
}

/* Query the current logging level */
int logger::get_level(){
    return level;
}

/* Change the logging level */
void logger::set_level(int new_level){
    level = new_level;
}

/* Log method to dump a byte buffer to the log */
void logger::dump_buf(int l, void *b, size_t len){
    unsigned char *buf = (unsigned char *)b;
    for(int i = 0; i < (int)len; i++){
        log(l, "%02X ", buf[i]);
        if((i+1) % 16 == 0)
            log(l, "\n");
    }
    log(l, "\n");
}

