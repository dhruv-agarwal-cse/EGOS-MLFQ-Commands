#include "egos.h"

int main() {
    for (int i = 0; i < 7500000; i++) {
        if (i%10000 == 0) {
            my_printf("Yielder running: iteration %d\n", i);
        }
    }
    return 0;
}