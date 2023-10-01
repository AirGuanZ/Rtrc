#pragma once

#include <span>

#include <ShaderCompiler/Common.h>

RTRC_SHADER_COMPILER_BEGIN

// Search first '^ or non-identifier char + keyword' in source, starting from beginPos.
// Returns string::npos when not found.
// Comments & string literials are not considered.
size_t FindKeyword(std::string_view source, std::string_view keyword, size_t beginPos);

size_t FindFirstKeyword(
    const std::string     &source,
    Span<std::string_view> keywords,
    size_t                 beginPos,
    std::string_view      &outKeyword);

// Replace comments with spaces without changing character positions
void RemoveComments(std::string &source);

// Replace comments & string contents with spaces without changing character positions
void RemoveCommentsAndStrings(std::string &source);

// Replace string contents with spaces without changing character positions
void RemoveStrings(std::string &source);

RTRC_SHADER_COMPILER_END
