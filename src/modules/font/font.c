#include "selene.h"
#include "lua_helper.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "font8x8/font8x8_latin.h"

static MODULE_FUNCTION(font, GetDefault) {
    int w = 2048;
    int h = 8;
    uint8_t* bitmap = malloc(w * h * 4);
    int ox = 0;
    int stride = w * sizeof(int);
    for (int i = 0; i < 128; i++) {
        for (int y = 0; y < 8; y++) {
            uint8_t l = font8x8_basic[i][y];
            int offset = ox + (i * 8 * sizeof(int)) + (y * stride);
            for (int x = 0; x < 8; x++) {
                char p = (l >> x) & 0x1;

                uint8_t* pixel = bitmap + offset;
                pixel[0] = 255;
                pixel[1] = 255;
                pixel[2] = 255;
                pixel[3] = 255 * p;
                offset += 4;
            }
        }
    }
    ox = 128 * 8 * sizeof(int);
    for (int i = 0; i < 32; i++) {
        for (int y = 0; y < 8; y++) {
            uint8_t l = font8x8_control[i][y];
            int offset = ox + (i * 8 * sizeof(int)) + (y * stride);
            for (int x = 0; x < 8; x++) {
                char p = (l >> x) & 0x1;

                uint8_t* pixel = bitmap + offset;
                pixel[0] = 255;
                pixel[1] = 255;
                pixel[2] = 255;
                pixel[3] = 255 * p;
                offset += 4;
            }
        }
    }
    ox = 160 * 8 * sizeof(int);
    for (int i = 0; i < 96; i++) {
        for (int y = 0; y < 8; y++) {
            uint8_t l = font8x8_ext_latin[i][y];
            int offset = ox + (i * 8 * sizeof(int)) + (y * stride);
            for (int x = 0; x < 8; x++) {
                char p = (l >> x) & 0x1;

                uint8_t* pixel = bitmap + offset;
                pixel[0] = 255;
                pixel[1] = 255;
                pixel[2] = 255;
                pixel[3] = 255 * p;
                offset += 4;
            }
        }
    }
    NEW_UDATA(Data, dt);
    dt->size = w * h * 4;
    dt->data = bitmap;
    PUSH_INTEGER(w);
    PUSH_INTEGER(h);
    lua_newtable(L);
    for (int i = 0; i < 256; i++) {
        lua_newtable(L);
        lua_pushinteger(L, 8);
        lua_setfield(L, -2, "ax");
        lua_pushinteger(L, 0);
        lua_setfield(L, -2, "ay");
        lua_pushinteger(L, 8);
        lua_setfield(L, -2, "bh");
        lua_pushinteger(L, 8);
        lua_setfield(L, -2, "bl");
        lua_pushinteger(L, 8);
        lua_setfield(L, -2, "bt");
        lua_pushinteger(L, 8);
        lua_setfield(L, -2, "bw");
        lua_pushinteger(L, (8 * i));
        lua_setfield(L, -2, "tx");
        lua_rawseti(L, -2, i);
    }
    return 4;
}

static MODULE_FUNCTION(font, LoadTTF) {
    INIT_ARG();
    CHECK_STRING(path);
    OPT_INTEGER(font_size, 16);
    stbtt_fontinfo info;
#if defined(OS_WIN)
    FILE* fp;
    fopen_s(&fp, path, "rb");
#else
    FILE* fp = fopen(path, "rb");
#endif
    if (!fp)
        return luaL_error(L, "Failed to open font: %s", path);
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    void* data = malloc(size);
    if (!data)
        return luaL_error(L, "Failed to alloc memory for font");
    fread(data, size, 1, fp);

    fclose(fp);

    if (!stbtt_InitFont(&info, data, 0))
        return luaL_error(L, "Failed to init font data");

    int ascent, descent, line_gap;
    float fsize = (float)font_size;
    float scale = stbtt_ScaleForMappingEmToPixels(&info, fsize);
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    int baseline = (int)(ascent * scale);

    int tw, th;
    tw = th = 0;

    int i;
    struct {
        int ax, ay;
        int bl, bw;
        int bh, bt;
        int tx;
    } glyphs[256];

    for (i = 0; i < 256; i++) {
        int ax, bl;
        int x0, y0, x1, y1;
        int w, h;

        stbtt_GetCodepointHMetrics(&info, i, &ax, &bl);
        stbtt_GetCodepointBitmapBox(&info, i, scale, scale, &x0, &y0, &x1, &y1);

        w = x1 - x0;
        h = y1 - y0;

        glyphs[i].ax = (int)(ax * scale);
        glyphs[i].ay = 0;
        glyphs[i].bl = (int)(bl * scale);
        glyphs[i].bw = w;
        glyphs[i].bh = h;
        glyphs[i].bt = baseline + y0;

        tw += w;
        th = MAX(th, h);
    }

    int height = th;
    const int final_size = tw * th;

    NEW_UDATA(Data, dt);
    dt->size = final_size * sizeof(uint32_t);
    dt->data = malloc(dt->size);

    uint32_t* bitmap = dt->data;
    memset(bitmap, 0, dt->size);
    int x = 0;

    PUSH_INTEGER(tw);
    PUSH_INTEGER(th);

    lua_newtable(L);
    for (int i = 0; i < 256; i++) {
        int ww = glyphs[i].bw;
        int hh = glyphs[i].bh;
        int ssize = ww * hh;
        int ox, oy;

        uint8_t* bmp = stbtt_GetCodepointBitmap(&info, 0, scale, i, NULL, NULL, &ox, &oy);
        int xx, yy;
        xx = yy = 0;
        for (int j = 0; j < ssize; j++) {
            xx = (j % ww) + x;
            if (j != 0 && j % ww == 0)
                yy += tw;

            uint8_t* b = (uint8_t*)&bitmap[xx + yy];
            b[0] = 255;
            b[1] = 255;
            b[2] = 255;
            b[3] = bmp[j];
        }
        stbtt_FreeBitmap(bmp, info.userdata);

        glyphs[i].tx = x;

        x += glyphs[i].bw;

        lua_newtable(L);
        int p = 1;
        lua_pushinteger(L, glyphs[i].ax);
        lua_setfield(L, -2, "ax");
        lua_pushinteger(L, glyphs[i].ay);
        lua_setfield(L, -2, "ay");
        lua_pushinteger(L, glyphs[i].bh);
        lua_setfield(L, -2, "bh");
        lua_pushinteger(L, glyphs[i].bl);
        lua_setfield(L, -2, "bl");
        lua_pushinteger(L, glyphs[i].bt);
        lua_setfield(L, -2, "bt");
        lua_pushinteger(L, glyphs[i].bw);
        lua_setfield(L, -2, "bw");
        lua_pushinteger(L, glyphs[i].tx);
        lua_setfield(L, -2, "tx");
        lua_rawseti(L, -2, i);
    }
    free(data);
    return 4;
}

BEGIN_MODULE(font) {
    BEGIN_REG()
        REG_FIELD(font, GetDefault),
    END_REG()
    luaL_newlib(L, _reg);
    return 1;
}