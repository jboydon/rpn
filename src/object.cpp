#include <string>
#include <math.h>
using namespace std;

#include "mpfr.h"
#include "constant.h"
#include "object.hpp"

// floating_t statics
mpfr_prec_t floating_t::s_mpfr_prec = MPFR_DEFAULT_PREC_BITS;
mpfr_rnd_t floating_t::s_mpfr_rnd = MPFR_DEFAULT_RND;
unsigned int floating_t::s_mpfr_prec_bytes = MPFR_DEFAULT_STORING_LENGTH_BYTES;
const char* floating_t::s_mpfr_rnd_str[5] = MPFR_RND_STRINGS;

// number statics
number::mode_enum number::s_mode = DEFAULT_MODE;
int number::s_decimal_digits = DEFAULT_DECIMAL_DIGITS;
string number::s_mpfr_printf_format = string(MPFR_DEFAULT_FORMAT);

//
const char* object::s_cmd_type_string[cmd_max] = CMD_TYPE_STRINGS;

static bool is_min(mpfr_t p, mpfr_prec_t prec)
{
    // see mpfr_vasprintf code
    bool ret;
    int round_away;
    switch (floating_t::s_mpfr_rnd)
    {
        case MPFR_RNDA:
            round_away = 1;
            break;
        case MPFR_RNDD:
            round_away = MPFR_IS_NEG (p);
            break;
        case MPFR_RNDU:
            round_away = MPFR_IS_POS (p);
            break;
        case MPFR_RNDN:
        {
            /* compare |p| to y = 0.5*10^(-prec) */
            mpfr_t y;
            mpfr_exp_t e = MAX (MPFR_PREC (p), 56);
            mpfr_init2 (y, e + 8);
            do
            {
                /* find a lower approximation of
                 0.5*10^(-prec) different from |p| */
                e += 8;
                mpfr_set_prec (y, e);
                mpfr_set_si (y, -prec, MPFR_RNDN);
                mpfr_exp10 (y, y, MPFR_RNDD);
                mpfr_div_2ui (y, y, 1, MPFR_RNDN);
            } while (mpfr_cmpabs (y, p) == 0);
            
            round_away = mpfr_cmpabs (y, p) < 0;
            mpfr_clear (y);
        }
            break;
        default:
            round_away = 0;
    }
    
    if (round_away)
    /* round away from zero: the last output digit is '1' */
        ret = true;
    else
    /* only zeros in fractional part */
        ret = false;
    return ret;
}

static void print_fix(FILE* stream, mpfr_t real, int base)
{
    // see mpfr_vasprintf code
    mpfr_exp_t exp = mpfr_get_exp(real);
    int digits = 0; //forced 0 digits after separator
    int i;
    
    if (MPFR_UNLIKELY(MPFR_IS_SINGULAR(real)))
    {
        if (MPFR_IS_NAN(real))
            fputs("nan", stream);
        else if (MPFR_IS_INF(real))
        {
            if (MPFR_IS_NEG(real))
                fputc('-', stream);
            fputs("inf", stream);
        }
        else
        {
            // zero
            if (MPFR_IS_NEG(real))
                fputc('-', stream);//signed zero is allowed
            fputc('0', stream);
            if (digits > 0)
            {
                fputc('.', stream);
                for(i = 0; i < digits; i++)
                    fputc('0', stream);
            }
        }
    }
    else if (exp < -digits)
    {
        if (MPFR_IS_NEG(real))
            fputc('-', stream);
        fputc('0', stream);
        if (digits > 0)
        {
            fputc('.', stream);
            for (i = 0; i< digits - 1; i++)
                fputc('0', stream);
            
            if (is_min(real, digits))
                fputc('1', stream);
            else
                fputc('0', stream);
        }
    }
    else
    {
        char* str = mpfr_get_str(NULL, &exp, base, digits + exp + 1, real, floating_t::s_mpfr_rnd);
        if(str != NULL)
        {
            size_t len = strlen(str);
            if (len > 0)
            {
                if (str[0] == '-')
                {
                    fputc(str[0], stream);
                    len--;
                    str++;
                }
                else if (str[0] == '+')
                {
                    len--;
                    str++;
                }
                if (exp < 0)
                {
                    fputc('0', stream);
                    if (digits > 0)
                    {
                        fputc('.', stream);
                        for (i = 0; i < -(int)exp; i++)
                            fputc('0', stream);
                        fputs(str, stream);
                        for (i = 0; i < (int)(digits - len + exp); i++)
                            fputc('0', stream);
                    }
                }
                else
                {
                    if (exp == 0)
                        fputc('0', stream);
                    else
                        for (i = 0; i < (int)exp; i++)
                            fputc(str[i], stream);
                    if (digits > 0)
                    {
                        fputc('.', stream);
                        
                        int remaining = (int)MIN(strlen(str) - exp - 1, digits) + 1;
                        for (i = (int)exp; i < remaining + (int)exp; i++)
                            fputc(str[i], stream);
                        for (i = 0; i < (int)(exp + digits - len); i++)
                            fputc('0', stream);
                    }
                }
            }
            mpfr_free_str(str);
        }
    }
}

void object::show(FILE* stream)
{
    //int digits;
    //char* str;
    
    switch(_type)
    {
        case cmd_number:
            switch(((number*)this)->_representation)
            {
                case number::dec:
                    mpfr_fprintf(stream, number::s_mpfr_printf_format.c_str(), ((number*)this)->_value.mpfr);
                    break;
                case number::hex:
                    fprintf(stream, "0x");
                    print_fix(stream, ((number*)this)->_value.mpfr, 16);
                    break;
                case number::bin:
                    fprintf(stream, "0b");
                    print_fix(stream, ((number*)this)->_value.mpfr, 2);
                    break;
                case number::base:
                    fprintf(stream, "%db", ((number*)this)->_base);
                    print_fix(stream, ((number*)this)->_value.mpfr, ((number*)this)->_base);
                    break;
                default:
                    fprintf(stream, "<unknown number representation>");
                    break;
            }
            break;
        case cmd_complex:
            switch(((complex*)this)->_representation)
            {
                case number::dec:
                    fprintf(stream, "(");
                    mpfr_fprintf(stream, number::s_mpfr_printf_format.c_str(), ((complex*)this)->re()->mpfr);
                    fprintf(stream, ",");
                    mpfr_fprintf(stream, number::s_mpfr_printf_format.c_str(), ((complex*)this)->im()->mpfr);
                    fprintf(stream, ")");
                    break;
                case number::hex:
                    fprintf(stream, "(");
                    mpfr_fprintf(stream, string(MPFR_FORMAT_HEX).c_str(), ((complex*)this)->re()->mpfr);
                    fprintf(stream, ",");
                    mpfr_fprintf(stream, string(MPFR_FORMAT_HEX).c_str(), ((complex*)this)->im()->mpfr);
                    fprintf(stream, ")");
                    break;
                default:
                    fprintf(stream, "<unknown complex representation>");
            }
            break;
        case cmd_string:
            fprintf(stream, "\"%s\"", ((ostring*)this)->_value);
            break;
        case cmd_program:
            fprintf(stream, "<<%s>>", ((oprogram*)this)->_value);
            break;
        case cmd_symbol:
            fprintf(stream, "'%s'", ((symbol*)this)->_value);
            break;
        case cmd_keyword:
        case cmd_branch:
            fprintf(stream, "%s", ((keyword*)this)->_value);
            break;
        default:
            fprintf(stream, "< unknown object representation >");
            break;
    }
}
