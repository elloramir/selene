#include "sdl2.h"

static MODULE_FUNCTION(GLContext, Create) {
    INIT_ARG();
    CHECK_UDATA(Window, win);
    NEW_UDATA(GLContext, ctx);
    *ctx = SDL_GL_CreateContext(*win);
    return 1;
}

static META_FUNCTION(GLContext, Delete) {
    CHECK_META(GLContext);
    SDL_GL_DeleteContext(*self);
    return 0;
}

MODULE_FUNCTION(GLContext, meta) {
    BEGIN_REG()
        REG_FIELD(GLContext, Create),
    END_REG()
    BEGIN_REG(_index)
        REG_META_FIELD(GLContext, Delete),
    END_REG()
    NEW_META(GLContext, _reg, _index_reg);
    return 1;
}
