#include "../src/SelectionUtils.hpp"

#include <cassert>
#include <set>
#include <string>

using namespace fmaill::selection;

int main() {
    assert((parseIDs("1, 5, 20-23", 30) == std::set<int>{1, 5, 20, 21, 22, 23}));
    assert((parseIDs("5-3; 3; 99; -1; abc", 10) == std::set<int>{3, 4, 5}));
    assert((parseIDs("1-999", 4) == std::set<int>{1, 2, 3, 4}));
    assert(parseIDs("", 10).empty());
    assert(serializeIDs(std::set<int>{1, 4, 8}) == "1,4,8");
    assert(pageCount(485, 18) == 27);
    assert(pageCount(0, 18) == 1);
    assert((pageRange(0, 43, 18) == std::pair<int, int>(1, 18)));
    assert((pageRange(2, 43, 18) == std::pair<int, int>(37, 43)));
    assert((pageRange(99, 43, 18) == std::pair<int, int>(37, 43)));
    return 0;
}
