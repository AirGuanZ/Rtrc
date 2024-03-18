#pragma once

#include <span>

#include <Rtrc/Core/Container/Span.h>

RTRC_BEGIN

// Search for the first occurrence of a non-identifier character (or the beginning of a string) followed by
// the keyword in the source, starting from the beginPos.
// Returns the position of the first character of the found keyword, or string::npos if the keyword is not found.
// Comments and string literals are not present in the source.
size_t FindKeyword(std::string_view source, std::string_view keyword, size_t beginPos);

size_t FindFirstKeyword(
    std::string_view       source,
    Span<std::string_view> keywords,
    size_t                 beginPos,
    std::string_view      &outKeyword);

// Given the source string and a starting index 'begin', finds the matching '}' for the '{' at the given position.
// Returns the position of the matching '}' character, or string::npos if no matching '}' is found.
size_t FindMatchedRightBracket(std::string_view source, size_t begin);

// Replaces comments in the source string with spaces, while preserving the character positions.
void RemoveComments(std::string &source);

// Replaces comments and string literals in the source string with spaces, while preserving the character positions.
void RemoveCommentsAndStrings(std::string &source);

// Replaces string literals in the source string with spaces, while preserving the character positions.
void RemoveStrings(std::string &source);

RTRC_END
