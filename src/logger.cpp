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
,l_mut()
{ }

/* Constructor from filename */
logger::logger(int l, const char *f_name)
:level(l)
,fp(NULL)
,l_mut()
{
    if(NULL == (fp = fopen(f_name, "a+")))
        error(-1, errno, "Unable to open file '%s'", f_name);
}

/* 
 * Construct a new logger from an existing logger.
 *  The current implementation simply copies over the
 *  other logger's level and fp, but creates a new
 *  mutex. This is because the mutex is a class type
 *  rather than reference or pointer, and std::mutex
 *  does not allow copy-construction.
 *  If we want to support the more logical behavior
 *  of sharing a mutex for a log fp, then we'll need
 *  to place the mutex on the heap and do some sort
 *  of reference counting with smart_ptrs.
 */
logger::logger(const logger& l)
:level(l.level)
,fp(l.fp)
,l_mut()
{ }

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

    //Make the log operation atomic
    l_mut.lock();

    vfprintf(fp, str, list);
    fflush(fp);

    va_end(list);

    l_mut.unlock();
}

/* Query the current logging level */
int logger::get_level(){
    return level;
}

/* Change the logging level */
void logger::set_level(int new_level){
    //Even though the assignment operation should be atomic,
    //  it's better to be safe than sorry.
    l_mut.lock();

    level = new_level;

    l_mut.unlock();
}

/* Log method to dump a byte buffer to the log */
void logger::dump_buf(int l, void *b, size_t len){
    if(l > level)
        return;

    // WARNING!
    //  Don't use logger::log() in this method!
    //  We want the entire dump_buf method to be atomic,
    //  so calling a lower-level log method will cause
    //  a deadlock because that method will also try to
    //  be atomic.
    l_mut.lock();

    //Log the buf
    unsigned char *buf = (unsigned char *)b;
    for(int i = 0; i < (int)len; i++){
        fprintf(fp, "%02X ", buf[i]);
        if((i+1) % 16 == 0)
            fprintf(fp, "\n");
    }
    fprintf(fp, "\n");

    //Release the lock
    l_mut.unlock();
}

void logger::pretty_print_id_list(int l, std::vector< unsigned > &list){
    if(l > level)
        return;

    // WARNING!
    //  Don't use logger::log() in this method!
    //  We want the entire dump_buf method to be atomic,
    //  so calling a lower-level log method will cause
    //  a deadlock because that method will also try to
    //  be atomic.
    l_mut.lock();

    //Walk along the vector and print series of consecutive
    //integers as a range "n-m".  e.g. "1, 3, 5-9, 12"
    if(list.size() > 0){
        unsigned last = 0;
        bool in_ell = false;
        std::vector< unsigned >::iterator it = list.begin();
        fprintf(fp, "%u", *it);

        for(it++; it != list.end(); it++){
            if(*it != last + 1){
                if(in_ell){
                    in_ell = false;
                    fprintf(fp, "%u, %u", last, *it);
                }else{
                    fprintf(fp, ", %u", *it);
                }
            }else if(!in_ell){
                in_ell = true;
                fprintf(fp, "-");
            }
            last = *it;
        }

        if(in_ell)
            fprintf(fp, "%u\n", last);
        else
            fprintf(fp, "\n");
    }else{
        fprintf(fp, "Empty.\n");
    }

    l_mut.unlock();
}

