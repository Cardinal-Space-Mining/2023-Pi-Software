#include "jassert.hpp"

#include <string>

namespace
{
    inline std::string generate_message(const char *assertion, const char *file, unsigned int line, const char *function)
    {
        // Fmt: main.cpp:10: main: Assertion `x >= 0.0' failed.

        std::string s = "In File: ";
        s = s + file + "; Line: " + std::to_string(line) + "; Function: " + function + "; Assertion '" + assertion + "' failed.";
        return s;
    }
}

AssertError::AssertError(const char *assertion, const char *file, unsigned int line, const char *function) : std::runtime_error(generate_message(assertion, file, line, function))
{
}

char const * AssertError::what(){
    return std::runtime_error::what();
}