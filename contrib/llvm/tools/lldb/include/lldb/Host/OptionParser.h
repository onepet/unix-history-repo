//===-- OptionParser.h ------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_OptionParser_h_
#define liblldb_OptionParser_h_

namespace lldb_private {

typedef struct Option
{
    // name of long option
    const char *name;
    // one of no_argument, required_argument, and optional_argument:
    // whether option takes an argument
    int has_arg;
    // if not NULL, set *flag to val when option found
    int *flag;
    // if flag not NULL, value to set *flag to; else return value
    int val;
} Option;

class OptionParser
{
public:
    enum OptionArgument
    {
        eNoArgument = 0,
        eRequiredArgument,
        eOptionalArgument
    };

    static void Prepare();

    static void EnableError(bool error);

    static int Parse(int argc, char * const argv [],
        const char *optstring,
        const Option *longopts, int *longindex);

    static char* GetOptionArgument();
    static int GetOptionIndex();
    static int GetOptionErrorCause();
};

}

#endif  // liblldb_OptionParser_h_
