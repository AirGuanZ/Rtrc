#include <stack>

#include "eVariable.h"

RTRC_EDSL_BEGIN

namespace Detail
{

    thread_local std::stack<std::string> gMemberVariableNameStack;
    thread_local std::stack<eVariableCommonBase *> gVariableInitializationStack;
    thread_local std::stack<eVariableCommonBase *> gVariableCopyStack;
    thread_local bool gEnableStackVariableAllocation = true;

} // namespace Detail

RTRC_EDSL_END
