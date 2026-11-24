#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#pragma execution_character_set("utf-8")
#endif

#include "raylib.h"

#define W 1280
#define H 720
#define N 2
#define M 3
#define BW 420.0f
#define MW 360.0f
#define SX 570.0f
#define SW 260.0f
#define ION "assets/musicbutton.png"
#define ION1 "assets/soundbutton.png"
#define IOFF "assets/unmusicbutton.png"
#define IOFF1 "assets/unsoundbutton.png"
#define LOGO "assets/logo2.png"

enum { T_ST, T_SE, T_MU, T_FX, T_FS, T_LG, T_LN, T_BK, T_M1, T_M2, T_M3, T_N };

static const char* T[2][T_N] = {
    {"Bắt đầu", "Cài đặt", "Nhạc nền", "Nhạc hiệu ứng", "Toàn màn hình", "Ngôn ngữ", "Tiếng Việt", "Quay lại", "1 vs 1", "Chơi thường", "Chế độ khó"},
    {"Start", "Settings", "Music", "Sound effects", "Fullscreen", "Language", "English", "Back", "1 vs 1", "Normal", "Hard"}
};

typedef struct {
    Texture2D t[2];
    Rectangle r;
    float s;
} B;

static Font F;
static Texture2D si[4];
static Texture2D tl;
static int lg, mm, sm, fu;
static float mv = 0.6f, sv = 0.8f;

static int in(Rectangle r, Vector2 p) {
    return CheckCollisionPointRec(p, r);
}

static float cl(float x) {
    return x < 0 ? 0 : x > 1 ? 1 : x;
}

static void tri(Vector2 a, Vector2 b, Vector2 c, Color k) {
    DrawTriangle(a, b, c, k);
    DrawTriangle(a, c, b, k);
}

static void tx(const char* s, float x, float y, float z, Color c) {
    DrawTextEx(F, s, (Vector2) { x, y }, z, 1, c);
}

static void tc(const char* s, Rectangle r, float z, Color c) {
    Vector2 m = MeasureTextEx(F, s, z, 1);
    tx(s, r.x + (r.width - m.x) / 2, r.y + (r.height - m.y) / 2, z, c);
}

static void pn(Rectangle r, Color a, Color b) {
    DrawRectangleRounded((Rectangle) { r.x - 6, r.y - 6, r.width + 12, r.height + 12 }, 0.2f, 8, a);
    DrawRectangleRounded(r, 0.2f, 8, b);
}

static void bt(Rectangle r, const char* s, int o) {
    pn(r, (Color) { 120, 70, 30, 255 }, o ? (Color) { 250, 190, 100, 255 } : (Color) { 230, 160, 80, 255 });
    tc(s, r, 30, (Color) { 70, 35, 10, 255 });
}

static void sk(Rectangle r, int f, int m, int o) {
    Texture2D t = si[f * 2 + m];
    pn(r, (Color) { 120, 70, 30, 255 }, o ? (Color) { 250, 190, 100, 255 } : (Color) { 230, 160, 80, 255 });
    if (t.id) {
        float e = r.width * 0.14f;
        float a = (r.width - 2 * e) / t.width, b = (r.height - 2 * e) / t.height;
        float s = (a < b ? a : b) * (o ? 1.08f : 1.0f);
        float w = t.width * s, h = t.height * s;
        DrawTexturePro(t, (Rectangle) { 0, 0, (float)t.width, (float)t.height }, (Rectangle) { r.x + (r.width - w) / 2, r.y + (r.height - h) / 2, w, h }, (Vector2) { 0, 0 }, 0, WHITE);
        return;
    }
    Color k = (Color){ 70, 35, 10, 255 };
    float x = r.x + (r.width - 50) / 2, y = r.y + (r.height - 50) / 2;
    DrawRectangleRec((Rectangle) { x + 11, y + 19, 9, 12 }, k);
    tri((Vector2) { x + 20, y + 19 }, (Vector2) { x + 33, y + 10 }, (Vector2) { x + 33, y + 40 }, k);
    tri((Vector2) { x + 20, y + 19 }, (Vector2) { x + 33, y + 40 }, (Vector2) { x + 20, y + 31 }, k);
    if (!m) {
        DrawLineEx((Vector2) { x + 37, y + 19 }, (Vector2) { x + 41, y + 25 }, 3, k);
        DrawLineEx((Vector2) { x + 41, y + 25 }, (Vector2) { x + 37, y + 31 }, 3, k);
        DrawLineEx((Vector2) { x + 42, y + 14 }, (Vector2) { x + 47, y + 25 }, 3, k);
        DrawLineEx((Vector2) { x + 47, y + 25 }, (Vector2) { x + 42, y + 36 }, 3, k);
    }
    else {
        DrawLineEx((Vector2) { x + 36, y + 17 }, (Vector2) { x + 47, y + 33 }, 4, RED);
        DrawLineEx((Vector2) { x + 47, y + 17 }, (Vector2) { x + 36, y + 33 }, 4, RED);
    }
}

static void sl(float cy, float v, int m) {
    float kx = SX + SW * v;
    DrawRectangleRounded((Rectangle) { SX, cy - 6, SW, 12 }, 1.0f, 6, (Color) { 200, 160, 110, 255 });
    if (v > 0.03f) DrawRectangleRounded((Rectangle) { SX, cy - 6, SW* v, 12 }, 1.0f, 6, m ? (Color) { 150, 150, 150, 255 } : (Color) { 90, 170, 70, 255 });
    DrawCircle((int)kx, (int)cy, 15, (Color) { 120, 70, 30, 255 });
    DrawCircle((int)kx, (int)cy, 11, m ? (Color) { 190, 190, 190, 255 } : (Color) { 255, 214, 80, 255 });
}

static void ld(B* b, const char* a, const char* e, float w, float y) {
    b->t[0] = LoadTexture(a);
    b->t[1] = e ? LoadTexture(e) : b->t[0];
    if (b->t[0].id) SetTextureFilter(b->t[0], TEXTURE_FILTER_BILINEAR);
    if (e && b->t[1].id) SetTextureFilter(b->t[1], TEXTURE_FILTER_BILINEAR);
    float h = b->t[0].id ? w * b->t[0].height / b->t[0].width : w / 4;
    b->r = (Rectangle){ (W - w) / 2, y, w, h };
    b->s = 1.0f;
}

static void ul(B* b) {
    UnloadTexture(b->t[0]);
    if (b->t[1].id != b->t[0].id) UnloadTexture(b->t[1]);
}

static void dr(B* b, int o, const char* s) {
    float w = b->r.width * b->s;
    float h = b->r.height * b->s;
    float x = b->r.x + (b->r.width - w) / 2;
    float y = b->r.y + (b->r.height - h) / 2;
    Texture2D t = b->t[lg];
    if (t.id) {
        Color c = o ? WHITE : (Color) { 215, 215, 215, 255 };
        DrawTexturePro(t, (Rectangle) { 0, 0, (float)t.width, (float)t.height }, (Rectangle) { x, y, w, h }, (Vector2) { 0, 0 }, 0, c);
    }
    else {
        pn((Rectangle) { x, y, w, h }, (Color) { 120, 70, 30, 255 }, o ? (Color) { 250, 190, 100, 255 } : (Color) { 230, 160, 80, 255 });
        tc(s, (Rectangle) { x, y, w, h }, 40 * b->s, (Color) { 70, 35, 10, 255 });
    }
}

int main(void) {
    InitWindow(W, H, "Stardew Caro");
    InitAudioDevice();
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    int cp[512], nc = 0;
    for (int i = 32; i < 127; i++) cp[nc++] = i;
    for (int i = 0xC0; i <= 0x17F; i++) cp[nc++] = i;
    cp[nc++] = 0x1A0;
    cp[nc++] = 0x1A1;
    cp[nc++] = 0x1AF;
    cp[nc++] = 0x1B0;
    for (int i = 0x1EA0; i <= 0x1EF9; i++) cp[nc++] = i;

    const char* ff[3] = { "assets/unifont.otf", "C:/Windows/Fonts/arialbd.ttf", "C:/Windows/Fonts/arial.ttf" };
    int hf = 0;
    F = GetFontDefault();
    for (int i = 0; i < 3 && !hf; i++) {
        if (FileExists(ff[i])) {
            F = LoadFontEx(ff[i], 40, cp, nc);
            SetTextureFilter(F.texture, TEXTURE_FILTER_BILINEAR);
            hf = 1;
        }
    }

    const char* fm[3] = { "assets/bg_music.mp3", "assets/bgm.ogg", "assets/bgm.wav" };
    const char* fk[3] = { "assets/click.wav", "assets/click.ogg", "assets/click.mp3" };
    Music mu = { 0 };
    Sound ck = { 0 };
    int hm = 0, hs = 0;
    for (int i = 0; i < 3 && !hm; i++) {
        if (FileExists(fm[i])) {
            mu = LoadMusicStream(fm[i]);
            if (mu.stream.buffer) {
                hm = 1;
                mu.looping = true;
                PlayMusicStream(mu);
            }
        }
    }
    for (int i = 0; i < 3 && !hs; i++) {
        if (FileExists(fk[i])) {
            ck = LoadSound(fk[i]);
            if (ck.stream.buffer) hs = 1;
        }
    }

    Texture2D bg = LoadTexture("assets/LobbyTheme.png");
    tl = LoadTexture(LOGO);
    if (tl.id) SetTextureFilter(tl, TEXTURE_FILTER_BILINEAR);
    RenderTexture2D rt = LoadRenderTexture(W, H);
    SetTextureFilter(rt.texture, TEXTURE_FILTER_BILINEAR);

    si[0] = LoadTexture(ION);
    si[1] = LoadTexture(IOFF);
    si[2] = LoadTexture(ION1);
    si[3] = LoadTexture(IOFF1);
    for (int i = 0; i < 4; i++)
        if (si[i].id) SetTextureFilter(si[i], TEXTURE_FILTER_BILINEAR);

    const char* fv[N] = { "assets/Batdaubutton.png", "assets/Caidatbutton.png" };
    const char* fe[N] = { "assets/Startbutton.png", "assets/Settingsbutton.png" };
    const char* mv0[M] = { "assets/1vs1button.png", "assets/choitthuongbutton.png", "assets/Chedokhobutton.png" };
    const char* me0[M] = { 0, "assets/normalmodebutton.png", "assets/asianmodebutton.png" };

    B b[N], mb[M];
    for (int i = 0; i < N; i++) ld(&b[i], fv[i], fe[i], BW, 330.0f + i * 130.0f);
    for (int i = 0; i < M; i++) ld(&mb[i], mv0[i], me0[i], MW, 265.0f + i * 100.0f);

    Rectangle mr[2] = { {870, 192, 56, 56}, {870, 282, 56, 56} };
    Rectangle hr[2] = { {SX - 14, 200, SW + 28, 40}, {SX - 14, 290, SW + 28, 40} };
    Rectangle cb = { SX, 378, 44, 44 };
    Rectangle lb = { SX, 462, 350, 56 };
    Rectangle bk = { 530, 552, 220, 56 };
    Rectangle bk2 = { 530, 620, 220, 56 };
    Rectangle pr = { 290, 80, 700, 560 };
    Rectangle lm = { W - 108, 24, 72, 72 };

    int st = 0, k = 0, k2 = 0, gm = 0, q = 0, dg = 0;
    Color br = (Color){ 70, 35, 10, 255 };

    while (!q) {
        if (WindowShouldClose()) q = 1;

        int sw = GetScreenWidth(), sh = GetScreenHeight();
        float sc = (float)sw / W < (float)sh / H ? (float)sw / W : (float)sh / H;
        float ox = (sw - W * sc) / 2, oy = (sh - H * sc) / 2;
        Vector2 m = GetMousePosition();
        Vector2 p = { (m.x - ox) / sc, (m.y - oy) / sc };
        Vector2 d = GetMouseDelta();
        int lc = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        fu = IsWindowFullscreen();

        if (hm) {
            UpdateMusicStream(mu);
            SetMusicVolume(mu, mm ? 0.0f : mv);
        }
        if (hs) SetSoundVolume(ck, sm ? 0.0f : sv);

        if (st < 2 && lc && in(lm, p)) {
            mm = !mm;
            if (hs) PlaySound(ck);
            lc = 0;
        }

        if (st == 0) {
            int c = 0;
            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) k = (k + N - 1) % N;
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) k = (k + 1) % N;
            if (d.x != 0 || d.y != 0)
                for (int i = 0; i < N; i++)
                    if (in(b[i].r, p)) k = i;
            if (IsKeyPressed(KEY_ENTER)) c = 1;
            if (lc && in(b[k].r, p)) c = 1;
            for (int i = 0; i < N; i++) {
                float g = i == k ? 1.08f : 1.0f;
                b[i].s += (g - b[i].s) * 0.2f;
            }
            if (c) {
                if (hs) PlaySound(ck);
                st = k + 1;
                k2 = 0;
            }
        }
        else if (st == 1) {
            int c = 0;
            if (IsKeyPressed(KEY_ESCAPE)) st = 0;
            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) k2 = (k2 + M) % (M + 1);
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) k2 = (k2 + 1) % (M + 1);
            if (d.x != 0 || d.y != 0) {
                for (int i = 0; i < M; i++)
                    if (in(mb[i].r, p)) k2 = i;
                if (in(bk2, p)) k2 = M;
            }
            Rectangle cr = k2 < M ? mb[k2].r : bk2;
            if (IsKeyPressed(KEY_ENTER)) c = 1;
            if (lc && in(cr, p)) c = 1;
            for (int i = 0; i < M; i++) {
                float g = i == k2 ? 1.08f : 1.0f;
                mb[i].s += (g - mb[i].s) * 0.2f;
            }
            if (c && st == 1) {
                if (hs) PlaySound(ck);
                if (k2 == M) st = 0;
                else {
                    gm = k2;
                    st = 3;
                }
            }
        }
        else if (st == 3) {
            if (IsKeyPressed(KEY_ESCAPE)) st = 1;
        }
        else {
            if (IsKeyPressed(KEY_ESCAPE)) st = 0;
            for (int i = 0; i < 2; i++)
                if (lc && in(hr[i], p)) dg = i + 1;
            if (dg) {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    float v = cl((p.x - SX) / SW);
                    if (dg == 1) mv = v;
                    else sv = v;
                }
                else {
                    if (dg == 2 && hs) PlaySound(ck);
                    dg = 0;
                }
            }
            for (int i = 0; i < 2; i++) {
                if (lc && in(mr[i], p)) {
                    if (i == 0) mm = !mm;
                    else sm = !sm;
                    if (hs) PlaySound(ck);
                }
            }
            if (lc && in(cb, p)) {
                ToggleFullscreen();
                if (hs) PlaySound(ck);
            }
            if (lc && in(lb, p)) {
                lg = !lg;
                if (hs) PlaySound(ck);
            }
            if (lc && in(bk, p)) {
                st = 0;
                if (hs) PlaySound(ck);
            }
        }

        BeginTextureMode(rt);
        ClearBackground(SKYBLUE);
        if (bg.id) DrawTexturePro(bg, (Rectangle) { 0, 0, (float)bg.width, (float)bg.height }, (Rectangle) { 0, 0, W, H }, (Vector2) { 0, 0 }, 0, WHITE);
        else DrawRectangle(0, 520, W, H - 520, (Color) { 90, 170, 70, 255 });

        if (st == 3) {
            tc(T[lg][T_M1 + gm], (Rectangle) { 0, 0, W, H }, 70, WHITE);
            tx("ESC", 20, 20, 32, WHITE);
        }
        else {
            if (tl.id) {
                float a = 790.0f / tl.width, b = 230.0f / tl.height;
                float s = a < b ? a : b;
                float w = tl.width * s, h = tl.height * s;
                DrawTexturePro(tl, (Rectangle) { 0, 0, (float)tl.width, (float)tl.height }, (Rectangle) { (W - w) / 2, 40 + (170 - h) / 2, w, h }, (Vector2) { 0, 0 }, 0, WHITE);
            }
            if (st == 1) {
                for (int i = 0; i < M; i++) dr(&mb[i], i == k2, T[lg][T_M1 + i]);
                bt(bk2, T[lg][T_BK], k2 == M);
            }
            else {
                for (int i = 0; i < N; i++) dr(&b[i], st == 0 && i == k, T[lg][i]);
            }
            if (st < 2) sk(lm, 0, mm, in(lm, p));
        }

        if (st == 2) {
            DrawRectangle(0, 0, W, H, (Color) { 0, 0, 0, 130 });
            pn(pr, (Color) { 120, 70, 30, 255 }, (Color) { 255, 236, 200, 255 });
            tc(T[lg][T_SE], (Rectangle) { pr.x, 100, pr.width, 60 }, 48, br);
            for (int i = 0; i < 2; i++) {
                float cy = 220.0f + i * 90.0f;
                tx(T[lg][T_MU + i], 330, cy - 18, 32, br);
                sl(cy, i ? sv : mv, i ? sm : mm);
                sk(mr[i], i, i ? sm : mm, in(mr[i], p));
            }
            tx(T[lg][T_FS], 330, 400 - 18, 32, br);
            pn(cb, (Color) { 120, 70, 30, 255 }, (Color) { 255, 250, 235, 255 });
            if (fu) {
                DrawLineEx((Vector2) { cb.x + 9, cb.y + 23 }, (Vector2) { cb.x + 18, cb.y + 32 }, 5, (Color) { 70, 150, 50, 255 });
                DrawLineEx((Vector2) { cb.x + 18, cb.y + 32 }, (Vector2) { cb.x + 36, cb.y + 11 }, 5, (Color) { 70, 150, 50, 255 });
            }
            tx(T[lg][T_LG], 330, 490 - 18, 32, br);
            bt(lb, T[lg][T_LN], in(lb, p));
            bt(bk, T[lg][T_BK], in(bk, p));
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(rt.texture, (Rectangle) { 0, 0, (float)W, -(float)H }, (Rectangle) { ox, oy, W* sc, H* sc }, (Vector2) { 0, 0 }, 0, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(rt);
    UnloadTexture(bg);
    UnloadTexture(tl);
    for (int i = 0; i < N; i++) ul(&b[i]);
    for (int i = 0; i < M; i++) ul(&mb[i]);
    for (int i = 0; i < 4; i++) UnloadTexture(si[i]);
    if (hm) UnloadMusicStream(mu);
    if (hs) UnloadSound(ck);
    if (hf) UnloadFont(F);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}