//
void rpn_nop()
{
    // nop
}

void rpn_good_bye()
{
    ERR_CONTEXT(ret_good_bye);
}

void rpn_help()
{
    // software name
    printf("\n" ATTR_BOLD "%s" ATTR_OFF "\n", uname);

    // description
    printf("%s\n\n", description);

    // syntax
    printf("%s\n", syntax);

    // keywords
    unsigned int i = 0;
    while(s_keywords[i].type != cmd_max)
    {
        if (s_keywords[i].comment.size() != 0)
        {
            // titles in bold
            if (s_keywords[i].type==cmd_undef)
                printf(ATTR_BOLD);
            // show title or keyword + comment
            printf("%s\t%s\n", s_keywords[i].name, s_keywords[i].comment.c_str());
            if (s_keywords[i].type==cmd_undef)
                printf(ATTR_OFF);
        }
        i++;
    }
    printf("\n");

    // show mode
    printf("Current float mode is ");
    switch(number::s_mode)
    {
        case number::std: printf("'std'"); break;
        case number::fix: printf("'fix'"); break;
        case number::sci: printf("'sci'"); break;
        default: printf("unknown"); break;
    }
    printf(" with %d digits after the decimal point\n", number::s_decimal_digits);

    // bits precision, decimal digits and rounding mode
    printf("Current floating point precision is %d bits\n", (int)floating_t::s_mpfr_prec);
    printf("Current rounding mode is \"%s\"\n", floating_t::s_mpfr_rnd_str[floating_t::s_mpfr_rnd]);
    printf("\n\n");
}

int base_digits_from_bit_precision(int base, int bit_precision)
{
    return (int)ceil(bit_precision * log(2.0) / log((double)base)) - 1;
}

string make_digit_format(int decimal_digits, const char* printf_format)
{
    stringstream ss;
    ss << MPFR_FORMAT_BEG;
    ss << number::s_decimal_digits;
    ss << printf_format;
    return ss.str();
}

bool check_decimal_digits(double precision)
{
    bool ret = true;

    // MPFR_PREC_MAX mpfr_prec_t depends on _MPFR_PREC_FORMAT macro (see mpfr.h)
    // this could not exceed 63 bits max (0x7FFFFFFFFFFFFFFF)
    double prec_max = (double)MPFR_PREC_MAX;
    double prec_min = 0.0;
    
    if (precision < prec_min || precision > prec_max)
        ret = false;
    
    return ret;
}

void rpn_std()
{
    MIN_ARGUMENTS(1);
    ARG_MUST_BE_OF_TYPE(0, cmd_number);

    double digits = double(((number*)_stack->pop_back())->_value);
    
    if (check_decimal_digits(digits))
    {
        // set mode, decimal digits and print format
        number::s_mode = number::std;
        number::s_decimal_digits = (int)digits;
        number::s_mpfr_printf_format = make_digit_format(number::s_decimal_digits, MPFR_FORMAT_STD);
    }
    else
        ERR_CONTEXT(ret_out_of_range);
}

void rpn_fix()
{
    MIN_ARGUMENTS(1);
    ARG_MUST_BE_OF_TYPE(0, cmd_number);

    double digits = double(((number*)_stack->pop_back())->_value);
    
    if (check_decimal_digits(digits))
    {
        // set mode, decimal digits and print format
        number::s_mode = number::fix;
        number::s_decimal_digits = (int)digits;
        number::s_mpfr_printf_format = make_digit_format(number::s_decimal_digits, MPFR_FORMAT_FIX);
    }
    else
        ERR_CONTEXT(ret_out_of_range);
}

void rpn_sci()
{
    MIN_ARGUMENTS(1);
    ARG_MUST_BE_OF_TYPE(0, cmd_number);

    double digits = double(((number*)_stack->pop_back())->_value);
    
    if (check_decimal_digits(digits))
    {
        // set mode, decimal digits and print format
        number::s_mode = number::sci;
        number::s_decimal_digits = (int)digits;
        number::s_mpfr_printf_format = make_digit_format(number::s_decimal_digits, MPFR_FORMAT_SCI);
    }
    else
        ERR_CONTEXT(ret_out_of_range);
}

void rpn_version()
{
    // allocate and set object
    unsigned int naked_entry_len = strlen(version);
    ostring* str = (ostring*)_stack->allocate_back(sizeof(ostring)+naked_entry_len+1, cmd_string);
    str->set(version, naked_entry_len);
}

void rpn_uname()
{
    // allocate and set object
    unsigned int naked_entry_len = strlen(uname);
    ostring* str = (ostring*)_stack->allocate_back(sizeof(ostring)+naked_entry_len+1, cmd_string);
    str->set(uname, naked_entry_len);
}

void rpn_history()
{
    //see command history on stdout
    int index = 0;
    char* line = linenoiseHistoryLine(index);
    while(line != NULL)
    {
        cout<<line<<endl;
        free(line);
        line = linenoiseHistoryLine(++index);
    }
}

void rpn_type()
{
    MIN_ARGUMENTS(1);

    int type = _stack->pop_back()->_type;
    if (type < 0 || type >= (int)cmd_max)
        type = (int)cmd_undef;

    unsigned int string_size = (unsigned int)strlen(object::s_cmd_type_string[type]);
    unsigned int size = sizeof(symbol)+string_size+1;
    ostring* typ = (ostring*)_stack->allocate_back(size, cmd_string);
    typ->set(object::s_cmd_type_string[type], string_size);
}

void rpn_default()
{
    program::apply_default();
}

void rpn_precision()
{
    MIN_ARGUMENTS(1);
    ARG_MUST_BE_OF_TYPE(0, cmd_number);

    //set precision
    unsigned long prec = mpfr_get_ui(((number*)_stack->pop_back())->_value.mpfr, floating_t::s_mpfr_rnd);
    if (prec>=(unsigned long)MPFR_PREC_MIN && prec<=(unsigned long)MPFR_PREC_MAX)
    {
        floating_t::s_mpfr_prec = (mpfr_prec_t)prec;
        floating_t::s_mpfr_prec_bytes = (unsigned int)mpfr_custom_get_size(prec);
        
        // modify digits seen by user if std mode
        if (number::s_mode == number::std)
        {
            // calc max nb of digits user can see with the current bit precision
            number::s_decimal_digits = base_digits_from_bit_precision(10, (int)floating_t::s_mpfr_prec);
            number::s_mpfr_printf_format = make_digit_format(number::s_decimal_digits, MPFR_FORMAT_STD);
        }
    }
    else
        ERR_CONTEXT(ret_out_of_range);
}

void rpn_round()
{
    MIN_ARGUMENTS(1);
    ARG_MUST_BE_OF_TYPE(0, cmd_string);
    
    ostring* str = (ostring*)_stack->pop_back();
    bool done = false;
    for(int rnd = (int)MPFR_DEFAULT_RND; rnd <= (int)MPFR_RNDA; rnd++)
    {
        if (string(floating_t::s_mpfr_rnd_str[rnd]) == str->_value)
        {
            floating_t::s_mpfr_rnd = (mpfr_rnd_t)rnd;
            done = true;
        }
    }
    if (!done)
        ERR_CONTEXT(ret_out_of_range);
}
