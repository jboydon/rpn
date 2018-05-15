void test_get_stack(string& stack_is, stack& stk)
{
    // write stack in a string, each entry separated between commas
    for (int i = 0; i < (int)stk.size(); i++)
    {
        FILE* tmp_file = tmpfile();
        char* line = NULL;
        size_t len;

        if (i > 0)
            stack_is += ", ";

        if (tmp_file != NULL)
        {
            ((object*)stk.seq_obj(i))->show(tmp_file);

            // write stack in a tmp file
            (void)rewind(tmp_file);
            if (getline(&line, &len, tmp_file) >=0)
            {
                stack_is += line;
                free(line);
            }
            (void)fclose(tmp_file);
        }
        else
            ERR_CONTEXT(ret_runtime_error);
    }
}

void test_show_result(string title, int tests, int tests_failed, int steps, int steps_failed)
{
    printf("%s: run %d tests: %d passed, ", title.c_str(), tests, tests-tests_failed);
    if(tests_failed>0)
        printf(FG_RED);
    printf("%d failed", tests_failed);
    if(tests_failed>0)
        printf(COLOR_OFF);
    
    printf(" (%d steps: %d passed, ", steps, steps-steps_failed);
    if(steps_failed>0)
        printf(FG_RED);
    printf("%d failed", steps_failed);
    if(steps_failed>0)
        printf(COLOR_OFF);
    printf(")\n");
}

void rpn_test()
{
    MIN_ARGUMENTS(1);
    ARG_MUST_BE_OF_TYPE(0, cmd_string);

    int total_tests=0;
    int total_tests_failed=0;
    int total_steps=0;
    int total_steps_failed=0;

    string test_filename = ((ostring*)_stack->pop_back())->_value;
    test(test_filename, total_tests, total_tests_failed, total_steps, total_steps_failed);
    test_show_result("Total", total_tests, total_tests_failed, total_steps, total_steps_failed);
}

void test(string test_filename, int& total_tests, int& total_tests_failed, int& total_steps, int& total_steps_failed)
{
    const string stack_size("-> stack size should be ");
    const string stack_value("-> stack should be ");
    const string cmd_error("-> error should be ");
    const string cmd_exit("exit test");

    ifstream test_file(test_filename.c_str());
    int tests = 0;
    int tests_failed = 0;
    int steps = 0;
    int steps_failed = 0;

    if (test_file.is_open())
    {
        string test_title;
        string entry;
        ret_value ret;
        stack stk;
        heap hp;
        bool failed = false;
        bool is_first_step = false;
        bool is_test_error_shown = false;
        int last_err = 0;
        stringstream cerr_buffer;
        streambuf* cerr_old_buffer;

        // redirect cerr
        cerr_old_buffer = cerr.rdbuf(cerr_buffer.rdbuf());

        while (!test_file.eof())
        {
            getline(test_file, entry);

            if (entry.substr(0,8)=="#include")
                test(entry.substr(9), total_tests, total_tests_failed, total_steps, total_steps_failed);
            else if (entry.substr(0,2)=="##")
                printf("\n%s: %s\n", test_filename.c_str(), entry.substr(3).c_str());
            else if (entry.substr(0,2)=="# ")
            {
                // indicates the status of previous test
                if (failed == false && tests > 0)
                    printf(FG_GREEN " PASSED" COLOR_OFF "\n");
                failed = false;

                // read a test title
                test_title = entry;
                is_first_step = true;
                is_test_error_shown = false;
                printf("%s", test_title.c_str());
            }
            // treat "-> stack size should be "
            else if (entry.find(stack_size, 0) == 0)
            {
                // count test and step
                if (is_first_step)
                    tests++;
                steps++;

                // check current stack size
                istringstream isub;
                int size;

                isub.str(entry.substr(stack_size.size()));
                isub>>size;
                if (size != (int)stk.size())
                {
                    // count fail test and step
                    if (!is_test_error_shown)
                    {
                        printf(FG_RED " FAIL" COLOR_OFF "\n");
                        tests_failed++;
                        is_test_error_shown = true;
                    }
                    steps_failed++;

                    // show failure
                    printf("\t%s\n", entry.c_str());
                    printf("\tbut real stack size is " FG_RED "%d" COLOR_OFF "\n", stk.size());
                    failed = true;
                }
                is_first_step = false;
            }
            // treat "-> stack should be "
            else if (entry.find(stack_value, 0) == 0)
            {
                // count test
                if (is_first_step)
                    tests++;
                steps++;

                // check current stack value
                string stack_should_be = entry.substr(stack_value.size());
                string stack_is;

                test_get_stack(stack_is, stk);

                if (stack_is != stack_should_be)
                {
                    // count fail test and step
                    if (!is_test_error_shown)
                    {
                        printf(FG_RED " FAIL" COLOR_OFF "\n");
                        tests_failed++;
                        is_test_error_shown = true;
                    }
                    steps_failed++;

                    // show failure
                    printf("\t%s\n", entry.c_str());
                    printf("\tbut real stack is " FG_RED "%s" COLOR_OFF "\n", stack_is.c_str());
                    failed = true;
                }
                is_first_step = false;
            }
            // treat "-> error should be "
            else if (entry.find(cmd_error, 0) == 0)
            {
                // count test
                if (is_first_step)
                    tests++;
                steps++;

                // check current error
                istringstream isub;
                int err_should_be;
                isub.str(entry.substr(cmd_error.size()));
                isub>>err_should_be;
                if (err_should_be != last_err)
                {
                    // count fail test and step
                    if (!is_test_error_shown)
                    {
                        printf(FG_RED " FAIL" COLOR_OFF "\n");
                        tests_failed++;
                        is_test_error_shown = true;
                    }
                    steps_failed++;

                    // show failure
                    printf("\t%s\n", entry.c_str());
                    printf("\tbut last error is " FG_RED "%d" COLOR_OFF "\n", last_err);
                    failed = true;
                }
                is_first_step = false;
            }
            else if (entry.find(cmd_exit, 0) == 0)
            {
                // forced test end
                break;
            }
            else if (entry.size() > 0)
            {
                // parse entry and run line
                program prog;
                ret = program::parse(entry.c_str(), prog);
                if (ret == ret_ok)
                {
                    // run it
                    (void)prog.run(stk, hp);
                    last_err = (int)prog.get_err();
                }
            }
        }

        // last test
        // indicates the status of previous test
        if (failed == false && tests > 0)
            printf(FG_GREEN " PASSED" COLOR_OFF "\n");

        // cerr back
        cerr.rdbuf(cerr_old_buffer);
        
        // conclusion: show and keep for total
        if (tests != 0)
        {
            test_show_result(test_filename, tests, tests_failed, steps, steps_failed);
            
            total_tests += tests;
            total_tests_failed += tests_failed;
            total_steps += steps;
            total_steps_failed += steps_failed;
        }
    }
    else
        fprintf(stderr, "test file '%s' not found\n", test_filename.c_str());
}
