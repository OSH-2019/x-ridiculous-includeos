#include "move.h"
#include "printf.h"

extern int _end;
extern int _ELF_START_;

int move(char *dest, char *src, int size) {
    printf("[ MOVE ] dest=%x, src=%x, size=%x\n", dest, src, size);
    printf("[ MOVE ] ELF_START=%x, _end=%x\n", &_ELF_START_, &_end);
    // Check for potential overlap
    if ( dest <= ((char *)&_end) && dest >= ((char *)&_ELF_START_) ) {
        printf("Overlap detected. Dest=%x, Src=%x, Size=%x, given ELF_START=%x, _end=%x\n",
            dest, src, size, &_ELF_START_, &_end);
        return -1;
    }
    int i;
    for (i = 0; i < size; i++) {
        dest[i] = src[i];
    }
    return i;
}