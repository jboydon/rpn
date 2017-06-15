void re()
{
    MIN_ARGUMENTS(1);
    ARG_MUST_BE_OF_TYPE(0, cmd_complex);
    
    stack::copy_and_push_back(*_stack, _stack->size()-1, _branch_stack);
    _stack->pop_back();

    number* re = (number*)_stack->allocate_back(number::calc_size(), cmd_number);
    CHECK_MPFR(mpfr_set(re->_value.mpfr, ((number*)_branch_stack.get_obj(0))->_value.mpfr, floating_t::s_mpfr_rnd));
    _branch_stack.pop_back();
}
