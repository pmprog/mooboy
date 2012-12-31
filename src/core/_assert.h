#ifndef ASSERT_H
#define ASSERT_H

    #include <stdio.h>
    #include <stdlib.h>
    #include <assert.h>
    #include "util/err.h"

    #define assert_unsupported(expr, msg) if(!(expr)) {err_set(ERR_ROM_UNSUPPORTED); fprintf(stderr, "%s\n", (msg)); exit(1); }
    #define assert_corrupt(expr, msg) if(!(expr)) {err_set(ERR_ROM_CORRUPT); fprintf(stderr, "%s\n", (msg)); exit(1); }
    #define assert_illegal_read(expr, msg) if(!(expr)) {err_set(ERR_ILLEGAL_READ); fprintf(stderr, "%s\n", (msg)); exit(1); }
    #define assert_illegal_write(expr, msg) if(!(expr)) {err_set(ERR_ILLEGAL_WRITE); fprintf(stderr, "%s\n", (msg)); exit(1); }

#endif // ASSERT_H