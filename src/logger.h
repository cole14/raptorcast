#ifndef __LOGGER_H
#define __LOGGER_H

#include <stdio.h>

class logger {
    public:
        logger(int level, FILE *fp);
        logger(int level, const char *f_name);
        ~logger();
        void log(int level, const char *str, ...) __attribute__((format(printf, 3, 4)));
        int get_level();
        void set_level(int new_level);
    private:
        int level;
        FILE *fp;
};

extern logger glob_log;

#endif /* __LOGGER_H */
