public:
static ret_value parse(const char* entry, program& prog)
{
    vector<string> entries;
    ret_value ret = ret_ok;

    //1. cut global entry string into shorter strings
    if (_cut(entry, entries))
    {
        //2. make an object from each entry, and add it to the program
        for (vector<string>::iterator it = entries.begin(); it != entries.end(); it++)
        {
            string remaining_entry;
            string main_entry = (*it);
            while(main_entry.size()>0)
            {
                // remaining_entry is used only in case of concatenated entry
                // ex:  entry="1 2+" -> vector<string> = {"1", "2+"} -> first "1", second "2" and remaining_entry="+"
                // this remaining entry is treated as an entry

                // TODO errors ?
                _obj_from_string(main_entry, prog, remaining_entry);
                main_entry = remaining_entry;
            }
        }
    }

    return ret;
}

// interactive entry and decoding
static ret_value entry(program& prog)
{
    char* entry;
    ret_value ret;
    int entry_len = 0;

    // linenoise for entry
    linenoiseSetCompletionCallback(program::entry_completion_generator);

    // get user entry
    entry = linenoise(PROMPT, &entry_len);

    // Ctrl-C
    if (linenoiseKeyType() == 1)
    {
        // simply clear the line if there is an entry or go out
        if (entry_len > 0)
            ret = ret_line_cancelled;
        else
            ret = ret_good_bye;
    }
    else
    {
        if (entry != NULL)
        {
            // parse it
            ret = parse(entry, prog);

            // keep history
            if (entry[0] != 0)
                linenoiseHistoryAdd(entry);
        }
        else
            ret = ret_internal;
    }

    free(entry);

    return ret;
}

private:
static ret_value get_fn(const char* fn_name, program_fn_t& fn, cmd_type_t& type)
{
    unsigned int i = 0;
    while(s_keywords[i].type != cmd_max)
    {
        if (strncasecmp(fn_name, s_keywords[i].name, sizeof(s_keywords[i].name)) == 0)
        {
            fn = s_keywords[i].fn;
            type = s_keywords[i].type;
            return ret_ok;
        }
        i++;
    }
    return ret_unknown_err;
}

static bool _cut(const char* entry, vector<string>& entries)
{
    string tmp;
    int len = strlen(entry);

    for (int i=0; i < len; i++)
    {
        switch(entry[i])
        {
            //symbol
            case '\'':
                //push prec entry if exists
                if (tmp.size() > 0)
                {
                    entries.push_back(tmp);
                    tmp.clear();
                }
                //get symbol
                tmp='\'';
                i++;
                while((i < len) && entry[i]!='\'')
                    tmp += entry[i++];
                if ((i < len) && entry[i] != '\'')
                    tmp += '\'';
                entries.push_back(tmp);
                tmp.clear();
                break;

            //string
            case '"':
                //push prec entry if exists
                if (tmp.size() > 0)
                {
                    entries.push_back(tmp);
                    tmp.clear();
                }
                //get expression
                tmp='"';
                i++;
                while(i < len && entry[i]!='"')
                    tmp += entry[i++];

                if ((i < len) && entry[i] != '"')
                    tmp += '"';

                entries.push_back(tmp);
                tmp.clear();
                break;

            //program
            case '<':
                //push prec entry if exists
                if (tmp.size()>0)
                {
                    entries.push_back(tmp);
                    tmp.clear();
                }

                if (strncmp(&entry[i], "<<", 2) == 0)
                {
                    int up = 1;

                    // found a program begin
                    i += 2;
                    tmp = "<< ";

                    // trim leading spaces
                    while (i < len && isspace(entry[i]))
                        i++;

                    while(i < len)
                    {
                        if (strncmp(&entry[i], "<<", 2) == 0)
                        {
                            up++;
                            i += 2;
                            tmp += "<< ";
                            // trim leading spaces
                            while (i < len && isspace(entry[i]))
                                i++;
                        }
                        else if (strncmp(&entry[i], ">>", 2) == 0)
                        {
                            if (isspace(entry[i-1]) && entry[i-2]!='>')
                                tmp += ">>";
                            else
                                tmp += " >>";

                            up--;
                            i += 2;

                            // trim trailing spaces
                            while (i < len && isspace(entry[i]))
                                i++;

                            // found end
                            if (up == 0)
                                break;
                        }
                        else
                        {
                            tmp += entry[i];
                            i++;
                        }
                    }
                    while((up--)>0)
                        tmp += " >>";

                    if (tmp.size()>0)
                    {
                        entries.push_back(tmp);
                        tmp.clear();
                    }
                    i--;// i has move 1 too far
                }
                else
                    // reinject '<'' which is not a prog begin
                    tmp = "<";
                break;

            //complex
            case '(':
                //push prec entry if exists
                if (tmp.size() > 0)
                {
                    entries.push_back(tmp);
                    tmp.clear();
                }
                //get complex
                while((i < len) && entry[i]!=')')
                    tmp += entry[i++];
                if ((i < len) && entry[i] != ')')
                    tmp += ')';
                if (tmp.size()>0)
                {
                    entries.push_back(tmp);
                    tmp.clear();
                }
                break;

            //other
            default:
                if (!isspace(entry[i]))
                    tmp += entry[i];
                else
                {
                    if (tmp.size()>0)
                    {
                        entries.push_back(tmp);
                        tmp.clear();
                    }
                }
                break;
        }
    }
    if (tmp.size()>0)
    {
        entries.push_back(tmp);
        tmp.clear();
    }
    return entries.size()>0;
}

static bool get_keyword(const string& entry, program& prog, string& remaining_entry)
{
    program_fn_t fn;
    unsigned int obj_len;
    cmd_type_t type;
    bool ret = false;

    // could be a command
    if (get_fn(entry.c_str(), fn, type) == ret_ok)
    {
        if (type == cmd_keyword)
        {
            // allocate keyword object
            obj_len = sizeof(keyword)+entry.size()+1;
            keyword* new_obj = (keyword*)prog.allocate_back(obj_len, cmd_keyword);
            new_obj->set(fn, entry.c_str(), entry.size());
            ret = true;
        }
        else if (type == cmd_branch)
        {
            // allocate branch object
            obj_len = sizeof(branch)+entry.size()+1;
            branch* new_obj = (branch*)prog.allocate_back(obj_len, cmd_branch);
            new_obj->set((branch_fn_t)fn, entry.c_str(), entry.size());
            ret = true;
        }
    }

    return ret;
}

static bool get_symbol(const string& entry, program& prog, string& remaining_entry)
{
    bool ret = false;
    int entry_len = entry.size();
    unsigned int obj_len;

    if (entry_len>=1 && entry[0]=='\'')
    {
        if (entry_len == 1)
        {
            // void symbol entry, like '
            // total object length
            obj_len = sizeof(symbol)+1;

            // allocate and set object
            // symbol beginning with ' is not autoevaluated
            symbol* new_obj = (symbol*)prog.allocate_back(obj_len, cmd_symbol);
            new_obj->set("", 0, false);
        }
        else
        {
            // symbol entry, like 'toto' or 'toto
            int naked_entry_len;

            // entry length without prefix / postfix
            naked_entry_len = entry[entry_len-1]=='\''?(entry_len-2):(entry_len-1);
            // total object length
            obj_len = sizeof(symbol)+naked_entry_len+1;

            // allocate and set object
            // symbol beginning with ' is not autoevaluated
            symbol* new_obj = (symbol*)prog.allocate_back(obj_len, cmd_symbol);
            new_obj->set(entry.substr(1, naked_entry_len).c_str(), naked_entry_len, false);
        }
        ret = true;
    }

    return ret;
}

static bool get_other(const string& entry, program& prog, string& remaining_entry)
{
    bool ret = false;
    int entry_len = entry.size();
    unsigned int obj_len;

    if (entry_len>=1)
    {
        // entry which is nothing is considered as an auto-evaluated symbol
        int naked_entry_len;

        // entry length without prefix / postfix
        naked_entry_len = entry[entry_len-1]=='\''?(entry_len-1):(entry_len);
        // total object length
        obj_len = sizeof(symbol)+naked_entry_len+1;

        // allocate and set object
        // symbol not beginning with ' is autoevaluated (ie is evaluated when pushed on stack)
        symbol* new_obj = (symbol*)prog.allocate_back(obj_len, cmd_symbol);
        new_obj->set(entry.c_str(), naked_entry_len, true);
        ret = true;
    }

    return ret;
}

static bool get_string(const string& entry, program& prog, string& remaining_entry)
{
    bool ret = false;
    unsigned int obj_len;
    int entry_len = entry.size();
    if (entry_len>=1 && entry[0]=='"')
    {
        if (entry_len == 1)
        {
            // total object length
            obj_len = sizeof(ostring)+1;

            // allocate and set object
            ostring* new_obj = (ostring*)prog.allocate_back(obj_len, cmd_string);
            new_obj->set("", 0);
        }
        else
        {
            int naked_entry_len;

            // entry length without prefix / postfix
            naked_entry_len = entry[entry_len-1]=='"'?(entry_len-2):(entry_len-1);

            // total object length
            obj_len = sizeof(ostring)+naked_entry_len+1;

            // allocate and set object
            ostring* new_obj = (ostring*)prog.allocate_back(obj_len, cmd_string);
            new_obj->set(entry.substr(1, naked_entry_len).c_str(), naked_entry_len);
        }
        ret = true;
    }

    return ret;
}

static bool get_program(string& entry, program& prog, string& remaining_entry)
{
    bool ret = false;
    unsigned int obj_len;
    int entry_len = entry.size();
    if (entry_len>=2 && entry[0]=='<' && entry[1]=='<')
    {
        int naked_entry_len;

        // entry length without prefix / postfix
        if (entry_len>=4 && entry[entry_len-1]=='>' && entry[entry_len-2]=='>')
            naked_entry_len = entry_len-4;
        else
            naked_entry_len = entry_len-2;

        // total object length
        obj_len = sizeof(oprogram)+naked_entry_len+1;

        // allocate and set object
        oprogram* new_obj = (oprogram*)prog.allocate_back(obj_len, cmd_program);
        new_obj->set(&entry[2], naked_entry_len);

        ret = true;
    }
    return ret;
}

static bool get_number(string& entry, program& prog, string& remaining_entry)
{
    char* endptr;
    bool ret = false;

    if (entry.size() > 0)
    {
        // pre parse to avoid doing a useless allocation
        // detect the begining of a number including nan, inf, @nan@, @inf@
        if (entry.find_first_of("+-0123456789.ni@", 0) == 0)
        {
            // detect an arbitrary base entry like 3bXXX or 27bYYY
            int base = 0;
            size_t base_detect = entry.find_first_of("b", 0);
            if (base_detect == 1 || base_detect == 2)
                if (sscanf(entry.c_str(), "%db", &base) == 1 && base >= 2 && base <= 62)
                    entry=entry.substr(base_detect+1);
                else
                    base = 0;

            number* num = (number*)prog.allocate_back(number::calc_size(), cmd_number);

            int mpfr_ret = mpfr_strtofr(num->_value.mpfr, entry.c_str(), &endptr, base, MPFR_DEFAULT_RND);
            if (endptr != NULL && endptr != entry.c_str())
            {
                // determine representation
                if (base != 0)
                {
                    num->_representation = number::base;
                    num->_base = base;
                }
                else
                {
                    string beg = entry.substr(0, 2);
                    if (beg == "0x" || beg == "0X")
                        num->_representation = number::hex;
                    else if (beg == "0b" || beg == "0B")
                        num->_representation = number::bin;
                    else
                        num->_representation = number::dec;
                }

                ret = true;

                // remaining string if any
                remaining_entry = endptr;
            }
            else
                (void)prog.pop_back();
        }
    }

    return ret;
}

static bool get_complex(const string& entry, program& prog, string& remaining_entry)
{
    char* endptr;
    bool ret = false;

    if (entry.size() > 0)
    {
        size_t comma = entry.find(',');
        if (comma != string::npos)
        {
            complex* cplx;

            // pre parse RE to avoid doing a useless allocation
            // detect the begining of a number, including nan, inf, @nan@, @inf@
            string re_str = entry.substr(1, comma-1).c_str();
            if (re_str.find_first_of(" +-0123456789.ni@", 0) == 0)
            {
                cplx = (complex*)prog.allocate_back(complex::calc_size(), cmd_complex);

                int mpfr_ret = mpfr_strtofr(cplx->re()->mpfr, re_str.c_str(), &endptr, 0, MPFR_DEFAULT_RND);
                if (endptr != NULL && endptr != re_str.c_str())
                {
                    // determine representation
                    string beg = re_str.substr(0, 2);
                    if (beg == "0x" || beg == "0X")
                        cplx->_representation = complex::hex;
                    else
                        cplx->_representation = complex::dec;

                    ret = true;
                }
                else
                    (void)prog.pop_back();
            }

            // pre parse IM to avoid doing a useless allocation
            // detect the begining of a number, including nan, inf, @nan@, @inf@
            string im_str = entry.substr(comma+1).c_str();
            if (ret == true && im_str.find_first_of(" +-0123456789.ni@", 0) == 0)
            {
                ret = false;
                int mpfr_ret = mpfr_strtofr(cplx->im()->mpfr, im_str.c_str(), &endptr, 0, MPFR_DEFAULT_RND);
                if (endptr != NULL && endptr != im_str.c_str())
                {
                    // determine representation
                    string beg = im_str.substr(0, 2);
                    if (beg == "0x" || beg == "0X")
                        cplx->_representation = complex::hex;
                    else
                        cplx->_representation = complex::dec;

                    ret = true;
                }
                else
                    (void)prog.pop_back();
            }
        }
    }

    return ret;
}

static bool _obj_from_string(string& entry, program& prog, string& remaining_entry)
{
    bool ret = false;

    remaining_entry.erase();

    if (get_number(entry, prog, remaining_entry))
        ret = true;
    else if (get_symbol(entry, prog, remaining_entry))
        ret = true;
    else if (get_string(entry, prog, remaining_entry))
        ret = true;
    else if (get_program(entry, prog, remaining_entry))
        ret = true;
    else if (get_keyword(entry, prog, remaining_entry))
        ret = true;
    else if (get_complex(entry, prog, remaining_entry))
        ret = true;
    else
        // nothing, considered as an auto-evaluated symbol
        if (get_other(entry, prog, remaining_entry))
            ret = true;

    return ret;
}

static void entry_completion_generator(const char* text, linenoiseCompletions* lc)
{
    int i = 0;
    int text_len = strnlen(text, 6);

    // propose all keywords
    if (text_len ==0)
    {
        while(s_keywords[i].type != cmd_max)
        {
            if (s_keywords[i].fn != NULL)
                // add all keywords
                linenoiseAddCompletion(lc, s_keywords[i].name);
            i++;
        }
    }
    // propose keywords matching to text begining
    else
    {
        while(s_keywords[i].type != cmd_max)
        {
            if (s_keywords[i].fn != NULL)
            {
                // compare list entry with text, return if match
                if (strncmp(s_keywords[i].name, text, text_len) == 0)
                    linenoiseAddCompletion(lc, s_keywords[i].name);
            }
            i++;
        }
    }
}
