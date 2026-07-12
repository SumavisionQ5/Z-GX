/*
 * n64_stub.h - stubs para compilar sin <ogc/n64.h>.
 * Los N64_BIT_* ya los define peripheral.h, asi que aqui NO se redefinen.
 * Solo se proveen el tipo N64Status, los N64_ERR_*, N64_BUTTON_* y N64_ReadAsync,
 * que no estan en peripheral.h. N64_ReadAsync siempre reporta "sin controlador".
 */
#ifndef N64_STUB_H
#define N64_STUB_H

#include <gctypes.h>

typedef struct {
    u32 err;
    s8  stickX;
    s8  stickY;
    u32 button;
} N64Status;

#ifndef N64_ERR_READY
#define N64_ERR_READY          0
#endif
#ifndef N64_ERR_NO_CONTROLLER
#define N64_ERR_NO_CONTROLLER  1
#endif

#ifndef N64_BUTTON_L
#define N64_BUTTON_L 0
#endif
#ifndef N64_BUTTON_Z
#define N64_BUTTON_Z 0
#endif

static inline void N64_ReadAsync(int chan, N64Status *st, void *cb) {
    (void)chan; (void)cb;
    if (st) st->err = N64_ERR_NO_CONTROLLER;
}

#endif /* N64_STUB_H */
