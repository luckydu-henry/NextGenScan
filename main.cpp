#include <expected>
#include <ranges>
#include <format>
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>
#include <fstream>
#include <numeric>
#include <string>
#include <string_view>

#include "fmtcore.h"
#include "fmtformat.h"
#include "std_scan_p1729r3.hpp"
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

    if (j != -1) { ptx.check_arg_id(j); } else { j = ptx.next_arg_id(); }
    return std::make_tuple(i, j);

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
std::p1729r3::basic_parser_result_type<CharT> _Parse_basic(const std::p1729r3::basic_scan_parse_context<CharT>& pctx, 
                                                                    _Basic_scn_specs<CharT>& specs) {
    auto i = pctx.begin();
    for (;*i != '}'; ++i) {
        // Do format specifies.
    }
    // Return the next element of }
    return std::next(i);
}

template <typename Ty, class Context>
std::p1729r3::basic_scanner_result_type<Context> _Scan_basic(const Context& sctx, Ty* ptr, 
                                                             const _Basic_scn_specs<typename Context::char_type>& specs) {

    auto rng = sctx.range();
    char buf[1200] = {};

    using namespace std::string_view_literals;
    using char_type = typename Context::char_type;
    namespace ranges = std::ranges;

    std::string_view digit_set = "0123456789"sv,
                     hexlt_set = "abcdef"sv,
                     hexbg_set = "ABCDEF"sv,
                     flt_dot   = "."sv,
                     hexpos_lt = "p"sv,
                     hexpos_bg = "P"sv,
                     exp_lt    = "e"sv,
                     exp_bg    = "E"sv;

    if constexpr (std::is_same_v<Ty, bool>) {
        return std::unexpected(std::p1729r3::scan_error(std::p1729r3::scan_error::invalid_scanned_value, "does not support now!"));
    }
    if constexpr (std::integral<Ty> && !std::is_same_v<Ty, bool>) {
        // Boolean value only contains true or false.
        Ty         v = 0;
        char*      q = buf;
        // Can't be replaced by copy_if
        for (auto i = rng.begin(); i != rng.end() && ranges::contains(digit_set, static_cast<char>(*i)); ++i) { *q++ = (*i) & 0xFF; }
        auto res = std::from_chars(buf, q, v);
        // Check whether we should write in this value.
        if (ptr) { *ptr = v; }
        return std::next(rng.begin(), res.ptr - buf);
    }
    if constexpr (std::floating_point<Ty>) {
        return std::unexpected(std::p1729r3::scan_error(std::p1729r3::scan_error::invalid_scanned_value, "does not support now!"));
    }
    if constexpr (std::is_same_v<Ty, void*>) {
        return std::unexpected(std::p1729r3::scan_error(std::p1729r3::scan_error::invalid_scanned_value, "does not support now!"));
    }
    // Input char should be allocated first!
    if constexpr (std::is_same_v<Ty, char_type*>) {
        return std::unexpected(std::p1729r3::scan_error(std::p1729r3::scan_error::invalid_scanned_value, "does not support now!"));
    }
    if constexpr (std::is_same_v<Ty, std::basic_string<char_type>>) {
        return std::unexpected(std::p1729r3::scan_error(std::p1729r3::scan_error::invalid_scanned_value, "does not support now!"));
    }
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
        auto pres = _Parse_basic(pctx, specs);
        if (pres.has_value()) { pctx.advance_to(pres.value()); }
        else { return std::unexpected(pres.error()); }
        return _Scan_basic(sctx, p, specs);
    }
    result_type operator()(typename std::p1729r3::basic_scan_arg<std::p1729r3::basic_scan_context<Rng, char>>::handle& hd) {
        auto result = hd.scan(pctx, sctx);
        if (result) { return  sctx.current(); }
        return std::unexpected(result);
    }
};

template <std::p1729r3::scannable_range<char> Rng>
std::p1729r3::vscan_result_type<Rng> format_from(Rng rg, std::string_view fmt, std::p1729r3::scan_args<Rng> args) {
    std::p1729r3::scan_context<Rng>   ctx{ rg, args };
    std::p1729r3::scan_parse_context  ptx{ fmt, args.size()};

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
                        else { return std::unexpected(k.error()); }
                    }
                    else  return std::unexpected(v.error());
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



// Construct an input range to a bidirectional range.
template <typename CharT>
class basic_scannable_istream {
public:
    using char_type = CharT;

    constexpr basic_scannable_istream(std::basic_istream<CharT>& strm) : ptr_(strm) , beg_(*this) {
        ptr_.seekg(0, std::ios::end); end_ = iterator(*this);
        ptr_.seekg(0, std::ios::beg);
    }
    // No default constructor.
    constexpr basic_scannable_istream() = delete;
    constexpr basic_scannable_istream(const basic_scannable_istream&) = default;
    constexpr basic_scannable_istream& operator=(const basic_scannable_istream&) = default;
    constexpr basic_scannable_istream(basic_scannable_istream&& other) noexcept : ptr_(other.ptr_), beg_(other.beg_), end_(other.end_) {}

    class iterator {
        friend class basic_scannable_istream;
        static constexpr unsigned int chars_infinity_ = std::numeric_limits<unsigned int>::infinity();
    public:
#ifdef __cpp_lib_concepts
        using iterator_concept  = std::forward_iterator_tag;
#endif // __cpp_lib_concepts
        using iterator_category = std::forward_iterator_tag;
        using value_type        = unsigned int; // For unicode and binary mode read.
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        iterator() = default;
        iterator(basic_scannable_istream& view) : view_(std::addressof(view)), position_(view.ptr_.tellg()) {}

        iterator(const iterator&)            = default;
        iterator(iterator&&)                 = default;
        iterator& operator=(iterator&&)      = default;
        iterator& operator=(const iterator&) = default;

        value_type& operator*() const {
            if (val_ == chars_infinity_) {
                view_->ptr_.seekg(position_, std::ios::beg).get(reinterpret_cast<char_type&>(val_));
                view_->ptr_.seekg(-1, std::ios::cur);
            }
            return val_;
        }
        iterator&      operator++()    { val_ = chars_infinity_; ++position_; return *this; }
        iterator       operator++(int) { val_ = chars_infinity_; position_++; return *this; }
        constexpr bool operator==(const iterator& other) const {
            return position_ == other.position_;
        }
    private:
        mutable basic_scannable_istream* view_      = nullptr;
        mutable value_type               val_       = chars_infinity_;
        std::size_t                      position_  = 0;
    };
    // Range constructor.
    constexpr basic_scannable_istream(iterator beg, iterator end) : ptr_(beg.view_->ptr_), beg_(beg), end_(end) {}

    constexpr iterator begin() const { return beg_; }
    constexpr iterator end()   const { return end_; }

private:
    std::basic_istream<char_type>& ptr_;
    iterator                       beg_, end_;
};

int main(int argc, char* argv[]) {
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    namespace scn = std::p1729r3;

    int p = 0, q, m; unsigned long long o;

    std::stringstream      strm{ "12345 6789 9981 1928374655" };
    basic_scannable_istream rng{ strm };

    auto k0 = scn::make_scan_arg_store<decltype(rng)>(p, q, m, o);
    auto k = scn::make_scan_args(k0);
    auto t = format_from(rng, "{}{}{}{}", k);

    if (t.has_value()) {
        std::cout << o << std::endl;
    }
    else {
        std::cout << t.error().msg << std::endl;
    }

}