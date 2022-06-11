#pragma once

#include <vector>

#include <Rtrc/Utils/Variant.h>

RTRC_DSL_BEGIN

struct StringStatement;
struct Block;

using Statement = Variant<StringStatement, Block>;

struct StringStatement
{
    std::string str;
};

struct Block
{
    std::vector<RC<Statement>> statements;
};

RTRC_DSL_END
