#include <expected>
#include <ranges>
#include <format>
#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <string>
#include <string_view>

#include "std_scan_p1729r3.hpp"

struct mapper {

    void operator()(std::monostate k) const { return; }

    template <typename Ty>
    void operator()(Ty* p) {
        if (!p) {
            std::cout << "Ignored element!" << std::endl;
        }
        std::cout << typeid(Ty).name() << std::endl;
        if constexpr (std::is_same_v<Ty, int>) {
            *p = 10;
        }
    }
};

int main(int argc, char* argv[]) {
    using namespace std::string_literals;
    namespace scn = std::p1729r3;

    int p = 0, q = 0; scn::scan_ignored<int> m{};
    auto k = scn::make_scan_args<std::string>(p, q, m);

    auto t = k.get(0);
    t.visit(mapper{});

    std::cout << p;
}