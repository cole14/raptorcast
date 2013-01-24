#include "error_handling.h"


fatal_exception::fatal_exception(const char *str)
:msg(str)
{ }

fatal_exception::fatal_exception(const std::string &str)
:msg(str)
{ }

fatal_exception::~fatal_exception() throw() { }

const char *fatal_exception::what() const throw() {
    return msg.c_str();
}

