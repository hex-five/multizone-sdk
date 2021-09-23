/* Copyright(C) 2020 Hex Five Security, Inc. - All Rights Reserved */

#include <stddef.h>  // NULL

#define skip_leading_delim 1

static char *lasts = NULL;

char *strtok(char *s, const char *delim) { // @suppress("No return")

    char *spanp;
    int c, sc;
    char *tok;

    if (s == NULL && (s = lasts) == NULL)
        return (NULL);

    /*
     * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
     */
    cont: c = *s++;
    for (spanp = (char*) delim; (sc = *spanp++) != 0;) {
        if (c == sc) {
            if (skip_leading_delim) {
                goto cont;
            } else {
                lasts = s;
                s[-1] = 0;
                return (s - 1);
            }
        }
    }

    if (c == 0) { /* no non-delimiter characters */
        lasts = NULL;
        return (NULL);
    }
    tok = s - 1;

    /*
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for (;;) {
        c = *s++;
        spanp = (char*) delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                lasts = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */

}
