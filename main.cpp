#include <iostream>

#include "Delegate.h"

int add(int, int) {
    return 0;
}

class Test {
public:
    int Add(int a, int b) {
        std::cout << "Test::Add" << std::endl;
        return 9;
    }
};

int main() {
    auto plus = [](int a, int b) { return a + b; };

    Event::Delegate<int(int, int)> d = Event::MakeDelegate(add);
    Test test;
    Event::Delegate<int (Test::*)(int, int)> e = Event::MakeDelegate(&Test::Add);
    std::cout << e(test, 0, 0) << std::endl;
    return 0;
}
