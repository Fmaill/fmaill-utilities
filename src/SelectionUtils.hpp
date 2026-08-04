#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>

namespace fmaill::selection {
inline std::string trim(std::string value) {
    auto isNotSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), isNotSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
    return value;
}

inline std::optional<int> parseInteger(std::string const& value) {
    try {
        std::size_t consumed = 0;
        int result = std::stoi(value, &consumed);
        if (consumed != value.size()) {
            return std::nullopt;
        }
        return result;
    }
    catch (...) {
        return std::nullopt;
    }
}

inline std::set<int> parseIDs(std::string input, int maxID) {
    std::replace(input.begin(), input.end(), ';', ',');

    std::set<int> ids;
    std::size_t start = 0;

    while (start <= input.size()) {
        auto end = input.find(',', start);
        auto token = trim(input.substr(
            start,
            end == std::string::npos ? std::string::npos : end - start
        ));

        if (!token.empty()) {
            auto dash = token.find('-');
            if (dash == std::string::npos) {
                auto parsed = parseInteger(token);
                if (parsed && *parsed >= 1 && *parsed <= maxID) {
                    ids.insert(*parsed);
                }
            }
            else {
                auto first = parseInteger(trim(token.substr(0, dash)));
                auto last = parseInteger(trim(token.substr(dash + 1)));
                if (first && last) {
                    int rangeStart = std::max(std::min(*first, *last), 1);
                    int rangeEnd = std::min(std::max(*first, *last), maxID);
                    for (int id = rangeStart; id <= rangeEnd; ++id) {
                        ids.insert(id);
                    }
                }
            }
        }

        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    return ids;
}

inline std::string serializeIDs(std::set<int> const& ids) {
    std::ostringstream out;
    bool first = true;
    for (int id : ids) {
        if (!first) {
            out << ',';
        }
        first = false;
        out << id;
    }
    return out.str();
}

inline int pageCount(int itemCount, int pageSize) {
    if (itemCount <= 0 || pageSize <= 0) {
        return 1;
    }
    return (itemCount + pageSize - 1) / pageSize;
}

inline std::pair<int, int> pageRange(int page, int itemCount, int pageSize) {
    int pages = pageCount(itemCount, pageSize);
    int safePage = std::clamp(page, 0, pages - 1);
    int first = safePage * pageSize + 1;
    int last = std::min(first + pageSize - 1, itemCount);
    if (itemCount <= 0) {
        return { 0, 0 };
    }
    return { first, last };
}
} // namespace fmaill::selection
