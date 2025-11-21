#include "egos.h"

int main() {
    while (1) {
        // Very heavy CPU loop – consumes entire time slices
        for (volatile int i = 0; i < 2000000; i++);
    }
    return 0;
}