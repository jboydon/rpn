void dec()
{
	binary::s_mode = binary::dec;
}

void hex()
{
	binary::s_mode = binary::hex;
}

void oct()
{
	binary::s_mode = binary::oct;
}

void bin()
{
	binary::s_mode = binary::bin;
}

void rtob()
{
	MIN_ARGUMENTS(1);
	ARG_MUST_BE_OF_TYPE(0, cmd_number);

	binary bin((integer_t)getf());
    _stack->push_back(&bin, sizeof(binary), cmd_binary);

}

void btor()
{
	MIN_ARGUMENTS(1);
	ARG_MUST_BE_OF_TYPE(0, cmd_binary);

	putf((float_t)((binary*)_stack->back())->_value);
}
