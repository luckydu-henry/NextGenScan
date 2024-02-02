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


enum class _Scn_align : uint8_t { _None, _Left, _Right, _Center };
enum class _Scn_sign  : uint8_t { _None, _Plus, _Minus, _Space};

template <class CharT>
struct _Basic_scn_specs {
    int        width         = 0;
    int        precision     = -1;
    char       type          = '\0';
    _Scn_align alignment     = _Scn_align::_None;
    _Scn_sign  sgn           = _Scn_sign::_None;
    bool       alt           = false;
    bool       localized     = false;
    bool       leading_zero  = false;
    uint8_t    fill_length   = 1;
    // At most one codepoint (so one char32_t or four utf-8 char8_t).
    CharT      fill[4 / sizeof(CharT)] = { CharT{' '} };
};


template <class Rng>
struct _Default_arg_scanner {
    Rng rng;
    using iterator = std::ranges::iterator_t<Rng>;

    iterator operator()(std::monostate k) const { return rng.end(); }
    template <typename Ty>
    iterator operator()(Ty* p) {
        char buf[1200] = {};
        if constexpr (std::is_same_v<Ty, int>) {
            int v = 0;
            char* q = buf;
            // Can't be replaced by copy_if
            for (auto i = rng.begin(); i != rng.end() && *i >= '0' && *i <= '9'; ++i) { *q++ = *i; }
            auto res = std::from_chars(buf, q, v);
            if (p) { *p = v; }
            return std::next(rng.begin(), res.ptr - buf);
        }
        return rng.begin();
    }
    iterator operator()(typename std::p1729r3::basic_scan_arg<std::p1729r3::basic_scan_context<Rng, char>>::handle hd) {
        return rng.begin();
    }
};

template <std::p1729r3::scannable_range<char> Rng>
std::p1729r3::vscan_result_type<Rng> format_from(Rng rg, std::string_view fmt, std::p1729r3::scan_args<Rng> args) {
    std::p1729r3::scan_context<Rng> ctx{ rg, args };
    std::size_t id = 0;

    for (auto i = fmt.begin();i != fmt.end(); ++i) {
        // One of format or context range has empty characters.
        if (*ctx.current() == ' ') { ctx.advance_to(std::next(ctx.current())); }

        if (*i != ' ' && *ctx.current() != ' ') {
            if (*i == '{' && *(i + 1) == '}') {
                ++i;
                auto j = ctx.arg(id).visit(_Default_arg_scanner<Rng>{ctx.range()});
                ctx.advance_to(j);
                ++id;
            }
            else {
                if (*i == *ctx.current()) { ctx.advance_to(std::next(ctx.current())); }
                else {
                    return std::unexpected(std::p1729r3::scan_error{
                        std::p1729r3::scan_error::pattern_unmatched,
                        "Fmt string's character is different from context's."
                    });
                }
            }
        }
    }
    return rg;
}


int main(int argc, char* argv[]) {
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    namespace scn = std::p1729r3;

    int p = 0, q = 0, m = 0;
    
    std::string_view buf = "Abc 10923 4567 89101";
    std::ranges::subrange rng{ buf };

    auto k0 = scn::make_scan_arg_store<decltype(rng)>(p, q, m);
    auto k = scn::make_scan_args(k0);
    auto t = format_from(rng, "Abc {}{}{}", k);

    if (t.has_value()) {
        std::cout << m << std::endl;
    }


}