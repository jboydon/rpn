void instr()
{
	MIN_ARGUMENTS(1);

    //TODO
#if 0
	// stringify only if not already a string
	if (_stack->get_type(0) != cmd_string)
	{
		stringstream out;
		((object*)_stack->back())->show(out);
		_stack->pop_back();

		ostring* str = new ostring(out.str().c_str());
	    _stack->push_back(str, sizeof(ostring), cmd_string);
	}
#endif
}

void strout()
{
    //TODO
#if 0
    MIN_ARGUMENTS(1);
	ARG_MUST_BE_OF_TYPE(0, cmd_string);

	string& entry = *((ostring*)_stack->back())->_value;
	_stack->pop_back();

	program prog;

	// make program from string in stack level 1
	if (program::parse(entry, prog) == ret_ok)
	{
		// run it
		prog.run(*_stack, *_heap);
	}
#endif
}
