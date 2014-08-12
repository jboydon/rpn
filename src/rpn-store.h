//
void sto(void)
{
	MIN_ARGUMENTS(2);
	ARG_IS_OF_TYPE(0, cmd_symbol);

	string name = getn();
	_heap->add(name, _stack->get_obj(0), _stack->get_len(0), _stack->get_type(0));
	_stack->pop_back();
}

void rcl(void)
{
	MIN_ARGUMENTS(1);
	ARG_IS_OF_TYPE(0, cmd_symbol);

	void* obj;
	unsigned int size;
	int type;
	if (_heap->get(getn(), obj, size, type))
		_stack->push_back(obj, size, type);
	else
		ERR_CONTEXT(ret_unknown_variable);
}

// carefull : this is not a langage command
void auto_rcl(symbol* symb)
{
	if (symb->_auto_eval)
	{
		void* obj;
		unsigned int size;
		int type;
		if (_heap->get(*symb->_name, obj, size, type))
			_stack->push_back(obj, size, type);
		else
			_stack->push_back(symb, sizeof(symbol), cmd_symbol);
	}
	else
		_stack->push_back(symb, sizeof(symbol), cmd_symbol);
}

void purge(void)
{
	MIN_ARGUMENTS(1);
	ARG_IS_OF_TYPE(0, cmd_symbol);

	if (!_heap->erase(getn()))
		ERR_CONTEXT(ret_unknown_variable);
}

void vars(void)
{
	object* obj;
	unsigned int size;
	int type;
	string name;

	for (int i=0; i<(int)_heap->size(); i++)
	{
		(void)_heap->get_by_index(i, name, (void*&)obj, size, type);
		cout<<"var "<<i+1<<": name '"<<name<<"', type "<<cmd_type_string[type]<<", value ";
		obj->show();
		cout<<endl;
	}
}