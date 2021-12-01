#pragma once
#include <vector>
#include <string>
#include <set>


#define SPECIAL_SYMBOLS "special symbols"s
#define NEGATIVE_ID "negative id"s
#define ID_EXISTS "document with the same id already exists"s
#define WORD_AFTER_MINUS "missing word after '-'"s
#define MANY_MINUSES "too many '-'s"s
#define LARGE_INDEX "index is more than a documents count"s

std::vector<std::string> SplitIntoWords(const std::string_view text);

template<typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(StringContainer strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
bool HasSpecialSymbols(const std::string& text);