#pragma once
#include <stdexcept>

#ifdef _MSC_VER
#if _MSC_VER && !__INTEL_COMPILER
#define FN_NAME __FUNCSIG__
#endif

#elif __GNUC__
#define FN_NAME __PRETTY_FUNCTION__
#elif
#error "Compiler Not Supported"
#endif
class AssertError : public std::runtime_error
{
public:
	AssertError(const char *assertion, const char *file, unsigned int line, const char *function);
	virtual char const *what();
	virtual ~AssertError() = default;
};

/*
One issue I am running into is that the failing assertions crash the program before the thread can log the error.
This will throw an exception that can be caught, then logged.
Then we can call exit(-1)
*/

#define jassert(item_to_eval)                                          \
	if ((item_to_eval))                                                \
	{                                                                  \
	}                                                                  \
	else                                                               \
	{                                                                  \
		throw AssertError(#item_to_eval, __FILE__, __LINE__, FN_NAME); \
	};
