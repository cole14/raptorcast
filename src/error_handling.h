#ifndef __ERROR_HANDLING_H
#define __ERROR_HANDLING_H

#include <string.h>

#include <string>
#include <exception>

#include <sstream>

class fatal_exception : public std::exception {
    public:
        fatal_exception(const char *str);
        fatal_exception(const std::string &str);
        virtual const char *what() const throw();
        virtual ~fatal_exception() throw();

    private:
        std::string msg;
};

#define throw_except(type, args) \
    do { \
        std::stringstream ss; \
        ss << __FILE__ << ":" << __LINE__ << " " << args; \
        throw type(ss.str()); \
    } while(true)

#define throw_fatal(args) throw_except(fatal_exception, args)

#define throw_errno(type, errno, args) throw_except(type, strerror(errno) << " : " << args)

#endif /* __ERROR_HANDLING_H */
