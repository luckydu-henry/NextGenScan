#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <charconv>

const size_t int_default_width = 10;
const size_t float_default_width = 8; // with dot

union scan_arg {
    int* i32;
    unsigned int* u32;
    float* f32;
    char* ch8;
    char* str; // This one should allocate first before passing in as arg.
    void** ptr;
    bool* bln;
    // These types should combine with state.
    long long* i64;
    unsigned long long* u64;
    double* f64;
    short* i16;
    unsigned short* u16;
    signed char* i8;
    unsigned char* u8;
};

enum format_type : char {
    signed_int, unsigned_int, big_norm_float, big_exp_float,
    little_norm_float, little_exp_float, character, string,
    big_hex, little_hex, octal_int, pointer, boolean, range
};
// boolean read as 'true' or 'false'.

struct replacement {
    // Layout of the state bit mask(endian is little).
    // ------------------------------------------------------
    // | bit position |   usage   |         explain         |
    // ------------------------------------------------------
    // |      0       |  escaped  | First char is %         |
    // ------------------------------------------------------
    // |      1       |  ignored  | First char is *         |
    // ------------------------------------------------------
    // |      2       |   sign    | Has +- notation         |
    // ------------------------------------------------------
    // |      3       | precision | Floating point has .    |
    // ------------------------------------------------------
    // |      4       | included  | Range has ^             |
    // ------------------------------------------------------
    // |     5~6      |   length  | l/ll/h/hh -> 0,1,2,3    |
    // ------------------------------------------------------
    uint8_t       state;
    size_t        width;
    format_type   type;
    char          chset[1 << 7];     // contents inside [] except from ^ can enum at most 128 ascii characters or use - to represent a range.
};

// Beg starts from the % and reach to \0 or next %
const char* dd_parse_replacement(replacement* rep, const char* beg, const char* end) {
    // Initial settings.
    rep->state = 0;
    rep->width = 0;
    // Escape character
    if (*beg == '%') {
        rep->state |= 0x01;
        return end;
    }
    char* j = rep->chset;
    for (; beg != end; ++beg) {
        switch (*beg) {
        case '*':
            rep->state |= 0x02; break;
        case '+':
            rep->state |= 0x04; break;
            // Check width
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            rep->width = rep->width * 10 + *beg - '0'; break;
        case '.':
            rep->state |= 0x08;                        break;
        case 'l':
            if (*(beg + 1) == *beg) {
                rep->state |= 0x20;
            }
            else { rep->state |= 0; } break;
        case 'h':
            if (*(beg + 1) == *beg) {
                rep->state |= 0x60;
            }
            else { rep->state |= 0x40; } break;
        case 'd': case 'i':
            rep->type = format_type::signed_int;
            if (rep->width == 0) { rep->width = int_default_width; }
            break;
        case 'u':
            rep->type = format_type::unsigned_int;
            if (rep->width == 0) { rep->width = int_default_width; }
            break;
        case 'g': case 'f':
            rep->type = format_type::little_norm_float;
            rep->width = float_default_width;
            break;
        case 'G': case 'F':
            rep->type = format_type::big_norm_float;
            rep->width = float_default_width;
            break;
            // TODO: Implement exponent form floating point!
        case 'e':
            rep->type = format_type::little_exp_float;     break;
        case 'E':
            rep->type = format_type::big_exp_float;        break;
        case 'c':
            rep->type = format_type::character;
            if (rep->width == 0) { rep->width = 1; }
            break;
        case 's':
            rep->type = format_type::string;               break;
        case 'x':
            rep->type = format_type::little_hex;
            if (rep->width == 0) { rep->width = 8; }
            break;
        case 'X':
            rep->type = format_type::big_hex;
            if (rep->width == 0) { rep->width = 8; }
            break;
        case 'o':
            rep->type = format_type::octal_int;
            if (rep->width == 0) { rep->width = 11; }
            break;
        case 'p':
            rep->type = format_type::pointer;
            rep->width = 16; // For 64-bit system it's just a 64bit integer.
            break;
        case 'b':
            rep->type = format_type::boolean;              break;
        case '[':
            rep->type = format_type::range;
            // Start from the next one of '['.
            ++beg;
            for (; *beg != ']'; ++beg) {
                switch (*beg) {
                case '^':
                    if (!(rep->state & 0x10)) {
                        rep->state |= 0x10;
                    }
                    else { *j++ = *beg; }
                    break;
                case '-':
                    if (*(beg + 1) == '-') {
                        *j++ = '-';
                    }
                    break;
                case 'z':
                case 'Z':
                    if (*(j - 1) == *beg - 25) {
                        --j;
                        for (char i = *beg - 25; i < *beg + 1; ++i) {
                            *j++ = i;
                        }
                    }
                    else { *j++ = *beg; }
                    break;
                case '9':
                    if (*(j - 1) == *beg - 9) {
                        --j;
                        for (char i = *beg - 9; i < *beg + 1; ++i) {
                            *j++ = i;
                        }
                    }
                    else { *j++ = *beg; }
                    break;
                default:
                    *j++ = *beg; break;
                }
            }
            break;
        default:
            return beg; // Invalid format.
        }
    }
    return end;
}
bool dd_is_empty(char v) {
    switch (v) {
    case '\n': case '\r': case '\t': case  ' ':
        return true;
    default:
        return false;
    }
}
// Buf should point to the start of replacement.
const char* dd_handle_replacement(replacement* rep, const char* buf, scan_arg* p) {
    switch (rep->type) {
    case signed_int: {
        switch ((rep->state >> 5) & 3) {
        case 0: {
            int v = 0;
            buf = std::from_chars(buf, buf + rep->width, v).ptr;
            if (p != nullptr) { *p->i32 = v; }
            break;
        }
        case 1: {
            long long v = 0;
            buf = std::from_chars(buf, buf + rep->width, v).ptr;
            if (p != nullptr) { *p->i64 = v; }
            break;
        }
        case 2: {
            short v = 0;
            buf = std::from_chars(buf, buf + rep->width, v).ptr;
            if (p != nullptr) { *p->i16 = v; }
            break;
        }
        case 3: {
            signed char v = 0;
            buf = std::from_chars(buf, buf + rep->width, v).ptr;
            if (p != nullptr) { *p->i8 = v; }
            break;
        }
        }
        break;
    }
    case unsigned_int: {
        switch ((rep->state >> 5) & 3) {
        case 0: {
            unsigned int v = 0;
            buf = std::from_chars(buf, buf + rep->width, v).ptr;
            if (p != nullptr) { *p->u32 = v; }
            break;
        }
        case 1: {
            unsigned long long v = 0;
            buf = std::from_chars(buf, buf + rep->width, v).ptr;
            if (p != nullptr) { *p->u64 = v; }
            break;
        }
        case 2: {
            unsigned short v = 0;
            buf = std::from_chars(buf, buf + rep->width, v).ptr;
            if (p != nullptr) { *p->u16 = v; }
            break;
        }
        case 3: {
            unsigned char v = 0;
            buf = std::from_chars(buf, buf + rep->width, v).ptr;
            if (p != nullptr) { *p->u8 = v; }
            break;
        }
        }
        break;
    }
    case big_norm_float:
    case little_norm_float:
        switch ((rep->state >> 5) & 3) {
        case 0: {
            float v = 0;
            buf = std::from_chars(buf, buf + rep->width, v).ptr;
            if (p != nullptr) { *p->f32 = v; }
            break;
        }
        case 1: {
            double v = 0;
            buf = std::from_chars(buf, buf + rep->width, v).ptr;
            if (p != nullptr) { *p->f64 = v; }
            break;
        }
        }
        break;
    case big_exp_float:
    case little_exp_float:
        switch ((rep->state >> 5) & 3) {
        case 0: {
            float v = 0;
            buf = std::from_chars(buf, buf + rep->width, v, std::chars_format::scientific).ptr;
            if (p != nullptr) { *p->f32 = v; }
            break;
        }
        case 1: {
            double v = 0;
            buf = std::from_chars(buf, buf + rep->width, v, std::chars_format::scientific).ptr;
            if (p != nullptr) { *p->f64 = v; }
            break;
        }
        }
        break;
    case character:
        if (p != nullptr) {
            buf = (const char*)memcpy(p->ch8, buf, rep->width);
            break;
        }
        buf += rep->width;
        break;
    case string: {
        if (rep->width == 0) {
            while (!dd_is_empty(*buf)) {
                if (p != nullptr) {
                    *p->str = *buf;
                    ++p->str;
                }
                ++buf;
            }
            break;
        }
        else {
            for (int i = 0; i < rep->width && !dd_is_empty(*buf); ++i) {
                if (p != nullptr) {
                    *p->str = *buf;
                    ++p->str;
                }
                ++buf;
            }
            break;
        }
    }
    case big_hex:
    case little_hex: {
        unsigned int v = 0;
        buf += 2; // Cross 0x or 0X.
        buf = std::from_chars(buf, buf + rep->width, v, 16).ptr;
        if (p != nullptr) { *p->u32 = v; }
        break;
    }
    case octal_int: {
        unsigned int v = 0;
        buf = std::from_chars(buf, buf + rep->width, v, 8).ptr;
        if (p != nullptr) { *p->u32 = v; }
        break;
    }
    case pointer: {
        unsigned long long v = 0;
        buf = std::from_chars(buf, buf + rep->width, v, 16).ptr;
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
    case range:
        // TODO: implement a range mode.
        break;
    }
    return buf;
}
const char* dd_get_replacement(const char* beg) {
    if (*(beg + 1) == '%') {
        return beg + 2;
    }
    bool left_showed = false, right_showed = false;
    for (const char* i = beg + 1; ; ++i) {
        switch (*i) {
        case '[':
            if (!left_showed && !right_showed) { left_showed = true; break; }
            if (left_showed && !right_showed) { break; }
            if (left_showed && right_showed) { return i; }
            break;
        case ']':
            if (!left_showed && !right_showed) { return i; }
            if (left_showed && !right_showed) { right_showed = true; break; }
            if (left_showed && right_showed) { return i; }
            break;
        case 'd': case 'i': case 'u': case 'f': case 'g':
        case 'e': case 'o': case 'x': case 'c': case 's':
        case 'b': case 'p': case 'F': case 'G': case 'E':
        case 'X': case '.': case '+': case '0': case '1':
        case '2': case '3': case '4': case '5': case '6':
        case '7': case '8': case '9': case '*':
            if (left_showed && right_showed) {
                return i;
            }
            break;
        default:
            if (left_showed && !right_showed) {
                break;
            }
            return i;
        }
    }
}
int dd_vsscanf(const char* buf, const char* fmt, va_list args) {
    const char* bend = buf + strlen(buf), * fend = fmt + strlen(fmt);
    const char* bbeg = buf, * fbeg = fmt;

    while (bbeg != bend && fbeg != fend) {
        const bool bemp = dd_is_empty(*bbeg);
        const bool femp = dd_is_empty(*fbeg);

        if (bemp) {
            bbeg++;
        }
        if (femp) {
            fbeg++;
        }
        if (!(bemp || femp)) {
            if (*fbeg == '%') {
                const char* end = dd_get_replacement(fbeg); ++fbeg;
                replacement rep;
                fbeg = dd_parse_replacement(&rep, fbeg, end);
                if (rep.state & 1) {
                    if (*bbeg != '%') {
                        return -1; // Pattern not match.
                    }
                }
                else {
                    if (!((rep.state >> 1) & 1)) {
                        scan_arg argptr;
                        // switch rep.type and use va_arg convert.
                        switch (rep.type) {
                        case signed_int:
                        case big_hex:
                        case little_hex:
                        case octal_int:
                            switch ((rep.state >> 5) & 3) {
                            case 0: argptr.i32 = va_arg(args, int*);         break;
                            case 1: argptr.i64 = va_arg(args, long long*);   break;
                            case 2: argptr.i16 = va_arg(args, short*);       break;
                            case 3: argptr.i8 = va_arg(args, signed char*); break;
                            }
                            break;
                        case unsigned_int:
                            switch ((rep.state >> 5) & 3) {
                            case 0: argptr.u32 = va_arg(args, unsigned int*);         break;
                            case 1: argptr.u64 = va_arg(args, unsigned long long*);   break;
                            case 2: argptr.u16 = va_arg(args, unsigned short*);       break;
                            case 3: argptr.u8 = va_arg(args, unsigned char*);        break;
                            }
                            break;
                        case big_norm_float:
                        case big_exp_float:
                        case little_norm_float:
                        case little_exp_float:
                            switch ((rep.state >> 5) & 3) {
                            case 0: argptr.f32 = va_arg(args, float*);  break;
                            case 1: argptr.f64 = va_arg(args, double*); break;
                            }
                            break;
                        case character:
                            argptr.ch8 = va_arg(args, char*);
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
                        bbeg = dd_handle_replacement(&rep, bbeg, &argptr);
                    }
                    else {
                        bbeg = dd_handle_replacement(&rep, bbeg, nullptr);
                    }
                }
            }
            else {
                if (*fbeg != *bbeg) {
                    return -1; // Pattern not matched.
                }
                fbeg++; bbeg++;
            }
        }
    }
}

int dd_sscanf(const char* buf, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    dd_vsscanf(buf, fmt, args);
    va_end(args);
    return 0;
}
