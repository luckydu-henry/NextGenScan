#ifndef _STD_P1729R3_SCAN
#define _STD_P1729R3_SCAN

#ifdef _MSC_VER
#    define CXX_VERSION _MSVC_LANG
#else
#    define CXX_VERSION __cplusplus
#endif
// Predefined macros.
#define STD_BEGIN namespace std {
#define STD_END   }
#define STD       ::std::

#if CXX_VERSION >= 202004L // Has cxx 20 support
#include <expected>
#include <format>
#include <ranges>
#include <variant>
STD_BEGIN
namespace p1729r3 {

    // Some predefined concepts and types.
                                                                class scan_error;
    template <STD ranges::forward_range Rng, typename ... Args> class scan_result;
    template <typename Ty, typename CharT>                      class scanner;
    template <class Context>                                    class basic_scan_arg;
    template <class Context>                                    class basic_scan_args;
    template <STD ranges::forward_range Rng, typename CharT>    class basic_scan_context;

    template<class CharT> using basic_scan_parse_context = basic_format_parse_context<CharT>;
    using                             scan_parse_context = basic_scan_parse_context<char>;
    using                            wscan_parse_context = basic_scan_parse_context<wchar_t>;

    template<class T, class Context,
        class Scanner = typename Context::template scanner_type<remove_const_t<T>>>
    concept scannable_with =
        STD semiregular<Scanner> &&
        requires(Scanner & s, const Scanner & cs, T & t, Context & ctx,
    basic_scan_parse_context<typename Context::char_type>&pctx) {
        { s.parse(pctx) }   -> STD same_as<expected<typename decltype(pctx)::iterator, scan_error>>;
        { cs.scan(t, ctx) } -> STD same_as<expected<typename Context::iterator, scan_error>>;
    };
    template<class Rng, class T, class CharT>
                                      concept scannable         = scannable_with<remove_reference_t<T>, basic_scan_context<Rng, CharT>>;
    template<class Rng, class CharT>  concept scannable_range   = STD ranges::forward_range<Rng> && STD same_as<ranges::range_value_t<Rng>, CharT>;
    template<class Rng, class...Args>   using scan_result_type  = expected<scan_result<STD ranges::borrowed_subrange_t<Rng>, Args...>, scan_error>;
    template<class Rng, typename CharT> using scan_args_for     = basic_scan_args<basic_scan_context<CharT, STD ranges::range_value_t<Rng>>>;
    template<class Rng>                 using vscan_result_type = expected<STD ranges::borrowed_subrange_t<Rng>, scan_error>;

    // Implementations starts from here!

    class scan_error {
    public:
        enum code_type {
            good,
            end_of_range,
            invalid_format_string,
            invalid_scanned_value,
            value_out_of_range
        };
        constexpr             scan_error() = default;
        constexpr             scan_error(code_type error_code, const char* message);
        constexpr explicit    operator bool() const noexcept;
        constexpr code_type   code() const noexcept;
        constexpr const char* msg() const;

    private:
        code_type    code_;   
        const char  *message_;
    };

    template <STD ranges::forward_range Rng, typename ... Args>
    class scan_result {
    public:
        using range_type = Rng;
        using iterator   = STD ranges::iterator_t<Rng>;

        constexpr scan_result() = default;
        constexpr ~scan_result() = default;

        constexpr scan_result(range_type r, tuple<Args...>&& values);

        template<class OtherR, class... OtherArgs>
        constexpr explicit scan_result(OtherR&& it, tuple<OtherArgs...>&& values);

        constexpr scan_result(const scan_result&) = default;
        template<class OtherR, class... OtherArgs>
        constexpr explicit scan_result(const scan_result<OtherR, OtherArgs...>& other);

        constexpr scan_result(scan_result&&) = default;
        template<class OtherR, class... OtherArgs>
        constexpr explicit scan_result(scan_result<OtherR, OtherArgs...>&& other);

        constexpr scan_result& operator=(const scan_result&) = default;
        template<class OtherR, class... OtherArgs>
        constexpr scan_result& operator=(const scan_result<OtherR, OtherArgs...>& other);

        constexpr scan_result& operator=(scan_result&&) = default;
        template<class OtherR, class... OtherArgs>
        constexpr scan_result& operator=(scan_result<OtherR, OtherArgs...>&& other);

        constexpr range_type range() const;

        constexpr iterator begin() const;
        constexpr iterator end() const;

        template<class Self>
        constexpr auto&& values(this Self&&);

        template<class Self>
            requires (sizeof...(Args) == 1)
        constexpr auto && value(this Self&&);
    private:
        range_type     range_;
        tuple<Args...> values_; 
    };
    
    template <typename Ty, typename CharT>
    class scanner {};

    template <class Context>
    class basic_scan_arg {
    public:
        // Implementation of handle which handles custom types scanning.
        class handle {
            
        };

        // Member functions.
        basic_scan_arg() noexcept;
        explicit operator bool() const noexcept;
        // Visit member function can be used to replace visit_scan_arg.
        template <typename Visitor>
        decltype(auto) visit(this basic_scan_arg arg, Visitor&& vis) {
            return std::visit(std::forward<Visitor>(vis), arg);
        }
    private:
        using char_type = typename Context::char_type;

        STD variant<
            signed   char*, short*, int*, long*, long long*,
            unsigned char*, unsigned short*, unsigned int*, unsigned long*, unsigned long long*,
            bool*, char_type*, void**,
            float*, double*, long double*,
            std::basic_string<char_type>*, std::basic_string_view<char_type>*,
            handle
        > value;
        template <class T> basic_scan_arg(T& v) noexcept;
    };

    template <class Context, typename ... Args>
    class scan_arg_store {
    public:
        STD tuple<Args...>                                    args;
        STD array<basic_scan_arg<Context>, sizeof ... (Args)> data;
    };

    template <class Context>
    class basic_scan_args {
        size_t                     size_;
        basic_scan_arg<Context>*   data_;
    public:
        basic_scan_args() noexcept;
        template <typename ... Args>
        basic_scan_args(scan_arg_store<Context, Args...>& store) noexcept;

        basic_scan_arg<Context> get(size_t id) noexcept;
    };

    template <STD ranges::forward_range Rng, typename CharT>
    class basic_scan_context {
    public:
        using char_type        = CharT;
        using range_type       = Rng;
        using iterator         = STD ranges::iterator_t<range_type>;
        using sentinel         = STD ranges::sentinel_t<range_type>;
        template <typename Ty>
        using scanner_type     = scanner<Ty, CharT>;

        constexpr basic_scan_context(Rng rg, basic_scan_args<basic_scan_context> args) :
            current_(rg.begin()), end_(rg.end()), args_(args) {}
        constexpr basic_scan_context(Rng rg, basic_scan_args<basic_scan_context> args, const std::locale& loc) :
            current_(rg.begin()), end_(rg.end()), locale_(loc), args_(args) {}

        constexpr basic_scan_arg<basic_scan_context> arg(size_t id) const noexcept;
        STD locale                                   locale();
        constexpr iterator                           current() const;
        constexpr range_type                         range()   const;
        constexpr void                               advance_to(iterator it);
    private:
        iterator                            current_;
        sentinel                            end_;
        STD locale                          locale_;
        basic_scan_args<basic_scan_context> args_;
    };

    template<class... Args, scannable_range<char> Rng>
    scan_result_type<Rng, Args...> scan(Rng&& range, STD string_view fmt);

    template<class... Args, scannable_range<wchar_t> Rng>
    scan_result_type<Rng, Args...> scan(Rng&& range, STD wstring_view fmt);

    template<class... Args, scannable_range<char> Rng>
    scan_result_type<Rng, Args...> scan(const locale& loc, Rng&& range, STD string_view fmt);

    template <class... Args, scannable_range<wchar_t> Rng>
    scan_result_type<Rng, Args...> scan(const locale& loc, Rng&& range, STD wstring_view fmt);

    template<scannable_range<char> Rng>
    vscan_result_type<Rng> vscan(Rng&& range, string_view fmt, scan_args_for<Rng, char> args);

    template<scannable_range<wchar_t> Rng>
    vscan_result_type<Rng> vscan(Rng&& range, wstring_view fmt, scan_args_for<Rng, wchar_t> args);

    template<scannable_range<char> Rng>
    vscan_result_type<Rng> vscan(const locale& loc, Rng&& range, string_view fmt, scan_args_for<Rng, char> args);

    template<scannable_range<wchar_t> Rng>
    vscan_result_type<Rng> vscan(const locale& loc, Rng&& range, wstring_view fmt, scan_args_for<Rng, wchar_t> args);

    template<class Rng, class... Args>
    constexpr basic_scan_args<scan_parse_context> make_scan_args(Args& ... args);;

    template<class Rng, class... Args>
    constexpr basic_scan_args<wscan_parse_context> make_wscan_args(Args& ... args);;

    template<class Rng, class Context, class... Args>
    expected<scan_result<Rng, Args...>, scan_error> make_scan_result(expected<Rng, scan_error>&& source, scan_arg_store<Context, Args...>&& args);
} //! namespace p1729r3
STD_END
#else
#error "A C++20 compiler is recommended."
#endif
#endif //! _STD_P1729R3_SCAN
