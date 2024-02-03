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


#define _SCAN_UNEXPECT(error, str) std::unexpected(std::p1729r3::scan_error{std::p1729r3::scan_error::error, str })

template <typename CharT>
std::expected<std::tuple<typename std::p1729r3::basic_scan_parse_context<CharT>::iterator, std::size_t>,
    std::p1729r3::scan_error> _Get_scan_replacement(std::p1729r3::basic_scan_parse_context<CharT>& ptx) {

    auto i = std::next(ptx.begin());
    std::size_t j = -1;

    for (; *i != ':'; ++i) {
        if (*i >= '0' && *i <= '9') { j = (j == -1) ? (*i - '0') : (j * 10 + *i - '0'); }
        else if (*i == '}')         { goto ret_point; }
        else                        { return _SCAN_UNEXPECT(invalid_format_string, "Invalid character in replacment field"); }
    }
    if (*std::next(i) == '}') return _SCAN_UNEXPECT(invalid_format_string, "Scan description is empty!");
ret_point:
    return std::make_tuple(i, j != -1 ? j : ptx.next_arg_id());
}


enum class _Scn_align : uint8_t { _None, _Left, _Right, _Center };
enum class _Scn_sign : uint8_t { _None, _Plus, _Minus, _Space };

template <class CharT>
struct _Basic_scn_specs {
    int        width = 0;
    int        precision = -1;
    char       type = '\0';
    _Scn_align alignment = _Scn_align::_None;
    _Scn_sign  sgn = _Scn_sign::_None;
    bool       alt = false;
    bool       localized = false;
    bool       leading_zero = false;
    uint8_t    fill_length = 1;
    // At most one codepoint (so one char32_t or four utf-8 char8_t).
    CharT      fill[4 / sizeof(CharT)] = { CharT{' '} };
};



template <typename CharT>
std::expected<typename std::p1729r3::basic_scan_parse_context<CharT>::iterator,
                       std::p1729r3::scan_error> _Parse_scan_specs(const std::p1729r3::basic_scan_parse_context<CharT>& pctx, 
                                                                   _Basic_scn_specs<CharT>& specs) {
    auto i = pctx.begin();
    for (;*i != '}'; ++i) {
        // Do format specifies.
    }
    // Return the next element of }
    return std::next(i);
}


template <class Rng>
struct _Arg_visitor {

    std::p1729r3::scan_context<Rng>&     sctx;
    std::p1729r3::scan_parse_context&    pctx;

    using iterator    = typename std::p1729r3::scan_context<Rng>::iterator;
    using result_type = std::expected<iterator, std::p1729r3::scan_error>;

    result_type operator()(std::monostate k) const { return sctx.current(); }
    template <typename Ty>
    result_type operator()(Ty* p) {

        _Basic_scn_specs<char> specs;
        auto pres = _Parse_scan_specs(pctx, specs);

        if (pres.has_value()) { pctx.advance_to(pres.value()); }
        else                  { return std::unexpected(pres.error()); }

        auto rng = sctx.range();

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
    result_type operator()(typename std::p1729r3::basic_scan_arg<std::p1729r3::basic_scan_context<Rng, char>>::handle& hd) {
        hd.scan(pctx, sctx);
        return sctx.current();
    }
};

template <std::p1729r3::scannable_range<char> Rng>
std::p1729r3::vscan_result_type<Rng> format_from(Rng rg, std::string_view fmt, std::p1729r3::scan_args<Rng> args) {
    std::p1729r3::scan_context<Rng> ctx{ rg, args };
    std::p1729r3::scan_parse_context ptx{ fmt};

    auto pc = ptx.begin();
    auto sc = ctx.current();

    for (; pc != ptx.end(); pc = ptx.begin(), sc = ctx.current()) {
        // One of format or context range has empty characters.
        if (*pc == ' ') { ptx.advance_to(std::next(pc)); }
        if (*sc == ' ') { ctx.advance_to(std::next(sc)); }

        if (*pc != ' ' && *sc != ' ') {
            if (*pc == '{') {
                if (*std::next(pc) == '{') {
                    if (*sc == '{') {
                        ptx.advance_to(std::next(pc, 2));
                        ctx.advance_to(std::next(sc));
                    } else goto scan_end;
                }
                // Value should be scan in.
                else {
                    auto v = _Get_scan_replacement(ptx);
                    if (v.has_value()) {
                        ptx.advance_to(std::get<0>(v.value()));

                        auto k = ctx.arg(std::get<1>(v.value())).visit(_Arg_visitor<Rng>{ctx, ptx});
                        if (k.has_value()) { ctx.advance_to(k.value()); }
                        else               { return std::unexpected(v.error()); }
                    } else  return std::unexpected(v.error());
                }
            }
            else if (*pc == '}') {
                if (*std::next(pc) == '}') {
                    if (*sc == '}') {
                        ptx.advance_to(std::next(pc, 2));
                        ctx.advance_to(std::next(sc));
                    } else goto scan_end;
                }
                else return _SCAN_UNEXPECT(invalid_format_string, "Invalid escape code }");
            }
            else {
                if (*pc == *sc) {
                    ptx.advance_to(std::next(pc));
                    ctx.advance_to(std::next(sc));
                } else goto scan_end;
            }
        }
    }
    scan_end:
    return ctx.range();
}
#undef _SCAN_UNEXPECT


int main(int argc, char* argv[]) {
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    namespace scn = std::p1729r3;

    scn::scan_skip<int> p{}; int q = 0, m = 0;
    
    std::string_view buf = "Abc 10923 4567 89101";
    std::ranges::subrange rng{ buf };

    auto k0 = scn::make_scan_arg_store<decltype(rng)>(p, q, m);
    auto k = scn::make_scan_args(k0);
    auto t = format_from(rng, "Abc {0:d}{1}{2}", k);


    if (t.has_value()) {
        std::cout << q << std::endl;
    }
    else {
        std::cout << t.error().msg() << std::endl;
    }


}