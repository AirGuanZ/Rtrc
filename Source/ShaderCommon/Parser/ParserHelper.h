#pragma once

#include <span>

#include <Core/Container/Span.h>

RTRC_BEGIN

// Search first '^ or non-identifier char + keyword' in source, starting from beginPos.
// Returns string::npos when not found.
// Comments & string literials are not considered.
size_t FindKeyword(std::string_view source, std::string_view keyword, size_t beginPos);

size_t FindFirstKeyword(
    std::string_view       source,
    Span<std::string_view> keywords,
    size_t                 beginPos,
    std::string_view      &outKeyword);

// Assume source[begin] is '{', find matched '}'
size_t FindMatchedRightBracket(std::string_view source, size_t begin);

// Replace comments with spaces without changing character positions
void RemoveComments(std::string &source);

// Replace comments & string contents with spaces without changing character positions
void RemoveCommentsAndStrings(std::string &source);

// Replace string contents with spaces without changing character positions
void RemoveStrings(std::string &source);

RTRC_END
