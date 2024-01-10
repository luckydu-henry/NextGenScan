#pragma once

#include <unordered_set>
#include <cstdarg>
#include <stdexcept>
#include <charconv>

// dds is a modern C++ scanf implementation.
namespace dds {
    namespace detail {
        // thread_local makes it thread safe -- char tables for range matching.
        thread_local std::unordered_set<char>    t_char_set;
        thread_local std::unordered_set<wchar_t> t_wchar_set;
    }
    template <typename CharT> std::unordered_set<CharT>& char_set();
    template <> inline std::unordered_set<char>& char_set() { return detail::t_char_set; }
    template <> inline std::unordered_set<wchar_t>& char_set() { return detail::t_wchar_set; }

    union input_arg {
        int* i32;
        unsigned int* u32;
        float* f32;
        char* ch;
        char* str; // This one should allocate first before passing in as arg.
        void** ptr;
        bool* bln;
        // These types should combine with state.
        long* li;
        long long* i64;
        unsigned long long* u64;
        unsigned long* uli;
        double* f64;
        long double* llf;
        short* i16;
        unsigned short* u16;
        signed char* i8;
        unsigned char* u8;
    };

    enum format_type : unsigned char {
        signed_int, unsigned_int, big_norm_float, big_exp_float,
        little_norm_float, little_exp_float, character, string,
        big_hex, little_hex, octal_int, pointer, boolean, range, custom
    };
    // boolean read as 'true' or 'false'.

    struct input_replacement {
        // Layout of the state bit mask(endian is little).
        // ------------------------------------------------------
        // | bit position |   usage   |         explain         |
        // ------------------------------------------------------
        // |      0       |  escaped  | First char is {         |
        // ------------------------------------------------------
        // |      1       |  ignored  | First char is *         |
        // ------------------------------------------------------
        // |      2       |   sign    | Has +- notation         |
        // ------------------------------------------------------
        // |      3       | precision | Floating point has .    |
        // ------------------------------------------------------
        // |      4       | nincluded | Range has ^             |
        // ------------------------------------------------------
        // |     5~7      |   length  | l/ll/h/hh -> 1,2,3,4    |
        // ------------------------------------------------------
        uint8_t                        state;
        size_t                         width;
        format_type                    type;
    };

    // Beg starts from the first { and reach to 
    const char* parse_replacement(input_replacement& rep, const char* beg, const char* end) {
        // Initial settings.
        rep.state = 0;
        rep.width = 0;
        // Escape character
        if (*beg == '{') {
            rep.state |= 0x01;
            return beg + 1;
        }
        const auto r = std::find(beg, end, '}');
        // Has an invalid '}'
        if (*(r + 1) == '}' && *(r + 2) != '}') {
            throw std::runtime_error("Invalid } character!");
        }
        char_set<char>().clear();
        // Get the position of ':' after this are actual formatters.
        auto l = std::find(beg, end, ':');
        // std::from_chars(beg, l) // This range contains the index.

        ++l; // Now we start from formatters.
        for (; l != r; ++l) {
            switch (*l) {
            case '*':
                rep.state |= 0x02; break;
            case '+':
                rep.state |= 0x04; break;
                // Check width
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                rep.width = rep.width * 10 + *l - '0'; break;
            case '.':
                rep.state |= 0x08;                        break;
            case 'l':
                // ll mode
                if (*(l + 1) == *l) {
                    rep.state |= 0x40; ++l;
                }
                // l mode
                else { rep.state |= 0x20; } break;
            case 'h':
                // hh mode
                if (*(l + 1) == *l) {
                    rep.state |= 0x80; ++l;
                }
                else { rep.state |= 0x60; } break;
            case 'd': case 'i':
                rep.type = format_type::signed_int;
                if (rep.width == 0) { rep.width = 10; }
                break;
            case 'u':
                rep.type = format_type::unsigned_int;
                if (rep.width == 0) { rep.width = 10; }
                break;
            case 'g': case 'f':
                rep.type = format_type::little_norm_float;
                rep.width = (rep.state & 0x20) ? 13 : 8;
                break;
            case 'G': case 'F':
                rep.type = format_type::big_norm_float;
                rep.width = (rep.state & 0x20) ? 13 : 8;
                break;
                // TODO: Implement exponent form floating point!
            case 'e':
                rep.type = format_type::little_exp_float;     break;
            case 'E':
                rep.type = format_type::big_exp_float;        break;
            case 'c':
                rep.type = format_type::character;
                if (rep.width == 0) { rep.width = 1; }
                break;
            case 's':
                rep.type = format_type::string;               break;
            case 'x':
                rep.type = format_type::little_hex;
                if (rep.width == 0) { rep.width = 8; }
                break;
            case 'X':
                rep.type = format_type::big_hex;
                if (rep.width == 0) { rep.width = 8; }
                break;
            case 'o':
                rep.type = format_type::octal_int;
                if (rep.width == 0) { rep.width = 11; }
                break;
            case 'p':
                rep.type = format_type::pointer;
                rep.width = 16; // For 64-bit system it's just a 64bit integer.
                break;
            case 'b':
                rep.type = format_type::boolean;              break;
            case '[':
                rep.type = format_type::range;
                // Start from the next one of '['.
                ++l;
                for (; *l != ']'; ++l) {
                    switch (*l) {
                    case '^':
                        if (!(rep.state & 0x10)) {
                            rep.state |= 0x10;
                        }
                        else { char_set<char>().insert(*l); }
                        break;
                    case '-':
                        if (*(l + 1) != '-') {
                            for (char i = *(l - 1); i != *(l + 1); ++i) {
                                char_set<char>().insert(i);
                            }
                        }
                        else {
                            char_set<char>().insert('-');
                        }
                        break;
                    default:
                        char_set<char>().insert(*l);
                        break;
                    }
                }
                break;
            default:
                return l; // Invalid format.
            }
        }

        return r + 1;
    }
    constexpr inline bool is_empty_char(char v) {
        switch (v) {
        case '\n': case '\r': case '\t': case  ' ':
            return true;
        default:
            return false;
        }
    }
    // Buf should point to the start of replacement.
    const char* read_replacement(const input_replacement& rep, const char* buf, input_arg* p) {
        switch (rep.type) {
        case signed_int: {
            switch ((rep.state >> 5) & 3) {
            case 0: {
                int v = 0;
                buf = std::from_chars(buf, buf + rep.width, v).ptr;
                if (p != nullptr) { *p->i32 = v; }
                break;
            }
            case 1: {
                long v = 0;
                buf = std::from_chars(buf, buf + rep.width, v).ptr;
                if (p != nullptr) { *p->li = v; }
                break;
            }
            case 2: {
                long long v = 0;
                buf = std::from_chars(buf, buf + rep.width, v).ptr;
                if (p != nullptr) { *p->i64 = v; }
                break;
            }
            case 3: {
                short v = 0;
                buf = std::from_chars(buf, buf + rep.width, v).ptr;
                if (p != nullptr) { *p->i16 = v; }
                break;
            }
            case 4: {
                signed char v = 0;
                buf = std::from_chars(buf, buf + rep.width, v).ptr;
                if (p != nullptr) { *p->i8 = v; }
                break;
            }
            }
            break;
        }
        case unsigned_int: {
            switch ((rep.state >> 5) & 3) {
            case 0: {
                unsigned int v = 0;
                buf = std::from_chars(buf, buf + rep.width, v).ptr;
                if (p != nullptr) { *p->u32 = v; }
                break;
            }
            case 1: {
                unsigned long v = 0;
                buf = std::from_chars(buf, buf + rep.width, v).ptr;
                if (p != nullptr) { *p->uli = v; }
                break;
            }
            case 2: {
                unsigned long long v = 0;
                buf = std::from_chars(buf, buf + rep.width, v).ptr;
                if (p != nullptr) { *p->u64 = v; }
                break;
            }
            case 3: {
                unsigned short v = 0;
                buf = std::from_chars(buf, buf + rep.width, v).ptr;
                if (p != nullptr) { *p->u16 = v; }
                break;
            }
            case 4: {
                unsigned char v = 0;
                buf = std::from_chars(buf, buf + rep.width, v).ptr;
                if (p != nullptr) { *p->u8 = v; }
                break;
            }
            }
            break;
        }
        case big_norm_float:
        case little_norm_float:
            switch ((rep.state >> 5) & 3) {
            case 0: {
                float v = 0;
                buf = std::from_chars(buf, buf + rep.width, v, std::chars_format::fixed).ptr;
                if (p != nullptr) { *p->f32 = v; }
                break;
            }
            case 1: {
                double v = 0;
                buf = std::from_chars(buf, buf + rep.width, v, std::chars_format::fixed).ptr;
                if (p != nullptr) { *p->f64 = v; }
                break;
            }
            case 2: {
                long double v = 0;
                buf = std::from_chars(buf, buf + rep.width, v).ptr;
                if (p != nullptr) { *p->llf = v; }
                break;
            }
            }
            break;
        case big_exp_float:
        case little_exp_float:
            switch ((rep.state >> 5) & 3) {
            case 0: {
                float v = 0;
                buf = std::from_chars(buf, buf + rep.width, v, std::chars_format::scientific).ptr;
                if (p != nullptr) { *p->f32 = v; }
                break;
            }
            case 1: {
                double v = 0;
                buf = std::from_chars(buf, buf + rep.width, v, std::chars_format::scientific).ptr;
                if (p != nullptr) { *p->f64 = v; }
                break;
            }
            case 2: {
                long double v = 0;
                buf = std::from_chars(buf, buf + rep.width, v, std::chars_format::scientific).ptr;
                if (p != nullptr) { *p->llf = v; }
                break;
            }
            }
            break;
        case character:
            if (p != nullptr) {
                buf = std::copy_n(buf, rep.width, p->ch);
                break;
            }
            buf += rep.width;
            break;
        case string: {
            char* q = p == nullptr ? nullptr : p->str;
            if (rep.width == 0) {
                while (!is_empty_char(*buf)) {
                    if (p != nullptr) {
                        *q++ = *buf;
                    }
                    ++buf;
                }
            }
            else {
                for (int i = 0; i < rep.width && !is_empty_char(*buf); ++i) {
                    if (q != nullptr) {
                        *q++ = *buf;
                    }
                    ++buf;
                }
            }
            break;

        }
        case big_hex:
        case little_hex: {
            unsigned int v = 0;
            buf += 2; // Cross 0x or 0X.
            buf = std::from_chars(buf, buf + rep.width, v, 16).ptr;
            if (p != nullptr) { *p->u32 = v; }
            break;
        }
        case octal_int: {
            unsigned int v = 0;
            buf = std::from_chars(buf, buf + rep.width, v, 8).ptr;
            if (p != nullptr) { *p->u32 = v; }
            break;
        }
        case pointer: {
            unsigned long long v = 0;
            buf = std::from_chars(buf, buf + rep.width, v, 16).ptr;
            if (p != nullptr) { *p->u64 = v; }
            break;
        }
        case boolean: {
            bool v = false;
            v = *buf == 't' ? true : false;
            buf = *buf == 't' ? buf + 4 : buf + 5;
            if (p != nullptr) { *p->bln = v; }
        }
                    break;
        case range: {
            char* q = p->str;
            if (rep.width == 0 && (rep.state & 0x10)) {
                for (; !char_set<char>().contains(*buf); ++buf) {
                    *q++ = *buf;
                }
                break;
            }
            if (rep.width == 0 && !(rep.state & 0x10)) {
                for (; char_set<char>().contains(*buf); ++buf) {
                    *q++ = *buf;
                }
                break;
            }
            if (rep.width != 0 && (rep.state & 0x10)) {
                const char* i = buf;
                for (; (!char_set<char>().contains(*buf)) && (i - buf < rep.width); ++i) {
                    *q++ = *i;
                }
                buf = i;
                break;
            }
            if (rep.width != 0 && !(rep.state & 0x10)) {
                const char* i = buf;
                for (; (char_set<char>().contains(*buf)) && (i - buf < rep.width); ++i) {
                    *q++ = *i;
                }
                buf = i;
                break;
            }
        }
        }
        return buf;
    }
    int vformat_from(const char* buf, const char* fmt, va_list args) {
        const char* bend = buf + strlen(buf), * fend = fmt + strlen(fmt);
        const char* bbeg = buf, * fbeg = fmt;

        while (bbeg != bend && fbeg != fend) {
            const bool bemp = is_empty_char(*bbeg);
            const bool femp = is_empty_char(*fbeg);

            if (bemp) {
                bbeg++;
            }
            if (femp) {
                fbeg++;
            }
            if (!(bemp || femp)) {
                if (*fbeg == '{') {
                    input_replacement rep;
                    fbeg = parse_replacement(rep, fbeg + 1, fend);
                    if (rep.state & 1) {
                        if (*bbeg != '{') {
                            return 0; // Pattern not match.
                        }
                    }
                    else {
                        if (!((rep.state >> 1) & 1)) {
                            input_arg argptr;
                            // switch rep.type and use va_arg convert.
                            switch (rep.type) {
                            case signed_int:
                            case big_hex:
                            case little_hex:
                            case octal_int:
                                switch ((rep.state >> 5) & 3) {
                                case 0: argptr.i32 = va_arg(args, int*);         break;
                                case 1: argptr.li = va_arg(args, long*);        break;
                                case 2: argptr.i64 = va_arg(args, long long*);   break;
                                case 3: argptr.i16 = va_arg(args, short*);       break;
                                case 4: argptr.i8 = va_arg(args, signed char*); break;
                                }
                                break;
                            case unsigned_int:
                                switch ((rep.state >> 5) & 3) {
                                case 0: argptr.u32 = va_arg(args, unsigned int*);         break;
                                case 1: argptr.uli = va_arg(args, unsigned long*);        break;
                                case 2: argptr.u64 = va_arg(args, unsigned long long*);   break;
                                case 3: argptr.u16 = va_arg(args, unsigned short*);       break;
                                case 4: argptr.u8 = va_arg(args, unsigned char*);        break;
                                }
                                break;
                            case big_norm_float:
                            case big_exp_float:
                            case little_norm_float:
                            case little_exp_float:
                                switch ((rep.state >> 5) & 3) {
                                case 0: argptr.f32 = va_arg(args, float*);  break;
                                case 1: argptr.f64 = va_arg(args, double*); break;
                                case 2: argptr.llf = va_arg(args, long double*); break;
                                }
                                break;
                            case character:
                                argptr.ch = va_arg(args, char*);
                                break;
                            case string:
                                argptr.str = va_arg(args, char*);
                                break;
                            case pointer:
                                argptr.ptr = va_arg(args, void**);
                                break;
                            case boolean:
                                argptr.bln = va_arg(args, bool*);
                                break;
                            case range:
                                argptr.str = va_arg(args, char*);
                                break;
                            }
                            bbeg = read_replacement(rep, bbeg, &argptr);
                        }
                        else {
                            bbeg = read_replacement(rep, bbeg, nullptr);
                        }
                    }
                }
                else {
                    if (*fbeg != *bbeg) {
                        return 0; // Pattern not matched.
                    }
                    fbeg++; bbeg++;
                }
            }
        }
        return static_cast<int>(bbeg - buf);
    }
    int format_from(const char* buf, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        const int k = vformat_from(buf, fmt, args);
        va_end(args);
        return k;
    }
}
