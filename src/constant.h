#ifndef CONSTANT_H
#define CONSTANT_H

// default values
//

// default mode and number of printed digits
//
#define DEFAULT_MODE number::std
#define MPFR_DEFAULT_FORMAT "%.xxRg"

/* directly calculated from 128 bits precision
   ceil(128 * log10(2)) - 1 = 38 */
#define DEFAULT_DECIMAL_DIGITS 38

// MPFR related defaults
//

// rounding method
#define MPFR_DEFAULT_RND MPFR_RNDN

// 128 bits significand precision
#define MPFR_DEFAULT_PREC_BITS 128

// 128 bits significand storing length in bytes, result of mpfr_custom_get_size(128)
#define MPFR_DEFAULT_STORING_LENGTH_BYTES 16

// constants
//

// commands and entry related constants
#define MAX_COMMAND_LENGTH 24
#define AUTOCOMPLETE_KEY '\t'
#define SHOW_STACK_SEPARATOR "> "
#define PROMPT "rpn> "

// show formats
#define MPFR_FORMAT_BEG "%."
#define MPFR_FORMAT_STD "Rg"
#define MPFR_FORMAT_FIX "Rf"
#define MPFR_FORMAT_SCI "Re"
#define MPFR_FORMAT_HEX "%Ra"

#define MPFR_RND_STRINGS { "nearest", "toward zero", "toward +inf", "toward -inf", "away from zero" }

// return values, used by all classes
//
typedef enum {
    ret_ok,
    ret_unknown_err,
    ret_missing_operand,
    ret_bad_operand_type,
    ret_out_of_range,
    ret_unknown_variable,
    ret_internal,
    ret_deadly,
    ret_good_bye,
    ret_line_cancelled,
    ret_not_impl,
    ret_nop,
    ret_syntax,
    ret_div_by_zero,
    ret_runtime_error,
    ret_max
} ret_value;

#define RET_VALUE_STRINGS { \
        "ok", "unknown command", "missing operand", "bad operand type", "out of range", "unknown variable", "internal error, aborting", \
        "deadly", "goodbye", "line cancelled", "not implemented", "no operation", "syntax error", "division by zero", "runtime error" \
    }

// command types
//
#define CMD_TYPE_STRINGS { "undef", "number", "complex", "string", "symbol", "program", "keyword", "keyword" }

// history
#define HISTORY_FILE ".rpn_history"
#define HISTORY_FILE_MAX_LINES (100)

// base min and max
#define BASE_MIN 2
#define BASE_MAX 62

// some defs for mpfr
#if _MPFR_EXP_FORMAT == 1
#define MPFR_EXP_MAX (SHRT_MAX)
#define MPFR_EXP_MIN (SHRT_MIN)
#elif _MPFR_EXP_FORMAT == 2
#define MPFR_EXP_MAX (INT_MAX)
#define MPFR_EXP_MIN (INT_MIN)
#elif _MPFR_EXP_FORMAT == 3
#define MPFR_EXP_MAX (LONG_MAX)
#define MPFR_EXP_MIN (LONG_MIN)
#elif _MPFR_EXP_FORMAT == 4
#define MPFR_EXP_MAX (MPFR_INTMAX_MAX)
#define MPFR_EXP_MIN (MPFR_INTMAX_MIN)
#else
#error "Invalid MPFR Exp format"
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(h,i) ((h) > (i) ? (h) : (i))

#define MPFR_EXP_INF (MPFR_EXP_MIN+3)
#define MPFR_EXP_NAN (MPFR_EXP_MIN+2)
#define MPFR_EXP(x) ((x)->_mpfr_exp)
#define MPFR_IS_SINGULAR(x) (MPFR_EXP(x) <= MPFR_EXP_INF)
#define MPFR_UNLIKELY(x) (__builtin_expect(!!(x),0))
#define MPFR_IS_NAN(x) (MPFR_EXP(x) == MPFR_EXP_NAN)
#define MPFR_IS_INF(x) (MPFR_EXP(x) == MPFR_EXP_INF)
#define MPFR_IS_NEG(x) (MPFR_SIGN(x) < 0)
#define MPFR_IS_POS(x) (MPFR_SIGN(x) > 0)
#define MPFR_PREC(x) ((x)->_mpfr_prec)

#endif
