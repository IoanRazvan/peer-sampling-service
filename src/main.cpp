#include "SamplingService.h"

int main()
{
    int c = 10, H = 0, S = 0 ;
    SamplingService s(c, H, S, std::chrono::milliseconds(100));
    s.init();
}