#include <stack>

#include "eVariable.h"

RTRC_EDSL_BEGIN

namespace Detail
{

    std::stack<std::string> gMemberVariableNameStack;
    std::stack<eVariableCommonBase *> gNestedVariableInitializationStack;
    std::stack<eVariableCommonBase *> gVariableCopyStack;
    bool gEnableStackVariableAllocation = true;

} // namespace Detail

RTRC_EDSL_END
