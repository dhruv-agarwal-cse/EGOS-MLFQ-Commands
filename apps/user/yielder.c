#include "egos.h"

int main() {
    while (1) {
        // Small CPU bursts – always shorter than a time slice
        for (volatile int i = 0; i < 30000; i++);
        // Immediately loops — voluntarily “gives up” CPU early
    }
    return 0;
}