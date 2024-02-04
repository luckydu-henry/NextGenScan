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
#define RANGES    ::std::ranges::
#if CXX_VERSION >= 202004L // Has cxx 20 support
#include <array>
#include <expected>
#include <format>
#include <ranges>
STD_BEGIN
namespace p1729r3 {

    // Some predefined concepts and types.
    enum class _Scan_arg_type : uint8_t {
        _None, 
        _Signed_i8,   _Signed_i16,   _Signed_i32,   _Signed_i64,    _Signed_long,
        _Unsigned_i8, _Unsigned_i16, _Unsigned_i32, _Unsigned_i64,  _Unsigned_long,
        _Float32,     _Float64,      _Float_ext,
        _Bool,        _Void_ptr,     _C_string,
        _Std_string,  _Custom
    };
    struct scan_error;
    // It's just a wrap which contains noting but a type alias.
    template <typename Ty>                                      class scan_skip;
    template <RANGES forward_range Rng, typename ... Args>      class scan_result;
    template <typename Ty, typename CharT>                      class scanner;
    template <class Context>                                    class basic_scan_arg;
    template <class Context>                                    class basic_scan_args;
    template <RANGES forward_range Rng, typename CharT>         class basic_scan_context;

    template<class CharT> using basic_scan_parse_context = basic_format_parse_context<CharT>;
    using                             scan_parse_context = basic_scan_parse_context<char>;
    using                            wscan_parse_context = basic_scan_parse_context<wchar_t>;
    template <class Rng>  using             scan_context = basic_scan_context<Rng, char>;
    template <class Rng>  using            wscan_context = basic_scan_context<Rng, wchar_t>;

    template<class T, class Context,
        class Scanner = typename Context::template scanner_type<remove_const_t<T>>>
    concept scannable_with =
        STD semiregular<Scanner> &&
        requires(Scanner & s, const Scanner & cs, T & t, Context & ctx,
    basic_scan_parse_context<typename Context::char_type>&pctx) {
        { s.parse(pctx) }    -> STD same_as<expected<typename decltype(pctx)::iterator, scan_error>>;
        { cs.scan(&t, ctx) } -> STD same_as<expected<typename Context::iterator, scan_error>>;
    };
    template<class Rng, class T, class CharT>
                                       concept scannable              = scannable_with<remove_reference_t<T>, basic_scan_context<Rng, CharT>>;
    template <class Rng, class CharT>  concept scannable_range        = RANGES forward_range<Rng> && STD same_as<RANGES range_value_t<Rng>, CharT>;
    template <class Rng, class...Args>   using scan_result_type       = expected<scan_result<RANGES borrowed_subrange_t<Rng>, Args...>, scan_error>;
    template <class Rng>                 using vscan_result_type      = expected<RANGES borrowed_subrange_t<Rng>, scan_error>;
    template <class Rng>                 using scan_from_result_type  = expected<RANGES borrowed_subrange_t<Rng>, scan_error>;
    template <class Rng>                 using scan_args              = basic_scan_args<basic_scan_context<Rng, char>>;
    template <class Rng>                 using wscan_args             = basic_scan_args<basic_scan_context<Rng, wchar_t>>;

    template <typename CharT>            using basic_parser_result_type  = std::expected<typename basic_scan_parse_context<CharT>::iterator, scan_error>;
    template <class Context>             using basic_scanner_result_type = std::expected<typename Context::iterator, scan_error>;


    // Basic structure of a scanner specification.
    template <typename Ty, typename CharT>
    class scanner {
        scanner()               = default;
        scanner(const scanner&) = delete;

        basic_parser_result_type<CharT>    parse(basic_scan_parse_context<CharT>& parse_ctx);
        template <typename Context> requires std::is_same_v<typename Context::char_type, CharT>
        basic_scanner_result_type<Context> scan(Ty* ptr, basic_scan_context<typename Context::range_type, CharT>& ctx);

        ~scanner() = default;
    };

    template <typename Ty>
    class scan_skip {
    public:
        using type = Ty;
        using pointer = Ty*;
        static constexpr pointer value = nullptr;
    };

    struct scan_error {
        enum code_type {
            good,
            end_of_range,
            invalid_format_string,
            invalid_scanned_value,
            value_out_of_range,
        };

        code_type        code;
        STD string_view  msg;

        constexpr operator bool() const { return good == code; }
    };

    template <RANGES forward_range Rng, typename ... Args>
    class scan_result {
    public:
        using range_type = Rng;
        using iterator   = RANGES iterator_t<Rng>;

        constexpr scan_result() = default;
        constexpr ~scan_result() = default;

        constexpr scan_result(range_type r, tuple<Args...>&& values) :
            range_(r), values_(STD forward<tuple<Args...>>(values)) {}

        template<class OtherR, class... OtherArgs>
        constexpr explicit scan_result(OtherR&& it, tuple<OtherArgs...>&& values) : range_(it), values_(values) {}

        constexpr scan_result(const scan_result&) = default;
        template<class OtherR, class... OtherArgs>
        constexpr explicit scan_result(const scan_result<OtherR, OtherArgs...>& other) : range_(other.range_), values_(other.values_) {}

        constexpr scan_result(scan_result&&) = default;
        template<class OtherR, class... OtherArgs>
        constexpr explicit scan_result(scan_result<OtherR, OtherArgs...>&& other) : range_(other.range_), values_(other.values_) {}

        constexpr scan_result& operator=(const scan_result&) = default;
        template<class OtherR, class... OtherArgs>
        constexpr scan_result& operator=(const scan_result<OtherR, OtherArgs...>& other) { range_ = other.range_; values_ = other.values_;  return *this;  }

        constexpr scan_result& operator=(scan_result&&) = default;
        template<class OtherR, class... OtherArgs>
        constexpr scan_result& operator=(scan_result<OtherR, OtherArgs...>&& other) { range_ = other.range_; values_ = other.values_; return *this; }

        constexpr range_type range() const { return range_; }

        constexpr iterator begin() const { return range_.begin(); }
        constexpr iterator end() const   { return range_.end(); }

        template<class Self>
        constexpr auto&& values(this Self&&) { return values_; }

        template<class Self>
        requires (sizeof...(Args) == 1)
        constexpr auto&& value(this Self&&) { return STD get<0>(values_); }
    private:
        range_type     range_;
        tuple<Args...> values_; 
    };

    template <class Context>
    class basic_scan_arg {
    public:
        class handle;
    private:
        using char_type = typename Context::char_type;
        
        union _Arg_variant {
            STD monostate                _None;
            signed char*                 _Signed_i8;
            short*                       _Signed_i16;
            int*                         _Signed_i32;
            long*                        _Signed_long;
            long long*                   _Signed_i64;
            unsigned char*               _Unsigned_i8;
            unsigned short*              _Unsigned_i16;
            unsigned int*                _Unsigned_i32;
            unsigned long*               _Unsigned_long;
            unsigned long long*          _Unsigned_i64;
            bool*                        _Bool;
            char_type**                  _C_string;
            void**                       _Void_ptr;
            float*                       _Float32;
            double*                      _Float64;
            long double*                 _Float_ext;
            STD basic_string<char_type>* _Std_string;
            handle                       _Custom;
        };

        _Arg_variant     value_;
        _Scan_arg_type   type_;
    public:
        // Implementation of handle which handles custom types scanning.
        class handle {
            void       *ptr_;
            scan_error(*scan_)(basic_scan_parse_context<char_type>& parse_ctx, Context scan_ctx, void*);

            template <typename Ty>
            scan_error scan_proto_(basic_scan_parse_context<char_type>& parse_ctx, Context& scan_ctx, void* ptr) {
                using value_type = STD remove_cvref_t<Ty>;
                typename Context::template scanner_type<value_type> scanner;

                if (auto pres = scanner.parse(parse_ctx); pres.has_value()) { parse_ctx.advance_to(pres.value()); } else { return pres.error(); }
                if (auto sres = scanner.scan((Ty*)ptr, scan_ctx); sres.has_value()) { scan_ctx.advance_to(sres.value()); } else { return sres.error(); }
                return scan_error{ scan_error::good, "" };
            }
        public:
            // When passing in a normal type these would be set to nullptr by default.
            constexpr explicit handle() = default;

            template <typename Ty> explicit handle(Ty& v)            : ptr_(STD addressof(v)),scan_(scan_proto_<Ty>) {}
            template <typename Ty> explicit handle(scan_skip<Ty>&)   : ptr_(nullptr)         ,scan_(scan_proto_<Ty>) {}

            decltype(auto) scan(basic_scan_parse_context<char_type>& parse_ctx, Context& scan_ctx) const {
                return scan_(parse_ctx, scan_ctx, ptr_);
            }

            ~handle() = default;
        };
        // Member functions.
        constexpr explicit basic_scan_arg() noexcept                               : type_(_Scan_arg_type::_None)          {value_._None = STD monostate{};}
        constexpr explicit basic_scan_arg(signed char* v) noexcept                 : type_(_Scan_arg_type::_Signed_i8)     {value_._Signed_i8 = v;}
        constexpr explicit basic_scan_arg(signed short* v) noexcept                : type_(_Scan_arg_type::_Signed_i16)    {value_._Signed_i16 = v;}
        constexpr explicit basic_scan_arg(signed int* v) noexcept                  : type_(_Scan_arg_type::_Signed_i32)    {value_._Signed_i32 = v;}
        constexpr explicit basic_scan_arg(signed long* v) noexcept                 : type_(_Scan_arg_type::_Signed_long)   {value_._Signed_long = v;}
        constexpr explicit basic_scan_arg(signed long long* v) noexcept            : type_(_Scan_arg_type::_Signed_i64)    {value_._Signed_i64 = v;}
        constexpr explicit basic_scan_arg(unsigned char* v) noexcept               : type_(_Scan_arg_type::_Unsigned_i8)   {value_._Unsigned_i8 = v;}
        constexpr explicit basic_scan_arg(unsigned short* v) noexcept              : type_(_Scan_arg_type::_Unsigned_i16)  {value_._Unsigned_i16 = v;}
        constexpr explicit basic_scan_arg(unsigned int* v) noexcept                : type_(_Scan_arg_type::_Unsigned_i32)  {value_._Unsigned_i32 = v;}
        constexpr explicit basic_scan_arg(unsigned long* v) noexcept               : type_(_Scan_arg_type::_Unsigned_long) {value_._Unsigned_long = v;}
        constexpr explicit basic_scan_arg(unsigned long long* v) noexcept          : type_(_Scan_arg_type::_Unsigned_i64)  {value_._Unsigned_i64 = v;}
        constexpr explicit basic_scan_arg(float* v) noexcept                       : type_(_Scan_arg_type::_Float32)       {value_._Float32 = v;}
        constexpr explicit basic_scan_arg(double* v) noexcept                      : type_(_Scan_arg_type::_Float64)       {value_._Float64 = v;}
        constexpr explicit basic_scan_arg(long double* v) noexcept                 : type_(_Scan_arg_type::_Float_ext)     {value_._Float_ext = v;}
        constexpr explicit basic_scan_arg(bool* v) noexcept                        : type_(_Scan_arg_type::_Bool)          {value_._Bool = v;}
        constexpr explicit basic_scan_arg(char_type** v) noexcept                  : type_(_Scan_arg_type::_C_string)      {value_._C_string = v;}
        constexpr explicit basic_scan_arg(void** v) noexcept                       : type_(_Scan_arg_type::_Void_ptr)      {value_._Void_ptr = v;}
        constexpr explicit basic_scan_arg(STD basic_string<char_type>* v) noexcept : type_(_Scan_arg_type::_Std_string)    {value_._Std_string = v;}

        constexpr explicit basic_scan_arg(scan_skip            <signed char>*)  : basic_scan_arg(scan_skip<signed char>::value) {}
        constexpr explicit basic_scan_arg(scan_skip                  <short>*)  : basic_scan_arg(scan_skip<short>::value) {}
        constexpr explicit basic_scan_arg(scan_skip                   <long>*)  : basic_scan_arg(scan_skip<long>::value) {}
        constexpr explicit basic_scan_arg(scan_skip                    <int>*)  : basic_scan_arg(scan_skip<int>::value) {}
        constexpr explicit basic_scan_arg(scan_skip              <long long>*)  : basic_scan_arg(scan_skip<long long>::value) {}
        constexpr explicit basic_scan_arg(scan_skip          <unsigned char>*)  : basic_scan_arg(scan_skip<unsigned char>::value) {}
        constexpr explicit basic_scan_arg(scan_skip         <unsigned short>*)  : basic_scan_arg(scan_skip<unsigned short>::value) {}
        constexpr explicit basic_scan_arg(scan_skip           <unsigned int>*)  : basic_scan_arg(scan_skip<unsigned int>::value) {}
        constexpr explicit basic_scan_arg(scan_skip          <unsigned long>*)  : basic_scan_arg(scan_skip<unsigned long>::value) {}
        constexpr explicit basic_scan_arg(scan_skip     <unsigned long long>*)  : basic_scan_arg(scan_skip<unsigned long long>::value) {}
        constexpr explicit basic_scan_arg(scan_skip                  <float>*)  : basic_scan_arg(scan_skip<float>::value) {}
        constexpr explicit basic_scan_arg(scan_skip                 <double>*)  : basic_scan_arg(scan_skip<double>::value) {}
        constexpr explicit basic_scan_arg(scan_skip            <long double>*)  : basic_scan_arg(scan_skip<long double>::value) {}
        constexpr explicit basic_scan_arg(scan_skip                   <bool>*)  : basic_scan_arg(scan_skip<bool>::value) {}
        constexpr explicit basic_scan_arg(scan_skip             <char_type*>*)  : basic_scan_arg(scan_skip<char_type*>::value) {}
        constexpr explicit basic_scan_arg(scan_skip                  <void*>*)  : basic_scan_arg(scan_skip<void*>::value) {}
        constexpr explicit basic_scan_arg(scan_skip<basic_string<char_type>>*)  : basic_scan_arg(scan_skip<basic_string<char_type>>::value) {}

        // Handle accepts both writable values and ignored values.
        constexpr basic_scan_arg(handle v) noexcept : type_(_Scan_arg_type::_Custom) { value_._Custom = v; }

        explicit operator bool() const noexcept {
            return type_ != _Scan_arg_type::_None;
        }
        // Visit member function can be used to replace visit_scan_arg.
        template <typename Visitor>
        decltype(auto) visit(Visitor&& vis) {
            switch (type_) {
            case _Scan_arg_type::_None:           return STD forward<Visitor>(vis)(value_._None);       
            case _Scan_arg_type::_Signed_i8:      return STD forward<Visitor>(vis)(value_._Signed_i8);  
            case _Scan_arg_type::_Signed_i16:     return STD forward<Visitor>(vis)(value_._Signed_i16); 
            case _Scan_arg_type::_Signed_i32:     return STD forward<Visitor>(vis)(value_._Signed_i32); 
            case _Scan_arg_type::_Signed_i64:     return STD forward<Visitor>(vis)(value_._Signed_i64); 
            case _Scan_arg_type::_Signed_long:    return STD forward<Visitor>(vis)(value_._Signed_long);
            case _Scan_arg_type::_Unsigned_i8:    return STD forward<Visitor>(vis)(value_._Unsigned_i8);
            case _Scan_arg_type::_Unsigned_i16:   return STD forward<Visitor>(vis)(value_._Unsigned_i16);  
            case _Scan_arg_type::_Unsigned_i32:   return STD forward<Visitor>(vis)(value_._Unsigned_i32);  
            case _Scan_arg_type::_Unsigned_i64:   return STD forward<Visitor>(vis)(value_._Unsigned_i64);  
            case _Scan_arg_type::_Unsigned_long:  return STD forward<Visitor>(vis)(value_._Unsigned_long); 
            case _Scan_arg_type::_Float32:        return STD forward<Visitor>(vis)(value_._Float32);       
            case _Scan_arg_type::_Float64:        return STD forward<Visitor>(vis)(value_._Float64);       
            case _Scan_arg_type::_Float_ext:      return STD forward<Visitor>(vis)(value_._Float_ext);     
            case _Scan_arg_type::_Bool:           return STD forward<Visitor>(vis)(value_._Bool);          
            case _Scan_arg_type::_Void_ptr:       return STD forward<Visitor>(vis)(value_._Void_ptr);      
            case _Scan_arg_type::_C_string:       return STD forward<Visitor>(vis)(value_._C_string);      
            case _Scan_arg_type::_Std_string:     return STD forward<Visitor>(vis)(value_._Std_string);    
            case _Scan_arg_type::_Custom:         return STD forward<Visitor>(vis)(value_._Custom);        
            }
        }
    };

    template <typename Ty, class Context>
    struct _Arg_ptr_cast { constexpr auto operator()(Ty& v) { return basic_scan_arg<Context>::handle(v); } };

#define DECL_ARG_PTR_CAST(type) \
    template <typename Context> struct _Arg_ptr_cast<type, Context>{constexpr auto operator()(type& v) { return &v; }}

    DECL_ARG_PTR_CAST(signed char);
    DECL_ARG_PTR_CAST(short);
    DECL_ARG_PTR_CAST(int);
    DECL_ARG_PTR_CAST(long);
    DECL_ARG_PTR_CAST(long long);
    DECL_ARG_PTR_CAST(unsigned char);
    DECL_ARG_PTR_CAST(unsigned short);
    DECL_ARG_PTR_CAST(unsigned int);
    DECL_ARG_PTR_CAST(unsigned long);
    DECL_ARG_PTR_CAST(unsigned long long);
    DECL_ARG_PTR_CAST(bool);
    DECL_ARG_PTR_CAST(typename Context::char_type*);
    DECL_ARG_PTR_CAST(void*);
    DECL_ARG_PTR_CAST(float);
    DECL_ARG_PTR_CAST(double);
    DECL_ARG_PTR_CAST(long double);
    DECL_ARG_PTR_CAST(basic_string<typename Context::char_type>);
    DECL_ARG_PTR_CAST(scan_skip<signed char>);
    DECL_ARG_PTR_CAST(scan_skip<short>);
    DECL_ARG_PTR_CAST(scan_skip<int>);
    DECL_ARG_PTR_CAST(scan_skip<long>);
    DECL_ARG_PTR_CAST(scan_skip<long long>);
    DECL_ARG_PTR_CAST(scan_skip<unsigned char>);
    DECL_ARG_PTR_CAST(scan_skip<unsigned short>);
    DECL_ARG_PTR_CAST(scan_skip<unsigned int>);
    DECL_ARG_PTR_CAST(scan_skip<unsigned long>);
    DECL_ARG_PTR_CAST(scan_skip<unsigned long long>);
    DECL_ARG_PTR_CAST(scan_skip<bool>);
    DECL_ARG_PTR_CAST(scan_skip<typename Context::char_type*>);
    DECL_ARG_PTR_CAST(scan_skip<void*>);
    DECL_ARG_PTR_CAST(scan_skip<float>);
    DECL_ARG_PTR_CAST(scan_skip<double>);
    DECL_ARG_PTR_CAST(scan_skip<long double>);
    DECL_ARG_PTR_CAST(scan_skip<basic_string<typename Context::char_type>>);

#undef DECL_ARG_PTR_CAST



    template <class Context, typename ... Args>
    class basic_scan_arg_store {
    public:
        STD tuple<Args ...>                                   args;
        STD array<basic_scan_arg<Context>, sizeof ... (Args)> data;

        basic_scan_arg_store(Args& ... _args) :
        args(STD make_tuple(_args...)), data({ basic_scan_arg<Context>{_Arg_ptr_cast<std::remove_cvref_t<decltype(_args)>, Context>{}(_args)} ...}) {}
    };

    template <class Context>
    class basic_scan_args {
        size_t                           size_;
        const basic_scan_arg<Context>*   data_; // Pointer to actual format store.
    public:
        basic_scan_args() noexcept = default;
        template <typename ... Args>
        basic_scan_args(const basic_scan_arg_store<Context, Args...>& store) noexcept :
        size_(sizeof ... (Args)), data_(&(store.data[0])) {}

        basic_scan_arg<Context> get(size_t id) const {
            if (id >= size_) {
                throw STD out_of_range("scan arg index out of range!");
            }
            return *(data_ + id);
        }
        size_t                  size() const {
            return size_;
        }
    };

    template <RANGES forward_range Rng, typename CharT>
    class basic_scan_context {
    public:
        using char_type        = CharT;
        using range_type       = Rng;
        using iterator         = RANGES iterator_t<range_type>;
        using sentinel         = RANGES sentinel_t<range_type>;
        template <typename Ty>
        using scanner_type     = scanner<Ty, CharT>;

        constexpr basic_scan_context(Rng rg, basic_scan_args<basic_scan_context> args):
            current_(rg.begin()), end_(rg.end()), args_(args) {}
        constexpr basic_scan_context(Rng rg, basic_scan_args<basic_scan_context> args, const std::locale& loc) :
            current_(rg.begin()), end_(rg.end()), locale_(loc), args_(args) {}

        constexpr basic_scan_arg<basic_scan_context> arg(size_t id) const noexcept { return args_.get(id); }
        STD locale                                   locale() { return locale_; }
        constexpr iterator                           current() const { return current_; }
        constexpr range_type                         range()   const { return range_type{ current_, end_ }; }
        constexpr void                               advance_to(iterator it) { current_ = it; }
    private:
        iterator                            current_;
        sentinel                            end_;
        STD locale                          locale_;
        basic_scan_args<basic_scan_context> args_;
    };





    template<scannable_range<char> Rng>
    vscan_result_type<Rng> vscan(Rng&& range, string_view fmt, scan_args<Rng> args);

    template<scannable_range<wchar_t> Rng>
    vscan_result_type<Rng> vscan(Rng&& range, wstring_view fmt, wscan_args<Rng> args);

    template<scannable_range<char> Rng>
    vscan_result_type<Rng> vscan(const locale& loc, Rng&& range, string_view fmt, scan_args<Rng> args);

    template<scannable_range<wchar_t> Rng>
    vscan_result_type<Rng> vscan(const locale& loc, Rng&& range, wstring_view fmt, wscan_args<Rng> args);

    template<class... Args, scannable_range<char> Rng>
    scan_result_type<Rng, Args...> scan(Rng&& range, STD string_view fmt);

    template<class... Args, scannable_range<wchar_t> Rng>
    scan_result_type<Rng, Args...> scan(Rng&& range, STD wstring_view fmt);

    template<class... Args, scannable_range<char> Rng>
    scan_result_type<Rng, Args...> scan(const locale& loc, Rng&& range, STD string_view fmt);

    template <class... Args, scannable_range<wchar_t> Rng>
    scan_result_type<Rng, Args...> scan(const locale& loc, Rng&& range, STD wstring_view fmt);

    template<class ... Args, scannable_range<char> Rng>
    scan_from_result_type<Rng> scan_from(Rng&& range, string_view fmt, Args& ... args);

    template<class ... Args, scannable_range<wchar_t> Rng>
    scan_from_result_type<Rng> scan_from(Rng&& range, wstring_view fmt, Args& ... args);

    template<class ... Args, scannable_range<char> Rng>
    scan_from_result_type<Rng> scan_from(const locale& loc, Rng&& range, string_view fmt, Args& ... args);

    template<class ... Args, scannable_range<wchar_t> Rng>
    scan_from_result_type<Rng> scan_from(const locale& loc, Rng&& range, wstring_view fmt, Args& ... args);


    template <class Rng, class ... Args>
    constexpr basic_scan_arg_store<scan_context<Rng>, Args ...> make_scan_arg_store(Args& ... args) {
        return basic_scan_arg_store<scan_context<Rng>, Args ...> { args... };
    }
    template<class Rng, class... Args>
    constexpr basic_scan_args<scan_context<Rng>> make_scan_args(basic_scan_arg_store<scan_context<Rng>,Args...> ast) {
        return basic_scan_args<scan_context<Rng>>{ast};
    }
    template <class Rng, class ... Args>
    constexpr basic_scan_arg_store<wscan_context<Rng>, Args ...> make_wscan_arg_store(Args& ... args) {
        return basic_scan_arg_store<wscan_context<Rng>, Args ...> { args... };
    }
    template<class Rng, class... Args>
    constexpr basic_scan_args<wscan_context<Rng>> make_wscan_args(basic_scan_arg_store<wscan_context<Rng>, Args...> ast) {
        return basic_scan_args<wscan_context<Rng>>{ast};
    }
    template<class Rng, class Context, class... Args>
    expected<scan_result<Rng, Args...>, scan_error> make_scan_result(expected<Rng, scan_error>&& source, basic_scan_arg_store<Context, Args...>&& args);
} //! namespace p1729r3
STD_END
#else
#error "A C++20 compiler is recommended."
#endif
#endif //! _STD_P1729R3_SCAN
