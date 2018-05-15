//
int rpn_if(branch& myobj)
{
    // myobj.arg1 = 'if' condition evaluation value
    MIN_ARGUMENTS_RET(1, -(int)ret_runtime_error);
    ARG_MUST_BE_OF_TYPE_RET(0, cmd_number, -(int)ret_runtime_error);

    if (mpfr_cmp_si(((number*)_stack->get_obj(0))->_value.mpfr, 0UL) != 0)
        myobj.arg1 = 1;
    else
        myobj.arg1 = 0;
    (void)_stack->pop_back();
    return -1;
}

int rpn_then(branch& myobj)
{
    // myobj.arg1 = index of then + 1
    // myobj.arg2 = index of else + 1 or end + 1
    // myobj.arg3 = index of if
    // if condition is true -> arg1 (= jump to then + 1)
    // else -> arg2 (= jump to else + 1 or end + 1)
    branch* if_cmd = (branch*)seq_obj(myobj.arg3);
    if (if_cmd->arg1 == 1)
        return myobj.arg1;
    else
        return myobj.arg2;
}

int rpn_else(branch& myobj)
{
    // myobj.arg1 = index of else + 1
    // myobj.arg2 = index of end + 1
    // myobj.arg3 = index of if
    // if condition was false -> arg1 (= jump to else + 1)
    // if condition was true -> arg2 (= jump to end + 1)
    branch* if_cmd = (branch*)seq_obj(myobj.arg3);
    if (if_cmd->arg1 == 1)
        return myobj.arg2;
    else
        return myobj.arg1;
}

int rpn_end(branch& myobj)
{
    int ret = -1;

    // arg1 = index of do+1 in case of do..unti..end
    if (myobj.arg1 != -1)
    {
        // in a template do..unti..end
        MIN_ARGUMENTS_RET(1, -(int)ret_runtime_error);
        ARG_MUST_BE_OF_TYPE_RET(0, cmd_number, -(int)ret_runtime_error);

        // check arg
        number* arg = (number*)_stack->pop_back();
        if (mpfr_cmp_si(arg->_value.mpfr, 0UL) == 0)
            ret = myobj.arg1;
    }
    // arg2 = index of while+1 in case of while..repeat..end
    else if (myobj.arg2 != -1)
        ret = myobj.arg2;

    return ret;
}

int rpn_do(branch& myobj)
{
    // nothing
    return -1;
}

int rpn_until(branch& myobj)
{
    // nothing
    return -1;
}

//
void rpn_ift(void)
{
    MIN_ARGUMENTS(2);
    ARG_MUST_BE_OF_TYPE(1, cmd_number);

    // check ift arg
    // arg is true if number != 0 or if is nan or +/-inf
    number* testee = ((number*)_stack->get_obj(1));
    
    if (!mpfr_zero_p(testee->_value.mpfr))
    {
        CHECK_MPFR(stack::copy_and_push_back(*_stack, _stack->size()-1, _calc_stack));
        (void)_stack->pop_back(2);

        CHECK_MPFR(stack::copy_and_push_back(_calc_stack, _calc_stack.size()-1, *_stack));
        (void)_calc_stack.pop_back();
    }
    else
        (void)_stack->pop_back(2);
}

void rpn_ifte(void)
{
    MIN_ARGUMENTS(3);
    ARG_MUST_BE_OF_TYPE(2, cmd_number);

    // check ifte arg
    // arg is true if number != 0 or if is nan or +/-inf
    number* testee = ((number*)_stack->get_obj(2));

    if (!mpfr_zero_p(testee->_value.mpfr))
        CHECK_MPFR(stack::copy_and_push_back(*_stack, _stack->size()-2, _calc_stack));
    else
        CHECK_MPFR(stack::copy_and_push_back(*_stack, _stack->size()-1, _calc_stack));

    (void)_stack->pop_back(3);

    CHECK_MPFR(stack::copy_and_push_back(_calc_stack, _calc_stack.size()-1, *_stack));
    (void)_calc_stack.pop_back();
}

//
int rpn_while(branch& myobj)
{
    // nothing
    return -1;
}

int rpn_repeat(branch& myobj)
{
    int ret = -1;

    MIN_ARGUMENTS_RET(1, -(int)ret_runtime_error);
    ARG_MUST_BE_OF_TYPE_RET(0, cmd_number, -(int)ret_runtime_error);

    // check arg
    // myobj.arg1 is end+1
    number* arg = (number*)_stack->pop_back();
    if (mpfr_cmp_si(arg->_value.mpfr, 0UL) == 0)
        ret = myobj.arg1;

    return ret;
}

int rpn_start(branch& myobj)
{
    int ret = -1;

    MIN_ARGUMENTS_RET(2, -(int)ret_runtime_error);
    ARG_MUST_BE_OF_TYPE_RET(0, cmd_number, -(int)ret_runtime_error);
    ARG_MUST_BE_OF_TYPE_RET(1, cmd_number, -(int)ret_runtime_error);

    // farg2 = last value of start command
    stack::copy_and_push_back(*_stack, _stack->size()-1, _calc_stack);
    myobj.farg2 = (number*)_calc_stack.back();
    _stack->pop_back();

    // farg1 = first value of start command
    stack::copy_and_push_back(*_stack, _stack->size()-1, _calc_stack);
    myobj.farg1 = (number*)_calc_stack.back();
    _stack->pop_back();

    // test value
    if (myobj.farg1->_value > myobj.farg2->_value)
        // last boundary lower than first boundary
        // -> next command shall be after 'next'
        // arg2 holds index of 'next'
        ret = myobj.arg2 + 1;

    return ret;
}

int rpn_for(branch& myobj)
{
    int ret;

    MIN_ARGUMENTS_RET(2, -(int)ret_runtime_error);
    ARG_MUST_BE_OF_TYPE_RET(0, cmd_number, -(int)ret_runtime_error);
    ARG_MUST_BE_OF_TYPE_RET(1, cmd_number, -(int)ret_runtime_error);

    symbol* sym = ((symbol*)seq_obj(myobj.arg1));

    // farg2 = last value of for command
    // arg1 = index of symbol to increase
    stack::copy_and_push_back(*_stack, _stack->size()-1, _calc_stack);
    myobj.farg2 = (number*)_calc_stack.back();
    _stack->pop_back();

    // farg1 = first value of for command
    stack::copy_and_push_back(*_stack, _stack->size()-1, _calc_stack);
    myobj.farg1 = (number*)_calc_stack.back();
    _stack->pop_back();

    // test value
    if (myobj.farg1->_value > myobj.farg2->_value)
        // last boundary lower than first boundary
        // -> next command shall be after 'next'
        // arg2 holds index of 'next'
        ret = myobj.arg2 + 1;
    else
    {
        // store symbol with first value
        _local_heap.add(sym->_value, (object*)myobj.farg1, myobj.farg1->size());
        ret = myobj.arg1 + 1;
    }

    return ret;
}

int rpn_next(branch& myobj)
{
    // arg1 = index of start or for command in program
    // farg1 = current count
    branch* start_or_for = (branch*)seq_obj(myobj.arg1);
    if (! myobj.arg_bool)
    {
        myobj.arg_bool = true;
        myobj.farg1 = start_or_for->farg1;
    }

    // increment then test
    mpfr_add_si(myobj.farg1->_value.mpfr, myobj.farg1->_value.mpfr, 1UL, MPFR_DEFAULT_RND);

    // for command: increment symbol too
    if (start_or_for->arg1 != -1)
    {
        //object* obj;
        //unsigned int size;
        symbol* var = (symbol*)seq_obj(start_or_for->arg1);

        // increase symbol variable
        _local_heap.replace_value(string(var->_value), myobj.farg1, myobj.farg1->size());
    }

    //test value
    if (myobj.farg1->_value > start_or_for->farg2->_value)
    {
        // end of loop
        myobj.arg_bool = false;// init again next time
        _calc_stack.pop_back(2);
        return -1;
    }
    else
    {
        // for command: next instruction will be after symbol variable
        if (start_or_for->arg1 != -1)
            return start_or_for->arg1 + 1;
        // start command: next instruction will be after start command
        else
            return myobj.arg1 + 1;
    }
}

int rpn_step(branch& myobj)
{
    int ret;
    MIN_ARGUMENTS_RET(1, -(int)ret_runtime_error);
    ARG_MUST_BE_OF_TYPE_RET(0, cmd_number, -(int)ret_runtime_error);

    number* step = (number*)_stack->pop_back();

    // end of loop if step is negative or zero
    if (mpfr_cmp_d(step->_value.mpfr, 0.0)<=0)
        ret = -1;
    else
    {
        // arg1 = index of start or for command in program
        // farg1 = current count
        branch* start_or_for = (branch*)seq_obj(myobj.arg1);
        if (! myobj.arg_bool)
        {
            myobj.arg_bool = true;
            myobj.farg1 = start_or_for->farg1;
        }

        // increment then test
        mpfr_add(myobj.farg1->_value.mpfr, myobj.farg1->_value.mpfr, step->_value.mpfr, MPFR_DEFAULT_RND);

        // for command: increment symbol too
        if (start_or_for->arg1 != -1)
        {
            //object* obj;
            //unsigned int size;
            symbol* var = (symbol*)seq_obj(start_or_for->arg1);

            // increase symbol variable
            _local_heap.replace_value(string(var->_value), myobj.farg1, myobj.farg1->size());
        }

        // test loop value is out of range
        if (myobj.farg1->_value > start_or_for->farg2->_value)
        {
            // end of loop
            myobj.arg_bool = false;// init again next time
            _calc_stack.pop_back(2);
            ret = -1;
        }
        else
        {
            // for command: next instruction will be after symbol variable
            if (start_or_for->arg1 != -1)
                ret = start_or_for->arg1 + 1;
            // start command: next instruction will be after start command
            else
                ret = myobj.arg1 + 1;
        }
    }
    
    return ret;
}
