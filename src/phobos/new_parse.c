#include "phobos.h"
#include "parse.h"

#define current_token(p) ((p)->tokens.at[(p)->current])
#define peek_token(p, n) ((p)->tokens.at[max((p)->current + (n), (p)->tokens.len)])
#define advance_token(p) if ((p)->current < (p)->tokens.len) (p)->current++
#define advance_n_tokens(p, n) do {\
        if ((p)->current + n < (p)->tokens.len) (p)->current += (n); else (p)->current = (p)->tokens.len - 1;\
    } while (0)

void expect_token(Parser* p, u8 type) {
    if (current_token(p).type == type) return;

    CRASH("");
}

