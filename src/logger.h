#ifndef __LOGGER_H
#define __LOGGER_H

#include <stdio.h>
#include <stddef.h>

#include <mutex>

class logger {
    public:
        // Constructors/Destructors
        logger(int level, FILE *fp);
        logger(int level, const char *f_name);
        //Read my comment in logger.cpp for usage info!
        logger(const logger& l);
        ~logger();

        // Logging methods
        void log(int level, const char *str, ...) __attribute__((format(printf, 3, 4)));
        void dump_buf(int level, void *buf, size_t len);

        // Log level getter/setters
        int get_level();
        void set_level(int new_level);

    private:
        // The current logging level for this logger
        //   Logging methods are NOPS for calls with level > the current logger's level
        int level;
        // File descriptor of where this logger logs to.
        FILE *fp;
        // Mutex for thread safe logging
        std::mutex l_mut;
};

//The global logger for this project
extern logger glob_log;

#endif /* __LOGGER_H */
