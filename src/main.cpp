// std c
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <linux/limits.h>

// internal includes
#include "program.hpp"

static heap s_global_heap;
static stack s_global_stack;
static program* s_prog_to_interrupt = NULL;

// actions to be done at rpn exit
void exit_interactive_rpn()
{
    struct passwd* pw = getpwuid(getuid());
    if (pw != NULL)
    {
        char history_path[PATH_MAX];
        sprintf(history_path, "%s/%s", pw->pw_dir, HISTORY_FILE);

        // trunc current history
        ofstream history(history_path, ios_base::out|ios_base::trunc);
        history.close();

        // save it
        if (linenoiseHistorySave(history_path) != 0)
            fprintf(stderr, "warning, could not save %s (errno=%d, '%s')\n", history_path, errno, strerror(errno));
    }
}

// actions to be done at rpn exit
void init_interactive_rpn()
{
    struct passwd* pw = getpwuid(getuid());
    if (pw != NULL)
    {
        char history_path[PATH_MAX];
        sprintf(history_path, "%s/%s", pw->pw_dir, HISTORY_FILE);

        // don't care about errors
        linenoiseHistorySetMaxLen(HISTORY_FILE_MAX_LINES);
        linenoiseHistoryLoad(history_path);
    }
}

// handle CtrlC signal
static void ctrlc_handler(int sig, siginfo_t* siginfo, void* context)
{
    if (s_prog_to_interrupt != NULL)
    {
        s_prog_to_interrupt->stop();
        s_prog_to_interrupt = NULL;
    }

    exit_interactive_rpn();
}

// handle SIGSEGV signal
static void segv_handler(int sig, siginfo_t* siginfo, void* context)
{
    fprintf(stderr, "Internal error\n");
    s_prog_to_interrupt->stop();
    s_prog_to_interrupt = NULL;
}

static void catch_signals(program* prog)
{
    struct sigaction act;

    s_prog_to_interrupt = prog;

    act.sa_sigaction = &ctrlc_handler;
    act.sa_flags = SA_SIGINFO;
    if (sigaction(SIGINT, &act, NULL) < 0)
        (void)fprintf(stderr, "Warning, Ctrl-C cannot be catched [errno=%d '%s']", errno, strerror(errno));

    act.sa_sigaction = &segv_handler;
    act.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &act, NULL) < 0)
        (void)fprintf(stderr, "Warning, SIGSEGV cannot be catched [errno=%d '%s']", errno, strerror(errno));
}

//
int main(int argc, char* argv[])
{
    int ret = 0;
    bool go_out = false;

    // apply default configuration
    program::apply_default();

    // run with interactive prompt
    if (argc == 1)
    {
        // init history
        init_interactive_rpn();

        // entry loop
        while (!go_out)
        {
            // make program from interactive entry
            program prog;
            switch(program::entry(prog))
            {
                case ret_good_bye:
                    // go out
                    go_out = true;
                    break;
                case ret_line_cancelled:
                    // simply wait for another entry
                    break;
                default:
                    // user could stop prog with CtrlC
                    catch_signals(&prog);

                    // run it
                    if (prog.run(s_global_stack, s_global_heap) == ret_good_bye)
                        break;
                    else
                        program::show_stack(s_global_stack);
                    break;
            }
        }

        // manage history and exit
        exit_interactive_rpn();
    }
    // run with cmd line arguments
    else
    {
        program prog;
        string entry;
        int i;

        // make one string from entry
        for (i=1; i<argc; i++)
        {
            entry += argv[i];
            entry += ' ';
        }

        // make program
        ret = program::parse(entry.c_str(), prog);
        if (ret == ret_ok)
        {
            string separator = "";

            // user could stop prog with CtrlC
            catch_signals(&prog);

            // run it
            ret = prog.run(s_global_stack, s_global_heap);
            program::show_stack(s_global_stack, true);
        }
    }

    mpfr_free_cache();

    return ret;
}
