void rpn_instr()
{
    MIN_ARGUMENTS(1);

    // stringify only if not already a string
    if (_stack->get_type(0) != cmd_string)
    {
        // write the object in stack(0) in a string and remove this obj
        FILE* tmp = tmpfile();
        if (tmp != NULL)
        {
            ((object*)_stack->pop_back())->show(tmp);

            // reserve the correct size on stack
            unsigned int str_size = (unsigned int)ftell(tmp);
            ostring* str = (ostring*)_stack->allocate_back(str_size+1+sizeof(ostring), cmd_string);
            str->_len = str_size;

            // fill the obj
            rewind(tmp);
            if (fread(str->_value, str_size, 1, tmp) != 1)
                ERR_CONTEXT(ret_runtime_error);
            str->_value[str_size] = 0;
            fclose(tmp);
        }
        else
            ERR_CONTEXT(ret_runtime_error);
    }
}

void rpn_strout()
{
    MIN_ARGUMENTS(1);
    ARG_MUST_BE_OF_TYPE(0, cmd_string);

    string entry(((ostring*)_stack->pop_back())->_value);

    program prog;

    // make program from string in stack level 1
    if (program::parse(entry.c_str(), prog) == ret_ok)
        // run it
        prog.run(*_stack, *_heap);
}

void rpn_chr()
{
    MIN_ARGUMENTS(1);
    ARG_MUST_BE_OF_TYPE(0, cmd_number);

    // get arg as number % 256
    char the_chr = (char)mpfr_get_d(((number*)_stack->pop_back())->_value.mpfr, floating_t::s_mpfr_rnd);
    if (the_chr<32 || the_chr>126)
        the_chr = '.';

    // reserve the correct size on stack (1 char)
    unsigned int str_size = 1;
    ostring* str = (ostring*)_stack->allocate_back(str_size+1+sizeof(ostring), cmd_string);
    str->_len = str_size;
    str->_value[0] = the_chr;
    str->_value[1] = 0;
}

void rpn_num()
{
    MIN_ARGUMENTS(1);
    ARG_MUST_BE_OF_TYPE(0, cmd_string);

    double the_chr = (double)((ostring*)_stack->pop_back())->_value[0];
    number* numb = (number*)_stack->allocate_back(number::calc_size(), cmd_number);
    numb->_value = the_chr;
}

void rpn_strsize()
{
    MIN_ARGUMENTS(1);
    ARG_MUST_BE_OF_TYPE(0, cmd_string);

    double len = ((ostring*)_stack->pop_back())->_len;
    number* numb = (number*)_stack->allocate_back(number::calc_size(), cmd_number);
    numb->_value = len;
}

void rpn_strpos()
{
    MIN_ARGUMENTS(2);
    ARG_MUST_BE_OF_TYPE(0, cmd_string);
    ARG_MUST_BE_OF_TYPE(1, cmd_string);

    long pos = 0;
    char* src = ((ostring*)_stack->get_obj(1))->_value;
    char* found = strstr(src, ((ostring*)_stack->get_obj(0))->_value);
    if (found != NULL)
        pos = (long)(found - src)+1L;

    _stack->pop_back(2);

    number* numb = (number*)_stack->allocate_back(number::calc_size(), cmd_number);
    numb->_value = pos;
}

void rpn_strsub()
{
    MIN_ARGUMENTS(3);
    ARG_MUST_BE_OF_TYPE(0, cmd_number);
    ARG_MUST_BE_OF_TYPE(1, cmd_number);
    ARG_MUST_BE_OF_TYPE(2, cmd_string);
    
    long first = long(((number*)_stack->get_obj(1))->_value) - 1;
    long last = long(((number*)_stack->get_obj(0))->_value) - 1;
    long len = ((ostring*)_stack->get_obj(2))->_len;
    bool result_is_void = false;

    _stack->pop_back(2);

    if (first < 0)
        first = 0;
    if (last < 0)
        last = 0;
    if (first > len && last > len)
        result_is_void=true;
    else
    {
        if (first > len)
            first = len -1;
        if (last > len)
            last = len -1;
        if (first > last)
            result_is_void=true;
    }

    if (!result_is_void)
    {
        unsigned int str_size = (unsigned int)(last - first + 1);
        ostring* str = (ostring*)_calc_stack.allocate_back(str_size+1+sizeof(ostring), cmd_string);
        str->_len = str_size;
        
        memcpy(((ostring*)_calc_stack.back())->_value, ((ostring*)_stack->get_obj(0))->_value+first, str_size);
        ((ostring*)_calc_stack.back())->_value[str_size] = 0;
        
        _stack->pop_back();
        stack::copy_and_push_back(_calc_stack, _calc_stack.size()-1, *_stack);
        _calc_stack.pop_back();
    }
    else
    {
        _stack->pop_back();
        ostring* str = (ostring*)_stack->allocate_back(1+sizeof(ostring), cmd_string);
        str->_len = 0;
        str->_value[0] = 0;
    }
}
