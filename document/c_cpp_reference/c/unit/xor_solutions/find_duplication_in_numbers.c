#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Build array 100 numbers:
// with 99 numbers differently and random another duplication.
int build_array_with_one_dup() {
    srand(time(0));
    int dup = rand() % 100 + 1;
    for (int i = 1; i <= 100; ++i) {
        if (i == dup) {
            printf("%d\n", i);
        }
        printf("%d\n", i);
    }
}

// Build randome  array and one duplicate
// gcc ./find_*.c -o ./build/xor_dup && ./build/xor_dup | shuf | paste -sd ','

int xs[] = {17, 26, 45, 43, 77, 98, 14, 10, 46, 27, 15, 42, 100, 2,  99, 55, 50, 51, 47, 13, 35, 80, 71, 24, 6,  88,
            23, 32, 63, 49, 92, 94, 96, 61, 70, 3,  93, 79, 5,   59, 19, 37, 21, 1,  83, 86, 84, 18, 16, 54, 65, 87,
            41, 33, 73, 89, 53, 76, 11, 12, 68, 62, 90, 7,  66,  4,  74, 29, 30, 44, 64, 36, 91, 52, 85, 20, 40, 97,
            9,  48, 67, 38, 81, 28, 82, 72, 58, 39, 69, 31, 41,  95, 8,  56, 22, 57, 34, 75, 25, 60, 78};

int find() {
    int x = 0;
    for (int i = 0; i <= 100; ++i) {
        x ^= i;
    }
    size_t n = sizeof(xs) / sizeof(xs[0]);
    for (int i = 0; i <= n; ++i) {
        x ^= xs[i];
    }
    printf("%d\n", x);
    return 0;
}

int main() {
    // build_array_with_one_dup();
    find();
    return 0;
}

// gcc ./find*.c -o ./build/xor_dup && ./build/xor_dup
