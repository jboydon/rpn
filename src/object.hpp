#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <mpfr.h>
#include <string.h>

// definitions for objects
////
typedef enum {
    cmd_undef,
    cmd_number,// floating point number
    cmd_complex,// complex, couple of floating point numbers
    cmd_string,// string like "string"
    cmd_symbol,// symbol like 'symbol'
    cmd_program,// program like << instructions >>
    cmd_keyword,// langage keyword
    cmd_branch,// langage branch keyword
    cmd_max
} cmd_type_t;

class program;
class branch;

typedef void (program::*program_fn_t)(void);
typedef int (program::*branch_fn_t)(branch&);

// MPFR object
////
struct floating_t
{
    mpfr_prec_t mpfr_prec;// precision in bits
    unsigned int mpfr_prec_bytes;// significand storing length in bytes
    mpfr_t mpfr;//mpfr object

    void init()
    {
        void* significand = (void*)(this+1);
        mpfr_prec = s_mpfr_prec;
        mpfr_prec_bytes = s_mpfr_prec_bytes;
        mpfr_custom_init(significand, MPFR_DEFAULT_PREC_BITS);
        mpfr_custom_init_set(mpfr, MPFR_ZERO_KIND, 0, mpfr_prec, significand);
    }

    void move()
    {
        void* significand = (void*)(this+1);
        mpfr_custom_move(mpfr, significand);
    }

    void operator=(const double val)
    {
        mpfr_set_d(mpfr, val, s_mpfr_rnd);
    }

    void operator=(const long int val)
    {
        mpfr_set_si(mpfr, val, s_mpfr_rnd);
    }

    void operator=(const unsigned long val)
    {
        mpfr_set_ui(mpfr, val, s_mpfr_rnd);
    }

    operator double()
    {
        return mpfr_get_d(mpfr, s_mpfr_rnd);
    }

    operator int()
    {
        return (int)mpfr_get_si(mpfr, s_mpfr_rnd);
    }
    
    operator long()
    {
        return mpfr_get_si(mpfr, s_mpfr_rnd);
    }

    bool operator>(const floating_t right)
    {
        return mpfr_cmp(mpfr, right.mpfr) > 0;
    }

    bool operator<(const floating_t right)
    {
        return mpfr_cmp(mpfr, right.mpfr) < 0;
    }

    // default precision in bits, precision length in bytes, rounding mode
    static mpfr_prec_t s_mpfr_prec;
    static unsigned int s_mpfr_prec_bytes;
    static mpfr_rnd_t s_mpfr_rnd;
    static const char* s_mpfr_rnd_str[5];
};

// object - a generic stack object
////
struct object
{
    // object type
    cmd_type_t _type;
    unsigned int _size;

    //
    unsigned int size() { return _size; }

    void show(FILE* stream = stdout);

    //
    static const char* s_cmd_type_string[cmd_max];
};

// stack objects derived from object
////
struct number : public object
{
    // members
    enum {
        dec,
        hex,
        bin,
        base
    } _representation;
    // base
    // carefull: _base is used only if _representation = base
    int _base;
    // mind that float value is at the end of the object
    // because its mantissa is just after the obj in memory
    floating_t _value;

    // publics
    number() { _type = cmd_number; }

    void init()
    {
        _type = cmd_number;
        _representation = dec;
        _value.init();
    }

    void move()
    {
        _value.move();
    }

    void set(unsigned long value)
    {
        _type = cmd_number;
        _value = value;
    }

    static unsigned int calc_size()
    {
        return (unsigned int)(sizeof(number)+floating_t::s_mpfr_prec_bytes);
    }

    // representation mode
    typedef enum {
        std,
        fix,
        sci
    } mode_enum;
    static mode_enum s_mode;

    // precision
    static int s_decimal_digits;
    static string s_mpfr_printf_format;
};

// stack objects derived from object
////
struct complex : public object
{
    enum {
        dec,
        hex
    } _representation;
    // mind that re float value is at the end of the object
    // because its mantissa is just after the obj in memory
    floating_t _re;

    complex() { _type = cmd_complex; }

    // re and im float values are at the end of the object
    floating_t* re() { return &_re; }
    floating_t* im() { return (floating_t*)((char*)&_re + sizeof(floating_t) + _re.mpfr_prec_bytes); }

    void init()
    {
        _type = cmd_complex;
        _representation = dec;
        re()->init();
        im()->init();
    }

    void move()
    {
        re()->move();
        im()->move();
    }

    static unsigned int calc_size()
    {
        return (unsigned int)(sizeof(complex)+2*(sizeof(floating_t)+floating_t::s_mpfr_prec_bytes));
    }
};

struct ostring : public object
{
    // ostring may first have been allocated with len+1 bytes
    void set(const char* value, unsigned int len)
    {
        _type = cmd_string;
        if (value != NULL)
        {
            if (len>0)
                (void)memcpy(_value, value, len);
            _value[len] = 0;
            _len = len;
        }
        else
        {
            _value[_len] = 0;
            _len = 0;
        }
    }

    // length of _value, not including the terminal '\0'
    unsigned int _len;
    char _value[0];
};

struct oprogram : public object
{
    // oprogram may first have been allocated with len+1 bytes
    void set(const char* value, unsigned int len)
    {
        _type = cmd_program;
        if (value != NULL)
        {
            if (len>0)
                (void)memcpy(_value, value, len);
            _value[len] = 0;
            _len = len;
        }
        else
        {
            _value[0] = 0;
            _len = 0;
        }
    }

    // length of _value, not includiong the terminal '\0'
    unsigned int _len;
    char _value[0];
};

struct symbol : public object
{
    // symbol may first have been allocated with len+1 bytes
    void set(const char* value, unsigned int len, bool auto_eval)
    {
        _type = cmd_symbol;
        _auto_eval = auto_eval;

        if (value != NULL)
        {
            if (len>0)
                (void)memcpy(_value, value, len);
            _value[len] = 0;
            _len = len;
        }
        else
        {
            _value[0] = 0;
            _len = 0;
        }
    }

    //
    bool _auto_eval;
    // length of _value, not includiong the terminal '\0'
    unsigned int _len;
    char _value[0];
};

struct keyword : public object
{
    // keyword may first have been allocated with len+1 bytes
    void set(program_fn_t fn, const char* value, unsigned int len)
    {
        _type = cmd_keyword;
        _fn = fn;
        if (value != NULL)
        {
            if (len>0)
                (void)memcpy(_value, value, len);
            _value[len] = 0;
            _len = len;
        }
        else
        {
            _value[0] = 0;
            _len = 0;
        }
    }

    //
    program_fn_t _fn;
    // length of _value, not includiong the terminal '\0'
    unsigned int _len;
    char _value[0];
};

struct branch : public object
{
    //
    void set(branch_fn_t fn, const char* value, unsigned int len)
    {
        _type = cmd_branch;
        _fn = fn;
        arg1 = -1;
        arg2 = -1;
        arg3 = -1;
        farg1 = NULL;
        farg2 = NULL;
        arg_bool = 0;
        if (value != NULL)
        {
            if (len>0)
                (void)memcpy(_value, value, len);
            _value[len] = 0;
            _len = len;
        }
        else
        {
            _value[0] = 0;
            _len = 0;
        }
    }

    // branch function
    branch_fn_t _fn;
    // args used by cmd_branch cmds
    int arg1, arg2, arg3;
    number *farg1, *farg2;
    bool arg_bool;
    
    // length of _value, not includiong the terminal '\0'
    unsigned int _len;
    char _value[0];
};

#endif
