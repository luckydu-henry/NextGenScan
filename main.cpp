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
            return;
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

    int p = 0;float q = 0; scn::scan_ignored<int> m{};

    auto k0 = scn::make_scan_arg_store<std::string>(p, q, q);
    auto t0 = scn::make_scan_arg_store<std::string>(q, p, p);
    auto k = scn::make_scan_args<std::string>(k0);
    auto t = scn::make_scan_args<std::string>(t0);

    scn::scan_context<std::string> ctx{ "Hello world!", k };

    ctx.arg(1).visit(mapper{});
}