#ifndef DEBUG_H
#define DEBUG_H

static void dump8(unsigned char* to_dump, unsigned long offset,
        unsigned long size)
{
    const int block_size = 1;
    const int block_per_line = 16;
    unsigned long max_line = size / block_size;
    unsigned char mychar;
    int i;
    int j;
    for (i = 0; i < max_line; i++)
    {
        if ((i % block_per_line) == 0)
        {
            if (i > 0)
            {
                printf(" ");
                for (j = i - block_per_line; j < i; j++)
                {
                    mychar = *(to_dump + j);
                    if ((mychar < 32) || (mychar >= 127))
                        mychar = '.';
                    printf("%c", mychar);
                }
            }
            printf("\n%08lX:", offset + i * block_size);
        }
        printf(" %02hhX", *(to_dump + i));
    }
    if (i > 0)
    {
        printf(" ");
        for (j = (i >= block_per_line) ? (i - block_per_line) : 0; j < i; j++)
        {
            mychar = *(to_dump + j);
            if ((mychar < 32) || (mychar >= 127))
                mychar = '.';
            printf("%c", mychar);
        }
    }
    printf("\n");
}

//
#define TRACE(...) do { printf(__VA_ARGS__); } while(0)

// chrono
#include <time.h>

static int chrono_next = 0;
static struct
{
    struct timespec ts_point;
    char comment[256];
} chrono_point[24];

static void chrono_start()
{
    clock_gettime(CLOCK_REALTIME, &chrono_point[0].ts_point);
    strcpy(chrono_point[0].comment, "START");
    chrono_next = 1;
}

static void chrono_add(const char* comment)
{
    clock_gettime(CLOCK_REALTIME, &chrono_point[chrono_next].ts_point);
    strcpy(chrono_point[chrono_next].comment, comment);
    chrono_next++;
}

static uint64_t chrono_diff_us(struct timespec* ts_from, struct timespec* ts_to)
{
    uint64_t from = (uint64_t)ts_from->tv_sec * 1000000UL + ((uint64_t)ts_from->tv_nsec) / 1000UL;
    uint64_t to = (uint64_t)ts_to->tv_sec * 1000000UL + ((uint64_t)ts_to->tv_nsec) / 1000UL;
    return to - from;
}

static void chrono_all_print(void)
{
    for(int i = 1; i < chrono_next; i++)
    {
        printf("CHRONO [%lu us] %s\n", (unsigned long)chrono_diff_us(&chrono_point[i-1].ts_point, &chrono_point[i].ts_point), chrono_point[i].comment);
    }
}

static void chrono_print(int chrono)
{
    if (chrono >= 1)
    {
        printf("CHRONO [%lu us] %s\n", (unsigned long)chrono_diff_us(&chrono_point[chrono-1].ts_point, &chrono_point[chrono].ts_point), chrono_point[chrono].comment);
    }
}

#define max(a, b) (((a)>(b))?(a):(b))

#endif
