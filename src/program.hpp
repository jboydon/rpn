#ifndef PROGRAM_HPP
#define PROGRAM_HPP

// std c
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <math.h>
#include <stdio.h>

extern "C" {
#include <readline/readline.h>
#include <readline/history.h>
}

// std c++
#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>
#include <fstream>
using namespace std;

// external libs
#include <mpfr.h>
#include "linenoise.h"

// internal includes
#include "constant.h"
#include "escape.h"
#include "version.h"
#include "debug.h"
#include "object.hpp"
#include "stack.hpp"

#include "linenoise.h"

//
struct if_layout_t
{
    if_layout_t():index_then_or_unti_or_repeat(-1),index_else(-1),index_end(-1),is_do_unti(false),is_while_repeat(false) { }
    int index_if_or_do_or_while;
    int index_then_or_unti_or_repeat;
    int index_else;
    int index_end;
    bool is_do_unti;
    bool is_while_repeat;
};

// program
class program : public stack
{
public:
    program(program* parent_prog = NULL) { _parent_prog = parent_prog; interrupt_now = false; }

    // run this program
    ret_value run(stack& stk, heap& hp)
    {
        bool go_out = false;
        ret_value ret = ret_ok;
        cmd_type_t type;

        // stack comes from outside
        _stack = &stk;

        // global heap comes from outside
        _heap = &hp;

        _err = ret_ok;
        _err_context = "";

        // branches for 'if'
        ret = preprocess();
        if (ret != ret_ok)
            return ret;

        // iterate commands
        for(int i = 0; (go_out==false) && (interrupt_now==false) && (i<(int)size());)
        {
            type = (cmd_type_t)seq_type(i);

            // could be an auto-evaluated symbol
            if (type == cmd_symbol)
            {
                auto_rcl((symbol*)seq_obj(i));
                i++;
            }

            // a keyword
            else if (type == cmd_keyword)
            {
                keyword* k = (keyword*)seq_obj(i);
                // call matching function
                (this->*(k->_fn))();
                switch(_err)
                {
                    // no pb -> go on
                    case ret_ok:
                        break;
                    // explicit go out software
                    case ret_good_bye:
                        go_out = true;
                        ret = ret_good_bye;
                        break;
                    default:
                        // error: abort prog
                        go_out = true;

                        // error: show it
                        if (show_error(_err, _err_context) == ret_deadly)
                        {
                            // pb showing error -> go out software
                            ret = ret_good_bye;
                        }
                        break;
                }
                i++;
            }

            // a branch keyword
            else if (type == cmd_branch)
            {
                // call matching function
                branch* b = (branch*)seq_obj(i);
                int next_cmd = (this->*(b->_fn))(*b);
                switch (next_cmd)
                {
                    case -1:
                        i++; // meaning 'next command'
                        break;
                    case -(int)ret_runtime_error:
                        // error: show it
                        (void)show_error(_err, _err_context);
                        go_out = true;// end of run
                        break;
                    default:
                        i = next_cmd;// new direction
                        break;
                }
            }

            // not a command, but a stack entry, manage it
            else
            {
                // copy the program stack entry to the running stack
                stack::copy_and_push_back(*this, i, stk);
                i++;
            }
        }
        
        if (interrupt_now)
        {
            fprintf(stderr, "\nInterrupted\n");
            interrupt_now = false;
        }

        return ret;
    }
    
    void stop()
    {
        interrupt_now = true;
    }

    bool compare_keyword(keyword* k, const char* str_to_compare, int len)
    {
        if (k->_len >= len)
            return strncmp(k->_value, str_to_compare, len) == 0;
        else
            return false;
    }

    bool compare_branch(branch* b, const char* str_to_compare, int len)
    {
        if (b->_len >= len)
            return strncasecmp(b->_value, str_to_compare, len) == 0;
        else
            return false;
    }

    ret_value preprocess(void)
    {
        // for if-then-else-end
        vector<struct if_layout_t> vlayout;
        int layout_index=-1;
        // for start-end-step
        vector<int> vstartindex;

        // analyse if-then-else-end branches
        // analyse start-{next, step} branches
        for(int i=0; i<(int)size(); i++)
        {
            int type = seq_type(i);
            if (type == cmd_branch)
            {
                branch* k = (branch*)seq_obj(i);
                if (compare_branch(k, "if", 2))
                {
                    if_layout_t layout;
                    layout.index_if_or_do_or_while = i;
                    vlayout.push_back(layout);
                    layout_index++;
                }
                else if (compare_branch(k, "then", 4))
                {
                    int next = i + 1;
                    if (next >= (int)size())
                        next = -1;

                    // nothing after 'then' -> error
                    if (next == -1)
                    {
                        // error: show it
                        show_syntax_error("missing end after then");
                        return ret_syntax;
                    }
                    if (layout_index<0)
                    {
                        // error: show it
                        show_syntax_error("missing if before then");
                        return ret_syntax;
                    }
                    if (vlayout[layout_index].index_then_or_unti_or_repeat != -1)
                    {
                        // error: show it
                        show_syntax_error("duplicate then");
                        return ret_syntax;
                    }
                    vlayout[layout_index].index_then_or_unti_or_repeat = i;
                    k->arg1 = next;
                    k->arg3 = vlayout[layout_index].index_if_or_do_or_while;
                }
                else if (compare_branch(k, "else", 4))
                {
                    int next = i + 1;
                    if (next >= (int)size())
                        next = -1;

                    // nothing after 'else' -> error
                    if (next == -1)
                    {
                        // error: show it
                        show_syntax_error("missing end after else");
                        return ret_syntax;
                    }
                    if (layout_index<0)
                    {
                        // error: show it
                        show_syntax_error("missing if before else");
                        return ret_syntax;
                    }
                    if (vlayout[layout_index].index_then_or_unti_or_repeat == -1)
                    {
                        // error: show it
                        show_syntax_error("missing then before else");
                        return ret_syntax;
                    }
                    if (vlayout[layout_index].index_else != -1)
                    {
                        // error: show it
                        show_syntax_error("duplicate else");
                        return ret_syntax;
                    }
                    vlayout[layout_index].index_else = i;
                    k->arg1 = next;// fill branch1 (if was false) of 'else'
                    k->arg3 = vlayout[layout_index].index_if_or_do_or_while;
                    ((branch*)seq_obj(vlayout[layout_index].index_then_or_unti_or_repeat))->arg2 = next;// fill branch2 (if was false) of 'then'
                }
                else if (compare_branch(k, "start", 5))
                    vstartindex.push_back(i);
                else if (compare_branch(k, "for", 3))
                {
                    vstartindex.push_back(i);
                    k->arg1 = i + 1;// arg1 points on symbol variable
                }
                else if(compare_branch(k, "next", 4))
                {
                    if (vstartindex.size() == 0)
                    {
                        // error: show it
                        show_syntax_error("missing start or for before next");
                        return ret_syntax;
                    }
                    k->arg1 = vstartindex[vstartindex.size() - 1]; // 'next' arg1 = 'start' index
                    ((branch*)seq_obj(vstartindex[vstartindex.size() - 1]))->arg2 = i; // 'for' or 'start' arg2 = 'next' index
                    vstartindex.pop_back();
                }
                else if (compare_branch(k, "step", 4))
                {
                    if (vstartindex.size() == 0)
                    {
                        // error: show it
                        show_syntax_error("missing start or for before step");
                        return ret_syntax;
                    }
                    k->arg1 = vstartindex[vstartindex.size() - 1];// fill 'step' branch1 = 'start' index
                    ((branch*)seq_obj(vstartindex[vstartindex.size() - 1]))->arg2 = i; // 'for' or 'start' arg2 = 'next' index
                    vstartindex.pop_back();
                }
                else if (compare_branch(k, "->", 2))
                {
                    k->arg1 = i;// arg1 is '->' command index in program
                }
                else if (compare_branch(k, "do", 2))
                {
                    if_layout_t layout;
                    layout.index_if_or_do_or_while = i;
                    layout.is_do_unti = true;
                    vlayout.push_back(layout);
                    layout_index++;
                }
                else if (compare_branch(k, "until", 4))
                {
                    int next = i + 1;
                    if (next >= (int)size())
                        next = -1;

                    // nothing after 'unti' -> error
                    if (next == -1)
                    {
                        // error: show it
                        show_syntax_error("missing end");
                        return ret_syntax;
                    }
                    if (layout_index<0 || !vlayout[layout_index].is_do_unti)
                    {
                        // error: show it
                        show_syntax_error("missing do");
                        return ret_syntax;
                    }
                    if (vlayout[layout_index].index_then_or_unti_or_repeat != -1)
                    {
                        // error: show it
                        show_syntax_error("duplicate until");
                        return ret_syntax;
                    }
                    vlayout[layout_index].index_then_or_unti_or_repeat = i;
                }
                else if (compare_branch(k, "while", 4))
                {
                    if_layout_t layout;
                    layout.index_if_or_do_or_while = i;
                    layout.is_while_repeat = true;
                    vlayout.push_back(layout);
                    layout_index++;
                }
                else if (compare_branch(k, "repeat", 5))
                {
                    if (layout_index<0 || !vlayout[layout_index].is_while_repeat)
                    {
                        // error: show it
                        show_syntax_error("missing while");
                        return ret_syntax;
                    }
                    if (vlayout[layout_index].index_then_or_unti_or_repeat != -1)
                    {
                        // error: show it
                        show_syntax_error("duplicate repeat");
                        return ret_syntax;
                    }
                    vlayout[layout_index].index_then_or_unti_or_repeat = i;
                }
                else if (compare_branch(k, "end", 3))
                {
                    int next = i + 1;
                    if (next >= (int)size())
                        next = -1;

                    if (layout_index<0)
                    {
                        // error: show it
                        show_syntax_error("missing branch instruction before end");
                        return ret_syntax;
                    }
                    else
                    {
                        if (vlayout[layout_index].is_do_unti)
                        {
                            // this end closes a do..unti
                            if (vlayout[layout_index].index_end != -1)
                            {
                                // error: show it
                                show_syntax_error("duplicate end");
                                return ret_syntax;
                            }

                            if (vlayout[layout_index].index_then_or_unti_or_repeat == -1)
                            {
                                // error: show it
                                show_syntax_error("missing until");
                                return ret_syntax;
                            }

                            k->arg1 = vlayout[layout_index].index_if_or_do_or_while + 1;
                            layout_index--;
                        }
                        else if (vlayout[layout_index].is_while_repeat)
                        {
                            // this end closes a while..repeat
                            if (vlayout[layout_index].index_end != -1)
                            {
                                // error: show it
                                show_syntax_error("duplicate end");
                                return ret_syntax;
                            }

                            k->arg2 = vlayout[layout_index].index_if_or_do_or_while + 1;
                            
                            if (vlayout[layout_index].index_then_or_unti_or_repeat == -1)
                            {
                                // error: show it
                                show_syntax_error("missing repeat");
                                return ret_syntax;
                            }

                            //fill 'repeat' arg1 with 'end+1'
                            ((branch*)seq_obj(vlayout[layout_index].index_then_or_unti_or_repeat))->arg1 = i + 1;
                            layout_index--;
                        }
                        else
                        {
                            // this end closes an if..then..(else)
                            if (vlayout[layout_index].index_end != -1)
                            {
                                // error: show it
                                show_syntax_error("duplicate end");
                                return ret_syntax;
                            }
                            if (vlayout[layout_index].index_else != -1)
                                //fill 'end' branch of 'else'
                                ((branch*)seq_obj(vlayout[layout_index].index_else))->arg2 = i;
                            else
                            {
                                //fill 'end' branch of 'then'
                                if (vlayout[layout_index].index_then_or_unti_or_repeat != -1)
                                    ((branch*)seq_obj(vlayout[layout_index].index_then_or_unti_or_repeat))->arg2 = i;
                                else
                                {
                                    // error: show it
                                    show_syntax_error("missing then");
                                    return ret_syntax;
                                }
                            }
                            layout_index--;
                        }
                    }
                }
            }
        }
        if (layout_index >= 0)
        {
            // error: show it
            show_syntax_error("missing end");
            return ret_syntax;
        }
        if (vstartindex.size() > 0)
        {
            // error: show it
            show_syntax_error("missing next or step after for or start");
            return ret_syntax;
        }
        return ret_ok;
    }

    ret_value show_error()
    {
        ret_value ret;

        // show last recorded error
        cerr<<_err_context<<": error "<<_err<<": "<<s_ret_value_string[_err]<<endl;
        switch(_err)
        {
            case ret_internal:
            case ret_deadly:
                ret = ret_deadly;
                break;
            default:
                ret = ret_ok;
                break;
        }
        
        return ret;
    }

    ret_value show_error(ret_value err, string& context)
    {
        // record error
        _err = err;
        _err_context = context;
        return show_error();
    }

    ret_value show_error(ret_value err, const char* context)
    {
        // record error
        _err = err;
        _err_context = context;
        return show_error();
    }

    void show_syntax_error(const char* context)
    {
        // record error
        _err = ret_syntax;
        _err_context = context;
        (void)show_error();
    }

    ret_value get_err(void) { return _err; }

    static void show_stack(stack& st, bool show_separator = true)
    {
        if (st.size() == 1)
        {
            ((object*)st.back())->show();
            printf("\n");
        }
        else
        {
            for (int i = st.size()-1; i>=0; i--)
            {
                if (show_separator)
                    printf("%d%s", i+1, SHOW_STACK_SEPARATOR);
                ((object*)st[i])->show();
                printf("\n");
            }
        }
    }
    
    static void apply_default()
    {
        //default float precision, float mode
        number::s_mode = DEFAULT_MODE;
        number::s_decimal_digits = DEFAULT_DECIMAL_DIGITS;

        // format for mpfr_printf 
        stringstream ss;
        ss << number::s_decimal_digits;
        number::s_mpfr_printf_format = string(MPFR_FORMAT_BEG) + ss.str() + string(MPFR_FORMAT_STD);

        // default calc precision for MPFR
        floating_t::s_mpfr_prec = (mpfr_prec_t)MPFR_DEFAULT_PREC_BITS;
        floating_t::s_mpfr_prec_bytes = MPFR_DEFAULT_STORING_LENGTH_BYTES;
    }

private:
    
    bool interrupt_now;
    
    // current error and its context
    ret_value _err;
    string _err_context;

    // global stack holding results for user
    stack* _stack;

    // global heap (sto, rcl)
    heap* _heap;

    // local heap for local loop variables (for..next)
    heap _local_heap;

    // calc stack internally used by branch and calc commands
    stack _calc_stack;

    // parent prog for inheriting heaps
    program* _parent_prog;

    int stack_size()
    {
        return _stack->size();
    }
    
private:
    static const char* s_ret_value_string[ret_max];

    // carefull : some of these macros modify program flow
    #define ERR_CONTEXT(err) do { _err = (err); _err_context = __FUNCTION__; } while(0)
    #define MIN_ARGUMENTS(num) do { if (stack_size()<(num)) { ERR_CONTEXT(ret_missing_operand); return; } } while(0)
    #define MIN_ARGUMENTS_RET(num, ret) do { if (stack_size()<(num)) { ERR_CONTEXT(ret_missing_operand); return (ret); } } while(0)
    #define ARG_MUST_BE_OF_TYPE(num, type) do { if (_stack->get_type(num) != (type)) { ERR_CONTEXT(ret_bad_operand_type); return; } } while(0)
    #define ARG_MUST_BE_OF_TYPE_RET(num, type, ret) do { if (_stack->get_type(num) != (type)) { ERR_CONTEXT(ret_bad_operand_type); return (ret); } } while(0)
    #define IS_ARG_TYPE(num, type) (_stack->get_type(num) == (type))
    #define CHECK_MPFR(op) do { (void)(op); } while(0)

    // keywords
    struct keyword_t
    {
        cmd_type_t type;
        char name[MAX_COMMAND_LENGTH];
        program_fn_t fn;
        string comment;
    };
    static keyword_t s_keywords[];

    // keywords implementation
    #include "rpn-general.hpp"
    #include "rpn-real.hpp"
    #include "rpn-complex.hpp"
    #include "rpn-test.hpp"
    #include "rpn-stack.hpp"
    #include "rpn-string.hpp"
    #include "rpn-branch.hpp"
    #include "rpn-store.hpp"
    #include "rpn-program.hpp"
    #include "rpn-trig.hpp"
    #include "rpn-logs.hpp"
    #include "rpn-test-core.hpp"

    // parser
    #include "parse.hpp"
};

#endif
