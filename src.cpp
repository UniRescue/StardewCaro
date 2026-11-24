#define _CRT_SECURE_NO_WARNINGS
#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#pragma execution_character_set("unicode")
#endif

#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

#define MAX_BOARD 16
#define MAX_HP 10

static Rectangle RC(float x, float y, float w, float h) {
    Rectangle r;
    r.x = x; r.y = y; r.width = w; r.height = h;
    return r;
}

static Vector2 V2(float x, float y) {
    Vector2 v;
    v.x = x; v.y = y;
    return v;
}

static Color CO(int r, int g, int b, int a) {
    Color c;
    c.r = (unsigned char)r;
    c.g = (unsigned char)g;
    c.b = (unsigned char)b;
    c.a = (unsigned char)a;
    return c;
}

enum { T_ST, T_SE, T_MU, T_FX, T_FS, T_LG, T_LN, T_BK, T_M1, T_M2, T_M3, T_N };

static const char* T[2][T_N] = {
    {"Bắt đầu", "Cài đặt", "Nhạc nền", "Nhạc hiệu ứng", "Toàn màn hình", "Ngôn ngữ", "Tiếng Việt", "Quay lại", "1 vs 1", "Bot thường", "Asian Mode"},
    {"Start", "Settings", "Music", "Sound effects", "Fullscreen", "Language", "English", "Back", "1 vs 1", "Normal Bot", "Asian Mode"}
};

typedef struct {
    Texture2D t[2];
    Rectangle r;
    float s;
} B;

typedef struct {
    int mode;
    int size;
    int board[MAX_BOARD][MAX_BOARD];
    int turn;
    int hp[3];
    int items[3][3];
    int activeItem;
    float turnTimer;
    float maxTurnTime;
    float aiThinkTimer;
    int lastR, lastC;
    int paused;
    int gameOver;
    int winner;
    char winReason[128];
    int winCells[5][2];
    int winCount;

    char toast[128];
    float toastTimer;
    Color toastColor;

    char pName[3][64];
    int activeInput;
} GameState;

typedef struct {
    int r, c;
    int score;
} MoveCand;

static Font F;
static Texture2D si[4];
static Texture2D tl;
static Texture2D leafTex;
static int lg = 0, mm = 0, sm = 0, fu = 0;
static float mv = 0.6f, sv = 0.8f;
static GameState G;

static const int DR[4] = { 0, 1, 1, 1 };
static const int DC[4] = { 1, 0, 1, -1 };

static const char* L(const char* vi, const char* en) {
    return lg ? en : vi;
}

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
    DrawTextEx(F, s, V2(x, y), z, 1, c);
}

static void tc(const char* s, Rectangle r, float z, Color c) {
    Vector2 m = MeasureTextEx(F, s, z, 1);
    tx(s, r.x + (r.width - m.x) / 2, r.y + (r.height - m.y) / 2, z, c);
}

static Texture2D createLeafTexture(void) {
    const char* paths[2] = { "assets/leaf.png", "assets/la.png" };
    for (int i = 0; i < 2; i++) {
        if (FileExists(paths[i])) {
            Texture2D t = LoadTexture(paths[i]);
            if (t.id) {
                SetTextureFilter(t, TEXTURE_FILTER_POINT);
                return t;
            }
        }
    }

    static const char* px[16] = {
        "................",
        "..11............",
        "...11.111.......",
        "....1122211.....",
        "...122332221....",
        "..12234322221...",
        "..12332432221...",
        "..12222243221...",
        "...1222224321...",
        "...1222222431...",
        "....122222341...",
        ".....12222231...",
        "......1222211...",
        ".......11111....",
        "................",
        "................"
    };

    Image img = GenImageColor(16, 16, BLANK);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            char c = px[y][x];
            if (c == '1') ImageDrawPixel(&img, x, y, CO(20, 72, 16, 255));
            else if (c == '2') ImageDrawPixel(&img, x, y, CO(38, 132, 26, 255));
            else if (c == '3') ImageDrawPixel(&img, x, y, CO(68, 172, 42, 255));
            else if (c == '4') ImageDrawPixel(&img, x, y, CO(115, 208, 65, 255));
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return tex;
}

static void drawLeafTex(float x, float y, float size, int flipX, int flipY) {
    if (!leafTex.id) return;
    Rectangle src = RC(
        0, 0,
        flipX ? -(float)leafTex.width : (float)leafTex.width,
        flipY ? -(float)leafTex.height : (float)leafTex.height
    );
    Rectangle dst = RC(x, y, size, size);
    DrawTexturePro(leafTex, src, dst, V2(0, 0), 0.0f, WHITE);
}

static void pn(Rectangle r, Color a, Color b) {
    (void)a;
    float pad = (r.height < 70.0f) ? 6.0f : 9.0f;
    float rd = (r.height < 70.0f) ? 0.28f : 0.08f;

    Color outBlk = CO(25, 14, 8, 255);
    Color wood = CO(247, 177, 118, 255);
    Color grain = CO(214, 142, 86, 255);
    Color innerBg = (b.r > 240 && b.g > 235) ? b : CO(250, 216, 180, 255);
    Color innerSh = CO(232, 182, 134, 255);

    DrawRectangleRounded(RC(r.x - pad - 2.5f, r.y - pad - 2.5f, r.width + (pad + 2.5f) * 2, r.height + (pad + 2.5f) * 2), rd, 8, outBlk);
    DrawRectangleRounded(RC(r.x - pad, r.y - pad, r.width + pad * 2, r.height + pad * 2), rd, 8, wood);

    int grains = (int)(r.width / 70.0f);
    if (grains < 2) grains = 2;
    for (int i = 0; i < grains; i++) {
        float gx = r.x + 12.0f + i * (r.width - 24.0f) / grains;
        DrawLineEx(V2(gx, r.y - pad * 0.5f), V2(gx + 22.0f, r.y - pad * 0.5f), 2.0f, grain);
        DrawLineEx(V2(gx + 10.0f, r.y + r.height + pad * 0.5f), V2(gx + 34.0f, r.y + r.height + pad * 0.5f), 2.0f, grain);
    }

    DrawRectangleRounded(RC(r.x - 2.0f, r.y - 2.0f, r.width + 4.0f, r.height + 4.0f), rd * 0.7f, 6, outBlk);
    DrawRectangleRounded(r, rd * 0.7f, 6, innerSh);
    DrawRectangleRounded(RC(r.x + 2.0f, r.y + 2.0f, r.width - 4.0f, r.height - 4.0f), rd * 0.6f, 6, innerBg);
}

static void btz(Rectangle r, const char* s, int o, float fz) {
    Color inner = o ? CO(255, 232, 200, 255) : CO(250, 216, 180, 255);
    pn(r, CO(25, 14, 8, 255), inner);

    if (r.width >= 110.0f && r.height >= 38.0f) {
        float ls = (r.height < 48.0f) ? 20.0f : 26.0f;
        drawLeafTex(r.x + 2.0f, r.y + 1.0f, ls, 0, 0);
        drawLeafTex(r.x + r.width - ls - 2.0f, r.y + 1.0f, ls, 1, 0);
        if (r.height >= 52.0f) {
            drawLeafTex(r.x + r.width - ls - 4.0f, r.y + r.height - ls - 1.0f, ls * 0.9f, 1, 1);
        }
    }

    Vector2 m = MeasureTextEx(F, s, fz, 1);
    float txX = r.x + (r.width - m.x) / 2.0f;
    float txY = r.y + (r.height - m.y) / 2.0f;
    tx(s, txX + 2.0f, txY + 2.0f, fz, CO(115, 48, 15, 255));
    tx(s, txX, txY, fz, o ? CO(235, 120, 45, 255) : CO(214, 102, 34, 255));
}

static void bt(Rectangle r, const char* s, int o) {
    btz(r, s, o, 30.0f);
}

static void appendUtf8(char* buf, int maxBytes, int cp) {
    int len = (int)strlen(buf);
    char tmp[5] = { 0 };
    int n = 0;
    if (cp < 0x80) {
        tmp[0] = (char)cp; n = 1;
    }
    else if (cp < 0x800) {
        tmp[0] = (char)(0xC0 | (cp >> 6));
        tmp[1] = (char)(0x80 | (cp & 0x3F)); n = 2;
    }
    else if (cp < 0x10000) {
        tmp[0] = (char)(0xE0 | (cp >> 12));
        tmp[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        tmp[2] = (char)(0x80 | (cp & 0x3F)); n = 3;
    }
    else {
        tmp[0] = (char)(0xF0 | (cp >> 18));
        tmp[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        tmp[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        tmp[3] = (char)(0x80 | (cp & 0x3F)); n = 4;
    }
    if (len + n < maxBytes - 1) {
        memcpy(buf + len, tmp, (size_t)n);
        buf[len + n] = '\0';
    }
}

static void popUtf8(char* buf) {
    int len = (int)strlen(buf);
    if (len == 0) return;
    int i = len - 1;
    while (i > 0 && ((unsigned char)buf[i] & 0xC0) == 0x80) i--;
    buf[i] = '\0';
}

static void drawHeart(float cx, float cy, float r, int filled) {
    Color border = CO(90, 25, 25, 255);
    Color fill = filled ? CO(225, 45, 55, 255) : CO(195, 175, 150, 255);

    float rb = r * 1.15f;
    DrawCircleV(V2(cx - rb * 0.44f, cy - rb * 0.18f), rb * 0.52f, border);
    DrawCircleV(V2(cx + rb * 0.44f, cy - rb * 0.18f), rb * 0.52f, border);
    tri(V2(cx - rb * 0.94f, cy - rb * 0.05f), V2(cx + rb * 0.94f, cy - rb * 0.05f), V2(cx, cy + rb * 0.96f), border);

    DrawCircleV(V2(cx - r * 0.44f, cy - r * 0.18f), r * 0.50f, fill);
    DrawCircleV(V2(cx + r * 0.44f, cy - r * 0.18f), r * 0.50f, fill);
    tri(V2(cx - r * 0.91f, cy - r * 0.05f), V2(cx + r * 0.91f, cy - r * 0.05f), V2(cx, cy + r * 0.92f), fill);

    if (filled) {
        DrawCircleV(V2(cx - r * 0.45f, cy - r * 0.30f), r * 0.16f, CO(255, 175, 180, 255));
    }
}

static void drawItemIcon(int type, float cx, float cy) {
    if (type == 0) {
        DrawLineEx(V2(cx - 10, cy + 10), V2(cx + 6, cy - 6), 5, CO(120, 70, 30, 255));
        DrawRectanglePro(RC(cx + 3, cy - 5, 16, 9), V2(8, 4.5f), -45.0f, CO(110, 115, 125, 255));
        DrawRectanglePro(RC(cx + 3, cy - 5, 14, 5), V2(7, 2.5f), -45.0f, CO(170, 175, 185, 255));
    }
    else if (type == 1) {
        DrawCircleV(V2(cx, cy), 12, CO(120, 70, 30, 255));
        DrawCircleV(V2(cx, cy), 9.5f, CO(255, 245, 215, 255));
        DrawLineEx(V2(cx, cy), V2(cx, cy - 6), 2.5f, CO(180, 40, 40, 255));
        DrawLineEx(V2(cx, cy), V2(cx + 5, cy + 2), 2.5f, CO(70, 35, 10, 255));
    }
    else {
        drawHeart(cx, cy, 10.5f, 1);
    }
}

static void sk(Rectangle r, int f, int m, int o) {
    Texture2D t = si[f * 2 + m];
    pn(r, CO(25, 14, 8, 255), o ? CO(255, 232, 200, 255) : CO(250, 216, 180, 255));
    if (t.id) {
        float e = r.width * 0.14f;
        float a = (r.width - 2 * e) / t.width, b = (r.height - 2 * e) / t.height;
        float s = (a < b ? a : b) * (o ? 1.08f : 1.0f);
        float w = t.width * s, h = t.height * s;
        DrawTexturePro(t, RC(0, 0, (float)t.width, (float)t.height), RC(r.x + (r.width - w) / 2, r.y + (r.height - h) / 2, w, h), V2(0, 0), 0, WHITE);
        return;
    }
    Color k = CO(70, 35, 10, 255);
    float x = r.x + (r.width - 50) / 2, y = r.y + (r.height - 50) / 2;
    DrawRectangleRec(RC(x + 11, y + 19, 9, 12), k);
    tri(V2(x + 20, y + 19), V2(x + 33, y + 10), V2(x + 33, y + 40), k);
    tri(V2(x + 20, y + 19), V2(x + 33, y + 40), V2(x + 20, y + 31), k);
    if (!m) {
        DrawLineEx(V2(x + 37, y + 19), V2(x + 41, y + 25), 3, k);
        DrawLineEx(V2(x + 41, y + 25), V2(x + 37, y + 31), 3, k);
        DrawLineEx(V2(x + 42, y + 14), V2(x + 47, y + 25), 3, k);
        DrawLineEx(V2(x + 47, y + 25), V2(x + 42, y + 36), 3, k);
    }
    else {
        DrawLineEx(V2(x + 36, y + 17), V2(x + 47, y + 33), 4, RED);
        DrawLineEx(V2(x + 47, y + 17), V2(x + 36, y + 33), 4, RED);
    }
}

static void sl(float cy, float v, int m) {
    float kx = SX + SW * v;
    DrawRectangleRounded(RC(SX, cy - 6, SW, 12), 1.0f, 6, CO(200, 160, 110, 255));
    if (v > 0.03f) DrawRectangleRounded(RC(SX, cy - 6, SW * v, 12), 1.0f, 6, m ? CO(150, 150, 150, 255) : CO(90, 170, 70, 255));
    DrawCircle((int)kx, (int)cy, 15, CO(120, 70, 30, 255));
    DrawCircle((int)kx, (int)cy, 11, m ? CO(190, 190, 190, 255) : CO(255, 214, 80, 255));
}

static void ld(B* b, const char* a, const char* e, float w, float y) {
    b->t[0] = LoadTexture(a);
    b->t[1] = e ? LoadTexture(e) : b->t[0];
    if (b->t[0].id) SetTextureFilter(b->t[0], TEXTURE_FILTER_BILINEAR);
    if (e && b->t[1].id) SetTextureFilter(b->t[1], TEXTURE_FILTER_BILINEAR);
    float h = b->t[0].id ? w * b->t[0].height / b->t[0].width : w / 4;
    b->r = RC((W - w) / 2, y, w, h);
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
        Color c = o ? WHITE : CO(215, 215, 215, 255);
        DrawTexturePro(t, RC(0, 0, (float)t.width, (float)t.height), RC(x, y, w, h), V2(0, 0), 0, c);
    }
    else {
        btz(RC(x, y, w, h), s, o, 40.0f * b->s);
    }
}

static void syncDefaultNames(void) {
    if (strcmp(G.pName[1], "Người chơi 1") == 0 || strcmp(G.pName[1], "Player 1") == 0)
        strcpy(G.pName[1], L("Người chơi 1", "Player 1"));
    if (strcmp(G.pName[2], "Người chơi 2") == 0 || strcmp(G.pName[2], "Player 2") == 0)
        strcpy(G.pName[2], L("Người chơi 2", "Player 2"));
    if (strcmp(G.pName[1], "Bạn") == 0 || strcmp(G.pName[1], "You") == 0)
        strcpy(G.pName[1], L("Bạn", "You"));
    if (strcmp(G.pName[2], "Bot Thường") == 0 || strcmp(G.pName[2], "Normal Bot") == 0)
        strcpy(G.pName[2], L("Bot Thường", "Normal Bot"));
}

static void setToast(const char* msg, Color col) {
    strncpy(G.toast, msg, sizeof(G.toast) - 1);
    G.toast[sizeof(G.toast) - 1] = '\0';
    G.toastTimer = 2.6f;
    G.toastColor = col;
}

static void startMatch(int mode) {
    G.mode = mode;
    G.size = (mode == 2) ? 16 : 10;
    memset(G.board, 0, sizeof(G.board));
    G.turn = 1;
    G.hp[1] = MAX_HP;
    G.hp[2] = MAX_HP;

    for (int p = 1; p <= 2; p++) {
        G.items[p][0] = 2;
        G.items[p][1] = 2;
        G.items[p][2] = 1;
    }

    G.activeItem = -1;
    G.maxTurnTime = (mode == 2) ? 12.0f : 15.0f;
    G.turnTimer = G.maxTurnTime;
    G.aiThinkTimer = 0.0f;
    G.lastR = -1;
    G.lastC = -1;
    G.paused = 0;
    G.gameOver = 0;
    G.winner = 0;
    G.winReason[0] = '\0';
    G.winCount = 0;
    G.toast[0] = '\0';
    G.toastTimer = 0.0f;

    if (mode == 0) {
        if (strlen(G.pName[1]) == 0) strcpy(G.pName[1], L("Người chơi 1", "Player 1"));
        if (strlen(G.pName[2]) == 0) strcpy(G.pName[2], L("Người chơi 2", "Player 2"));
        syncDefaultNames();
    }
    else if (mode == 1) {
        strcpy(G.pName[1], L("Bạn", "You"));
        strcpy(G.pName[2], L("Bot Thường", "Normal Bot"));
    }
    else {
        strcpy(G.pName[1], L("Bạn", "You"));
        strcpy(G.pName[2], "Asian AI");
    }
}

static int inBoard(int r, int c) {
    return r >= 0 && r < G.size && c >= 0 && c < G.size;
}

static int checkWinAt(int r, int c, int who, int recordCells) {
    for (int d = 0; d < 4; d++) {
        int cells[32][2];
        int cnt = 0;
        cells[cnt][0] = r; cells[cnt][1] = c; cnt++;

        for (int s = 1; s < 6; s++) {
            int nr = r + DR[d] * s, nc = c + DC[d] * s;
            if (!inBoard(nr, nc) || G.board[nr][nc] != who) break;
            cells[cnt][0] = nr; cells[cnt][1] = nc; cnt++;
        }
        for (int s = 1; s < 6; s++) {
            int nr = r - DR[d] * s, nc = c - DC[d] * s;
            if (!inBoard(nr, nc) || G.board[nr][nc] != who) break;
            cells[cnt][0] = nr; cells[cnt][1] = nc; cnt++;
        }
        if (cnt >= 5) {
            if (recordCells) {
                G.winCount = cnt > 5 ? 5 : cnt;
                for (int i = 0; i < G.winCount; i++) {
                    G.winCells[i][0] = cells[i][0];
                    G.winCells[i][1] = cells[i][1];
                }
            }
            return 1;
        }
    }
    return 0;
}

static int isBoardFull(void) {
    for (int r = 0; r < G.size; r++)
        for (int c = 0; c < G.size; c++)
            if (G.board[r][c] == 0) return 0;
    return 1;
}

static int isBlockingEnemyCombo(int r, int c, int opp) {
    int blocked = 0;
    G.board[r][c] = opp;
    for (int d = 0; d < 4; d++) {
        int cnt = 1;
        int openEnds = 0;

        int sr = r + DR[d], sc = c + DC[d];
        while (inBoard(sr, sc) && G.board[sr][sc] == opp) {
            cnt++; sr += DR[d]; sc += DC[d];
        }
        if (inBoard(sr, sc) && G.board[sr][sc] == 0) openEnds++;

        sr = r - DR[d]; sc = c - DC[d];
        while (inBoard(sr, sc) && G.board[sr][sc] == opp) {
            cnt++; sr -= DR[d]; sc -= DC[d];
        }
        if (inBoard(sr, sc) && G.board[sr][sc] == 0) openEnds++;

        if (cnt >= 4 || (cnt == 3 && openEnds == 2)) {
            blocked = 1;
            break;
        }
    }
    G.board[r][c] = 0;
    return blocked;
}

static void placePiece(int r, int c, Sound ck, int hs) {
    if (G.gameOver || G.paused || !inBoard(r, c) || G.board[r][c] != 0) return;

    int who = G.turn;
    int opp = 3 - who;
    int blockedCombo = isBlockingEnemyCombo(r, c, opp);

    G.board[r][c] = who;
    G.lastR = r;
    G.lastC = c;
    G.activeItem = -1;
    if (hs) PlaySound(ck);

    // Mỗi nước đi trừ 1 HP
    G.hp[who]--;

    // Kiểm tra thắng 5 quân ngay
    if (checkWinAt(r, c, who, 1)) {
        if (G.hp[who] < 1) G.hp[who] = 1;
        G.gameOver = 1;
        G.winner = who;
        snprintf(G.winReason, sizeof(G.winReason), L("%s đã nối đủ 5 quân!", "%s connected 5 in a row!"), G.pName[who]);
        return;
    }

    // Chặn combo: tăng +2 HP HOẶC nhận 1 vật phẩm ngẫu nhiên
    if (blockedCombo) {
        char msg[128];
        int giveItem = (G.hp[who] >= MAX_HP) ? 1 : GetRandomValue(0, 1);
        if (!giveItem) {
            G.hp[who] += 2;
            if (G.hp[who] > MAX_HP) G.hp[who] = MAX_HP;
            snprintf(msg, sizeof(msg), L("%s chặn combo! +2 HP", "%s blocked combo! +2 HP"), G.pName[who]);
            setToast(msg, CO(40, 155, 60, 255));
        }
        else {
            int it = GetRandomValue(0, 2);
            G.items[who][it]++;
            const char* itNameVi[3] = { "Búa phá", "Đồng hồ +10s", "Bình hồi HP" };
            const char* itNameEn[3] = { "Hammer", "+10s Clock", "Heal Potion" };
            snprintf(msg, sizeof(msg), L("%s chặn combo! +1 %s", "%s blocked combo! +1 %s"), G.pName[who], lg ? itNameEn[it] : itNameVi[it]);
            setToast(msg, CO(35, 120, 200, 255));
        }
    }

    // Kiểm tra hết HP sau nước đi
    if (G.hp[who] <= 0) {
        G.hp[who] = 0;
        G.gameOver = 1;
        G.winner = opp;
        snprintf(G.winReason, sizeof(G.winReason), L("%s đã cạn sạch HP!", "%s ran out of HP!"), G.pName[who]);
        return;
    }

    if (isBoardFull()) {
        G.gameOver = 1;
        if (G.hp[1] > G.hp[2]) {
            G.winner = 1;
            snprintf(G.winReason, sizeof(G.winReason), L("Hết ô! %s thắng nhờ nhiều HP hơn!", "Board full! %s wins by HP!"), G.pName[1]);
        }
        else if (G.hp[2] > G.hp[1]) {
            G.winner = 2;
            snprintf(G.winReason, sizeof(G.winReason), L("Hết ô! %s thắng nhờ nhiều HP hơn!", "Board full! %s wins by HP!"), G.pName[2]);
        }
        else {
            G.winner = 0;
            strcpy(G.winReason, L("Hòa nhau! Bàn cờ đã kín và bằng HP.", "Board full! Draw with equal HP."));
        }
        return;
    }

    G.turn = opp;
    G.turnTimer = G.maxTurnTime;
    if (G.mode > 0 && G.turn == 2) {
        G.aiThinkTimer = (G.mode == 2) ? 0.25f : 0.45f;
    }
}

static int cmpCandDesc(const void* a, const void* b) {
    const MoveCand* ca = (const MoveCand*)a;
    const MoveCand* cb = (const MoveCand*)b;
    if (cb->score > ca->score) return 1;
    if (cb->score < ca->score) return -1;
    return 0;
}

static int hasNeighbor(int r, int c, int dist) {
    int r0 = r - dist < 0 ? 0 : r - dist;
    int r1 = r + dist >= G.size ? G.size - 1 : r + dist;
    int c0 = c - dist < 0 ? 0 : c - dist;
    int c1 = c + dist >= G.size ? G.size - 1 : c + dist;
    for (int i = r0; i <= r1; i++)
        for (int j = c0; j <= c1; j++)
            if (G.board[i][j] != 0) return 1;
    return 0;
}

static int evalPointFor(int r, int c, int who) {
    int total = 0;
    int live3 = 0, dead4 = 0, live4 = 0;

    G.board[r][c] = who;
    for (int d = 0; d < 4; d++) {
        int cnt = 1;
        int openEnds = 0;
        int sr = r + DR[d], sc = c + DC[d];
        while (inBoard(sr, sc) && G.board[sr][sc] == who) { cnt++; sr += DR[d]; sc += DC[d]; }
        int end1Open = (inBoard(sr, sc) && G.board[sr][sc] == 0);
        int extra1 = 0;
        if (end1Open) {
            openEnds++;
            int nr = sr + DR[d], nc = sc + DC[d];
            while (inBoard(nr, nc) && G.board[nr][nc] == who) { extra1++; nr += DR[d]; nc += DC[d]; }
        }

        sr = r - DR[d]; sc = c - DC[d];
        while (inBoard(sr, sc) && G.board[sr][sc] == who) { cnt++; sr -= DR[d]; sc -= DC[d]; }
        int end2Open = (inBoard(sr, sc) && G.board[sr][sc] == 0);
        int extra2 = 0;
        if (end2Open) {
            openEnds++;
            int nr = sr - DR[d], nc = sc - DC[d];
            while (inBoard(nr, nc) && G.board[nr][nc] == who) { extra2++; nr -= DR[d]; nc -= DC[d]; }
        }

        if (cnt >= 5) {
            G.board[r][c] = 0;
            return 20000000;
        }
        if (cnt == 4) {
            if (openEnds == 2) { live4++; total += 2000000; }
            else if (openEnds == 1) { dead4++; total += 160000; }
        }
        else if (cnt == 3) {
            if (openEnds == 2) { live3++; total += 95000; }
            else if (openEnds == 1) total += 9000;
        }
        else if (cnt == 2) {
            if (openEnds == 2) total += 4500;
            else if (openEnds == 1) total += 550;
        }
        else if (cnt == 1 && openEnds == 2) {
            total += 120;
        }

        int brokenMax = cnt + (extra1 > extra2 ? extra1 : extra2);
        if (cnt < 4 && brokenMax >= 4) {
            dead4++;
            total += 140000;
        }
        else if (cnt < 3 && brokenMax == 3 && openEnds == 2) {
            live3++;
            total += 65000;
        }
    }
    G.board[r][c] = 0;

    if (live4 >= 1) total += 1500000;
    if (dead4 >= 2) total += 1200000;
    if (dead4 >= 1 && live3 >= 1) total += 1000000;
    if (live3 >= 2) total += 650000;

    return total;
}

static int evalBoardLine(int len, int openEnds) {
    if (len >= 5) return 10000000;
    if (openEnds == 0) return 0;
    if (len == 4) return (openEnds == 2) ? 1200000 : 90000;
    if (len == 3) return (openEnds == 2) ? 45000 : 3500;
    if (len == 2) return (openEnds == 2) ? 2000 : 250;
    return (openEnds == 2) ? 50 : 10;
}

static void scanLineEval(int r0, int c0, int dr, int dc, int* scoreAI, int* scoreHuman) {
    int r = r0, c = c0;
    while (inBoard(r, c)) {
        int who = G.board[r][c];
        if (who == 0) { r += dr; c += dc; continue; }
        int pr = r - dr, pc = c - dc;
        int openEnds = (inBoard(pr, pc) && G.board[pr][pc] == 0) ? 1 : 0;
        int len = 0;
        while (inBoard(r, c) && G.board[r][c] == who) {
            len++; r += dr; c += dc;
        }
        if (inBoard(r, c) && G.board[r][c] == 0) openEnds++;
        int val = evalBoardLine(len, openEnds);
        if (who == 2) *scoreAI += val;
        else *scoreHuman += val;
    }
}

static int evaluateBoard(int turnToMove) {
    int scoreAI = 0, scoreHuman = 0;
    int sz = G.size;

    for (int i = 0; i < sz; i++) {
        scanLineEval(i, 0, 0, 1, &scoreAI, &scoreHuman);
        scanLineEval(0, i, 1, 0, &scoreAI, &scoreHuman);
    }
    for (int i = 0; i < sz; i++) {
        scanLineEval(i, 0, 1, 1, &scoreAI, &scoreHuman);
        if (i > 0) scanLineEval(0, i, 1, 1, &scoreAI, &scoreHuman);
        scanLineEval(i, sz - 1, 1, -1, &scoreAI, &scoreHuman);
        if (i > 0) scanLineEval(0, sz - 1 - i, 1, -1, &scoreAI, &scoreHuman);
    }

    if (turnToMove == 1) return scoreAI - (int)(scoreHuman * 1.35f);
    return (int)(scoreAI * 1.2f) - scoreHuman;
}

static int generateCandidates(MoveCand* out, int maxKeep, int who) {
    int opp = 3 - who;
    int cnt = 0;
    for (int r = 0; r < G.size; r++) {
        for (int c = 0; c < G.size; c++) {
            if (G.board[r][c] != 0) continue;
            if (!hasNeighbor(r, c, 2)) continue;
            int atk = evalPointFor(r, c, who);
            int def = evalPointFor(r, c, opp);
            int centerBonus = (G.size - abs(r - G.size / 2) - abs(c - G.size / 2)) * 3;
            out[cnt].r = r;
            out[cnt].c = c;
            out[cnt].score = (int)(atk * 1.12f) + def + centerBonus;
            cnt++;
        }
    }
    qsort(out, (size_t)cnt, sizeof(MoveCand), cmpCandDesc);
    return cnt > maxKeep ? maxKeep : cnt;
}

static int minimaxAlphaBeta(int depth, int alpha, int beta, int maximizing) {
    if (depth == 0) return evaluateBoard(maximizing ? 2 : 1);

    int who = maximizing ? 2 : 1;
    MoveCand cands[MAX_BOARD * MAX_BOARD];
    int n = generateCandidates(cands, (depth >= 2) ? 9 : 7, who);
    if (n == 0) return 0;

    if (maximizing) {
        int best = -1000000000;
        for (int i = 0; i < n; i++) {
            int r = cands[i].r, c = cands[i].c;
            G.board[r][c] = 2;
            int val;
            if (checkWinAt(r, c, 2, 0)) val = 50000000 + depth * 1000;
            else val = minimaxAlphaBeta(depth - 1, alpha, beta, 0);
            G.board[r][c] = 0;
            if (val > best) best = val;
            if (best > alpha) alpha = best;
            if (beta <= alpha) break;
        }
        return best;
    }
    else {
        int best = 1000000000;
        for (int i = 0; i < n; i++) {
            int r = cands[i].r, c = cands[i].c;
            G.board[r][c] = 1;
            int val;
            if (checkWinAt(r, c, 1, 0)) val = -50000000 - depth * 1000;
            else val = minimaxAlphaBeta(depth - 1, alpha, beta, 1);
            G.board[r][c] = 0;
            if (val < best) best = val;
            if (best < beta) beta = best;
            if (beta <= alpha) break;
        }
        return best;
    }
}

static void computeNormalBotMove(int* outR, int* outC) {
    MoveCand cands[MAX_BOARD * MAX_BOARD];
    int cnt = 0;

    for (int r = 0; r < G.size; r++) {
        for (int c = 0; c < G.size; c++) {
            if (G.board[r][c] != 0) continue;
            if (!hasNeighbor(r, c, 2)) continue;
            int atk = evalPointFor(r, c, 2);
            int def = evalPointFor(r, c, 1);

            if (atk >= 20000000 && GetRandomValue(1, 100) <= 75) {
                *outR = r; *outC = c; return;
            }
            if (def >= 20000000 && GetRandomValue(1, 100) <= 65) {
                *outR = r; *outC = c; return;
            }

            cands[cnt].r = r;
            cands[cnt].c = c;
            cands[cnt].score = (atk / 20) + (def / 35) + GetRandomValue(0, 800);
            cnt++;
        }
    }

    if (cnt == 0) {
        *outR = G.size / 2;
        *outC = G.size / 2;
        return;
    }

    qsort(cands, (size_t)cnt, sizeof(MoveCand), cmpCandDesc);
    int pick = GetRandomValue(0, cnt > 3 ? 2 : cnt - 1);
    *outR = cands[pick].r;
    *outC = cands[pick].c;
}

static void computeAsianBotMove(int* outR, int* outC) {
    int anyPiece = 0;
    for (int r = 0; r < G.size && !anyPiece; r++)
        for (int c = 0; c < G.size && !anyPiece; c++)
            if (G.board[r][c] != 0) anyPiece = 1;
    if (!anyPiece) {
        *outR = G.size / 2; *outC = G.size / 2; return;
    }

    int bestWinR = -1, bestWinC = -1;
    int bestBlockWinR = -1, bestBlockWinC = -1;
    int bestForkR = -1, bestForkC = -1, bestForkSc = 0;
    int bestBlockForkR = -1, bestBlockForkC = -1, bestBlockForkSc = 0;

    for (int r = 0; r < G.size; r++) {
        for (int c = 0; c < G.size; c++) {
            if (G.board[r][c] != 0 || !hasNeighbor(r, c, 2)) continue;
            int atk = evalPointFor(r, c, 2);
            int def = evalPointFor(r, c, 1);

            if (atk >= 20000000) { bestWinR = r; bestWinC = c; }
            if (def >= 20000000) { bestBlockWinR = r; bestBlockWinC = c; }
            if (atk >= 600000 && atk > bestForkSc) { bestForkSc = atk; bestForkR = r; bestForkC = c; }
            if (def >= 600000 && def > bestBlockForkSc) { bestBlockForkSc = def; bestBlockForkR = r; bestBlockForkC = c; }
        }
    }

    if (bestWinR != -1) { *outR = bestWinR; *outC = bestWinC; return; }
    if (bestBlockWinR != -1) { *outR = bestBlockWinR; *outC = bestBlockWinC; return; }
    if (bestForkR != -1 && bestForkSc >= 1500000) { *outR = bestForkR; *outC = bestForkC; return; }
    if (bestBlockForkR != -1 && bestBlockForkSc >= 1500000) { *outR = bestBlockForkR; *outC = bestBlockForkC; return; }

    MoveCand cands[MAX_BOARD * MAX_BOARD];
    int n = generateCandidates(cands, 10, 2);
    if (n == 0) {
        for (int r = 0; r < G.size; r++)
            for (int c = 0; c < G.size; c++)
                if (G.board[r][c] == 0) { *outR = r; *outC = c; return; }
    }

    int bestVal = -2000000000;
    *outR = cands[0].r;
    *outC = cands[0].c;

    for (int i = 0; i < n; i++) {
        int r = cands[i].r, c = cands[i].c;
        G.board[r][c] = 2;
        int val = minimaxAlphaBeta(2, -1000000000, 1000000000, 0);
        G.board[r][c] = 0;
        if (val > bestVal) {
            bestVal = val;
            *outR = r;
            *outC = c;
        }
    }
}

static void drawPlayerHUD(int pid, Rectangle panel, Vector2 mouse, int lc, Sound ck, int hs) {
    int isTurn = (!G.gameOver && G.turn == pid);
    int isHumanTurn = isTurn && !G.paused && (G.mode == 0 || pid == 1);

    Color bgCol = isTurn ? CO(255, 240, 210, 255) : CO(250, 216, 180, 255);
    pn(panel, CO(25, 14, 8, 255), bgCol);
    drawLeafTex(panel.x + panel.width - 26.0f, panel.y + 4.0f, 22.0f, 1, 0);

    Color br = CO(70, 35, 10, 255);
    Color symCol = (pid == 1) ? CO(210, 45, 45, 255) : CO(30, 100, 200, 255);

    Rectangle badge = RC(panel.x + 16, panel.y + 14, 46, 46);
    DrawRectangleRounded(RC(badge.x - 3, badge.y - 3, badge.width + 6, badge.height + 6), 0.25f, 6, symCol);
    DrawRectangleRounded(badge, 0.25f, 6, CO(255, 250, 235, 255));
    tc(pid == 1 ? "X" : "O", badge, 34, symCol);

    tx(G.pName[pid], panel.x + 74, panel.y + 14, 24, br);
    tx(isTurn ? L("Đang tới lượt...", "Your turn...") : L("Đang chờ...", "Waiting..."), panel.x + 74, panel.y + 42, 18, isTurn ? CO(45, 145, 50, 255) : CO(130, 100, 70, 255));

    DrawLineEx(V2(panel.x + 16, panel.y + 70), V2(panel.x + panel.width - 16, panel.y + 70), 2, CO(195, 155, 105, 255));
    char hpLabel[32];
    snprintf(hpLabel, sizeof(hpLabel), "HP (%d/%d)", G.hp[pid], MAX_HP);
    tx(hpLabel, panel.x + 18, panel.y + 76, 20, br);

    // 2 hàng x 5 trái tim (10 HP)
    for (int i = 0; i < MAX_HP; i++) {
        int row = i / 5;
        int col = i % 5;
        float hx = panel.x + 38 + col * 54;
        float hy = panel.y + 116 + row * 34;
        drawHeart(hx, hy, 13.5f, i < G.hp[pid]);
    }

    DrawLineEx(V2(panel.x + 16, panel.y + 174), V2(panel.x + panel.width - 16, panel.y + 174), 2, CO(195, 155, 105, 255));
    tx(L("Vật phẩm", "Items"), panel.x + 18, panel.y + 182, 23, br);

    const char* itemNamesVi[3] = { "Búa phá quân", "Thêm +10 giây", "Hồi +2 tim HP" };
    const char* itemDescsVi[3] = { "Xóa 1 quân địch (mất lượt)", "Cộng giờ lượt hiện tại", "Hồi phục 2 HP ngay lập tức" };
    const char* itemNamesEn[3] = { "Smash Hammer", "Extra +10s", "Heal +2 HP" };
    const char* itemDescsEn[3] = { "Remove 1 enemy (uses turn)", "Add time to current turn", "Restore 2 HP instantly" };

    for (int i = 0; i < 3; i++) {
        Rectangle ir = RC(panel.x + 16, panel.y + 218 + i * 94, panel.width - 32, 80);
        int hasItem = G.items[pid][i] > 0;
        int selected = (isHumanTurn && G.activeItem == i);
        int hover = isHumanTurn && hasItem && in(ir, mouse);

        Color ib = selected ? CO(220, 60, 40, 255) : CO(25, 14, 8, 255);
        Color ifill = !hasItem ? CO(215, 195, 175, 255)
            : selected ? CO(255, 210, 140, 255)
            : hover ? CO(255, 236, 195, 255) : CO(248, 218, 165, 255);

        DrawRectangleRounded(RC(ir.x - 3, ir.y - 3, ir.width + 6, ir.height + 6), 0.2f, 6, ib);
        DrawRectangleRounded(ir, 0.2f, 6, ifill);

        DrawCircleV(V2(ir.x + 32, ir.y + 40), 21, CO(255, 248, 230, 255));
        drawItemIcon(i, ir.x + 32, ir.y + 40);

        tx(lg ? itemNamesEn[i] : itemNamesVi[i], ir.x + 62, ir.y + 13, 20, hasItem ? br : CO(125, 110, 95, 255));
        tx(lg ? itemDescsEn[i] : itemDescsVi[i], ir.x + 62, ir.y + 41, 15, CO(110, 75, 45, 255));

        char cntTxt[16];
        snprintf(cntTxt, sizeof(cntTxt), "x%d", G.items[pid][i]);
        tx(cntTxt, ir.x + ir.width - 36, ir.y + 11, 21, hasItem ? CO(180, 40, 30, 255) : CO(130, 120, 110, 255));

        if (hover && lc) {
            if (i == 0) {
                G.activeItem = (G.activeItem == 0) ? -1 : 0;
                if (hs) PlaySound(ck);
                if (G.activeItem == 0) setToast(L("Chọn 1 quân địch trên bàn để phá!", "Click an enemy piece to smash!"), CO(200, 70, 30, 255));
            }
            else if (i == 1) {
                G.items[pid][1]--;
                G.turnTimer += 10.0f;
                if (G.turnTimer > G.maxTurnTime + 10.0f) G.turnTimer = G.maxTurnTime + 10.0f;
                if (hs) PlaySound(ck);
                char msg[128];
                snprintf(msg, sizeof(msg), L("%s dùng Đồng hồ: +10 giây!", "%s used Clock: +10s!"), G.pName[pid]);
                setToast(msg, CO(35, 130, 190, 255));
            }
            else if (i == 2) {
                if (G.hp[pid] < MAX_HP) {
                    G.items[pid][2]--;
                    G.hp[pid] += 2;
                    if (G.hp[pid] > MAX_HP) G.hp[pid] = MAX_HP;
                    if (hs) PlaySound(ck);
                    char msg[128];
                    snprintf(msg, sizeof(msg), L("%s hồi phục +2 HP!", "%s healed +2 HP!"), G.pName[pid]);
                    setToast(msg, CO(40, 160, 60, 255));
                }
                else {
                    setToast(L("HP đã đầy, chưa cần dùng!", "HP is already full!"), CO(180, 100, 20, 255));
                }
            }
        }
    }

    if (pid == 1) {
        tx(L("Mỗi nước đi: -1 HP", "Every move: -1 HP"), panel.x + 18, panel.y + 518, 18, CO(175, 45, 35, 255));
        tx(L("Chặn combo: +HP hoặc Item!", "Block combo: +HP or Item!"), panel.x + 18, panel.y + 544, 18, CO(40, 140, 50, 255));
    }
    else {
        tx(L("Hết giờ sẽ bị -1 HP", "Timeout costs -1 HP"), panel.x + 18, panel.y + 518, 18, CO(175, 45, 35, 255));
        tx(L("và mất lượt đánh!", "and skips your turn!"), panel.x + 18, panel.y + 544, 18, CO(95, 60, 30, 255));
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
    cp[nc++] = 0x1A0; cp[nc++] = 0x1A1; cp[nc++] = 0x1AF; cp[nc++] = 0x1B0;
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
    leafTex = createLeafTexture();

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

    strcpy(G.pName[1], "Người chơi 1");
    strcpy(G.pName[2], "Người chơi 2");

    int st = 0, k = 0, k2 = 0, gm = 0, q = 0, dg = 0;
    Color br = CO(70, 35, 10, 255);

    while (!q) {
        if (WindowShouldClose()) q = 1;

        float dt = GetFrameTime();
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
                    if (gm == 0) {
                        if (strcmp(G.pName[1], "Bạn") == 0 || strcmp(G.pName[1], "You") == 0)
                            strcpy(G.pName[1], L("Người chơi 1", "Player 1"));
                        if (strcmp(G.pName[2], "Bot Thường") == 0 || strcmp(G.pName[2], "Normal Bot") == 0 || strcmp(G.pName[2], "Asian AI") == 0)
                            strcpy(G.pName[2], L("Người chơi 2", "Player 2"));
                        syncDefaultNames();
                        G.activeInput = 0;
                        st = 4;
                    }
                    else {
                        startMatch(gm);
                        st = 3;
                    }
                }
            }
        }
        else if (st == 4) {
            if (IsKeyPressed(KEY_ESCAPE)) st = 1;
            if (IsKeyPressed(KEY_TAB)) G.activeInput = 1 - G.activeInput;

            Rectangle box1 = RC(370, 240, 540, 56);
            Rectangle box2 = RC(370, 365, 540, 56);
            Rectangle btnBack = RC(370, 475, 220, 58);
            Rectangle btnPlay = RC(690, 475, 220, 58);

            if (lc) {
                if (in(box1, p)) G.activeInput = 0;
                else if (in(box2, p)) G.activeInput = 1;
                else if (in(btnBack, p)) { if (hs) PlaySound(ck); st = 1; }
                else if (in(btnPlay, p)) {
                    if (hs) PlaySound(ck);
                    startMatch(0);
                    st = 3;
                }
            }

            char* curBuf = (G.activeInput == 0) ? G.pName[1] : G.pName[2];
            int key = GetCharPressed();
            while (key > 0) {
                if (key >= 32 && strlen(curBuf) < 24) {
                    appendUtf8(curBuf, 64, key);
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) popUtf8(curBuf);
            if (IsKeyPressed(KEY_ENTER)) {
                if (hs) PlaySound(ck);
                startMatch(0);
                st = 3;
            }
        }
        else if (st == 3) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (!G.gameOver) {
                    G.paused = !G.paused;
                    if (hs) PlaySound(ck);
                }
                else {
                    st = 1;
                }
            }

            if (!G.paused && G.toastTimer > 0.0f) G.toastTimer -= dt;

            if (!G.gameOver && !G.paused) {
                G.turnTimer -= dt;
                if (G.turnTimer <= 0.0f) {
                    int loserTurn = G.turn;
                    G.hp[loserTurn]--;
                    G.activeItem = -1;
                    char msg[128];
                    snprintf(msg, sizeof(msg), L("Hết giờ! %s bị -1 HP!", "Time's up! %s -1 HP!"), G.pName[loserTurn]);
                    setToast(msg, CO(200, 40, 35, 255));
                    if (hs) PlaySound(ck);

                    if (G.hp[loserTurn] <= 0) {
                        G.hp[loserTurn] = 0;
                        G.gameOver = 1;
                        G.winner = 3 - loserTurn;
                        snprintf(G.winReason, sizeof(G.winReason), L("%s đã cạn sạch HP!", "%s ran out of HP!"), G.pName[loserTurn]);
                    }
                    else {
                        G.turn = 3 - loserTurn;
                        G.turnTimer = G.maxTurnTime;
                        if (G.mode > 0 && G.turn == 2) {
                            G.aiThinkTimer = (G.mode == 2) ? 0.25f : 0.45f;
                        }
                    }
                }

                if (!G.gameOver && G.mode > 0 && G.turn == 2) {
                    G.aiThinkTimer -= dt;
                    if (G.aiThinkTimer <= 0.0f) {
                        int ar = -1, ac = -1;
                        if (G.mode == 1) computeNormalBotMove(&ar, &ac);
                        else computeAsianBotMove(&ar, &ac);
                        if (ar != -1 && ac != -1) {
                            placePiece(ar, ac, ck, hs);
                        }
                    }
                }
            }
        }
        else if (st == 2) {
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
                syncDefaultNames();
                if (hs) PlaySound(ck);
            }
            if (lc && in(bk, p)) {
                st = 0;
                if (hs) PlaySound(ck);
            }
        }

        BeginTextureMode(rt);
        ClearBackground(SKYBLUE);
        if (bg.id) DrawTexturePro(bg, RC(0, 0, (float)bg.width, (float)bg.height), RC(0, 0, W, H), V2(0, 0), 0, WHITE);
        else DrawRectangle(0, 520, W, H - 520, CO(90, 170, 70, 255));

        if (st == 3) {
            DrawRectangle(0, 0, W, H, CO(20, 15, 10, 115));

            int gameClick = (!G.paused && !G.gameOver) ? lc : 0;

            Rectangle topBar = RC(20, 14, W - 40, 62);
            pn(topBar, CO(25, 14, 8, 255), CO(250, 216, 180, 255));

            const char* modeTitle = (G.mode == 0) ? "1 vs 1 (10x10)" : (G.mode == 1) ? L("Bot Thường (10x10)", "Normal Bot (10x10)") : "ASIAN MODE (16x16)";
            tx(modeTitle, 40, 30, 28, G.mode == 2 ? CO(185, 25, 25, 255) : br);

            float ratio = cl(G.turnTimer / G.maxTurnTime);
            Rectangle tBarBg = RC(430, 26, 420, 38);
            DrawRectangleRounded(RC(tBarBg.x - 3, tBarBg.y - 3, tBarBg.width + 6, tBarBg.height + 6), 0.4f, 6, CO(25, 14, 8, 255));
            DrawRectangleRounded(tBarBg, 0.4f, 6, CO(200, 170, 130, 255));
            Color tCol = ratio > 0.4f ? CO(75, 175, 65, 255) : (ratio > 0.2f ? CO(235, 165, 35, 255) : CO(220, 45, 40, 255));
            if (ratio > 0.02f) {
                DrawRectangleRounded(RC(tBarBg.x, tBarBg.y, tBarBg.width * ratio, tBarBg.height), 0.4f, 6, tCol);
            }
            char timerTxt[64];
            snprintf(timerTxt, sizeof(timerTxt), L("Lượt %s: %.1fs", "%s's turn: %.1fs"), G.pName[G.turn], G.turnTimer < 0 ? 0.0f : G.turnTimer);
            tc(timerTxt, tBarBg, 22, CO(45, 20, 5, 255));

            Rectangle btnReset = RC(975, 24, 135, 42);
            Rectangle btnPause = RC(1120, 24, 125, 42);
            btz(btnReset, L("Chơi lại", "Restart"), !G.paused && !G.gameOver && in(btnReset, p), 22.0f);
            btz(btnPause, L("Tạm dừng", "Pause"), !G.paused && !G.gameOver && in(btnPause, p), 22.0f);
            if (gameClick && in(btnReset, p)) {
                if (hs) PlaySound(ck);
                startMatch(G.mode);
            }
            if (gameClick && in(btnPause, p)) {
                if (hs) PlaySound(ck);
                G.paused = 1;
            }

            Rectangle leftPanel = RC(20, 94, 295, 600);
            Rectangle rightPanel = RC(965, 94, 295, 600);
            drawPlayerHUD(1, leftPanel, p, gameClick, ck, hs);
            drawPlayerHUD(2, rightPanel, p, gameClick, ck, hs);

            float boardPx = (G.size == 16) ? 592.0f : 600.0f;
            float cell = boardPx / G.size;
            float bx = (W - boardPx) / 2.0f;
            float by = 94.0f + (600.0f - boardPx) / 2.0f;
            Rectangle boardRect = RC(bx, by, boardPx, boardPx);

            pn(boardRect, CO(25, 14, 8, 255), CO(245, 222, 179, 255));

            for (int r = 0; r < G.size; r++) {
                for (int c = 0; c < G.size; c++) {
                    Rectangle cr = RC(bx + c * cell, by + r * cell, cell, cell);
                    if ((r + c) % 2 == 1) DrawRectangleRec(cr, CO(235, 208, 162, 255));
                }
            }

            if (G.lastR >= 0 && G.lastC >= 0) {
                Rectangle lr = RC(bx + G.lastC * cell + 2, by + G.lastR * cell + 2, cell - 4, cell - 4);
                DrawRectangleRec(lr, CO(255, 230, 110, 140));
            }

            for (int i = 0; i < G.winCount; i++) {
                Rectangle wr = RC(bx + G.winCells[i][1] * cell + 2, by + G.winCells[i][0] * cell + 2, cell - 4, cell - 4);
                DrawRectangleRec(wr, CO(110, 230, 110, 175));
            }

            for (int i = 0; i <= G.size; i++) {
                DrawLineEx(V2(bx + i * cell, by), V2(bx + i * cell, by + boardPx), 1.5f, CO(145, 100, 55, 200));
                DrawLineEx(V2(bx, by + i * cell), V2(bx + boardPx, by + i * cell), 1.5f, CO(145, 100, 55, 200));
            }

            int canHumanAct = (!G.gameOver && !G.paused && (G.mode == 0 || G.turn == 1));
            if (canHumanAct && in(boardRect, p)) {
                int hc = (int)((p.x - bx) / cell);
                int hr2 = (int)((p.y - by) / cell);
                if (inBoard(hr2, hc)) {
                    Rectangle hRect = RC(bx + hc * cell + 2, by + hr2 * cell + 2, cell - 4, cell - 4);
                    if (G.activeItem == 0) {
                        int opp = 3 - G.turn;
                        if (G.board[hr2][hc] == opp) {
                            DrawRectangleLinesEx(hRect, 3.5f, RED);
                            if (gameClick) {
                                G.board[hr2][hc] = 0;
                                G.items[G.turn][0]--;
                                G.activeItem = -1;
                                if (hs) PlaySound(ck);
                                char msg[128];
                                snprintf(msg, sizeof(msg), L("%s đã dùng Búa phá 1 quân địch!", "%s smashed an enemy piece!"), G.pName[G.turn]);
                                setToast(msg, CO(200, 75, 30, 255));
                                G.turn = opp;
                                G.turnTimer = G.maxTurnTime;
                                if (G.mode > 0 && G.turn == 2) {
                                    G.aiThinkTimer = (G.mode == 2) ? 0.25f : 0.45f;
                                }
                            }
                        }
                    }
                    else if (G.board[hr2][hc] == 0) {
                        DrawRectangleRec(hRect, CO(255, 255, 255, 95));
                        DrawRectangleLinesEx(hRect, 2.5f, CO(120, 70, 30, 220));
                        if (gameClick) {
                            placePiece(hr2, hc, ck, hs);
                        }
                    }
                }
            }

            for (int r = 0; r < G.size; r++) {
                for (int c = 0; c < G.size; c++) {
                    int val = G.board[r][c];
                    if (val == 0) continue;
                    float cx = bx + c * cell + cell / 2.0f;
                    float cy = by + r * cell + cell / 2.0f;
                    float pad = cell * 0.24f;
                    float thick = (G.size == 16) ? 4.0f : 6.0f;
                    if (val == 1) {
                        Color xc = CO(210, 42, 42, 255);
                        DrawLineEx(V2(bx + c * cell + pad, by + r * cell + pad), V2(bx + (c + 1) * cell - pad, by + (r + 1) * cell - pad), thick, xc);
                        DrawLineEx(V2(bx + (c + 1) * cell - pad, by + r * cell + pad), V2(bx + c * cell + pad, by + (r + 1) * cell - pad), thick, xc);
                    }
                    else {
                        Color oc = CO(28, 95, 200, 255);
                        float rad = cell * 0.34f;
                        DrawRing(V2(cx, cy), rad - thick * 0.85f, rad, 0, 360, 24, oc);
                    }
                }
            }

            if (G.toastTimer > 0.0f && strlen(G.toast) > 0) {
                Rectangle tr = RC(bx + 30, by + 12, boardPx - 60, 44);
                DrawRectangleRounded(RC(tr.x - 3, tr.y - 3, tr.width + 6, tr.height + 6), 0.3f, 6, G.toastColor);
                DrawRectangleRounded(tr, 0.3f, 6, CO(255, 248, 225, 240));
                tc(G.toast, tr, 22, G.toastColor);
            }

            if (G.paused && !G.gameOver) {
                DrawRectangle(0, 0, W, H, CO(10, 8, 5, 165));
                Rectangle pp = RC(430, 155, 420, 410);
                pn(pp, CO(25, 14, 8, 255), CO(250, 216, 180, 255));
                drawLeafTex(pp.x + 8.0f, pp.y + 8.0f, 30.0f, 0, 0);
                drawLeafTex(pp.x + pp.width - 38.0f, pp.y + 8.0f, 30.0f, 1, 0);
                tc(L("TẠM DỪNG", "PAUSED"), RC(pp.x, pp.y + 24, pp.width, 55), 42, br);

                Rectangle btnRes = RC(pp.x + 65, pp.y + 115, 290, 62);
                Rectangle btnRst = RC(pp.x + 65, pp.y + 205, 290, 62);
                Rectangle btnExt = RC(pp.x + 65, pp.y + 295, 290, 62);

                btz(btnRes, L("Tiếp tục", "Resume"), in(btnRes, p), 30.0f);
                btz(btnRst, L("Chơi lại", "Restart"), in(btnRst, p), 30.0f);
                btz(btnExt, L("Thoát", "Exit"), in(btnExt, p), 30.0f);

                if (lc && in(btnRes, p)) {
                    if (hs) PlaySound(ck);
                    G.paused = 0;
                }
                else if (lc && in(btnRst, p)) {
                    if (hs) PlaySound(ck);
                    startMatch(G.mode);
                }
                else if (lc && in(btnExt, p)) {
                    if (hs) PlaySound(ck);
                    G.paused = 0;
                    st = 1;
                }
            }

            if (G.gameOver) {
                DrawRectangle(0, 0, W, H, CO(0, 0, 0, 155));
                Rectangle gr = RC(360, 210, 560, 300);
                pn(gr, CO(25, 14, 8, 255), CO(250, 216, 180, 255));
                drawLeafTex(gr.x + 8.0f, gr.y + 8.0f, 30.0f, 0, 0);
                drawLeafTex(gr.x + gr.width - 38.0f, gr.y + 8.0f, 30.0f, 1, 0);

                char title[96];
                if (G.winner == 0) strcpy(title, L("TRẬN ĐẤU HÒA!", "IT'S A DRAW!"));
                else snprintf(title, sizeof(title), L("%s CHIẾN THẮNG!", "%s WINS!"), G.pName[G.winner]);
                tc(title, RC(gr.x, gr.y + 30, gr.width, 55), 38, G.winner == 1 ? CO(200, 40, 40, 255) : CO(30, 100, 195, 255));
                tc(G.winReason, RC(gr.x + 20, gr.y + 105, gr.width - 40, 45), 24, br);

                Rectangle btnAgain = RC(gr.x + 55, gr.y + 195, 205, 58);
                Rectangle btnOut = RC(gr.x + 300, gr.y + 195, 205, 58);
                btz(btnAgain, L("Đánh lại", "Play Again"), in(btnAgain, p), 28.0f);
                btz(btnOut, L("Về menu", "Main Menu"), in(btnOut, p), 28.0f);

                if (lc && in(btnAgain, p)) {
                    if (hs) PlaySound(ck);
                    startMatch(G.mode);
                }
                if (lc && in(btnOut, p)) {
                    if (hs) PlaySound(ck);
                    st = 1;
                }
            }
        }
        else if (st == 4) {
            DrawRectangle(0, 0, W, H, CO(0, 0, 0, 130));
            Rectangle np = RC(300, 115, 680, 470);
            pn(np, CO(25, 14, 8, 255), CO(250, 216, 180, 255));
            drawLeafTex(np.x + 8.0f, np.y + 8.0f, 30.0f, 0, 0);
            drawLeafTex(np.x + np.width - 38.0f, np.y + 8.0f, 30.0f, 1, 0);
            tc(L("Nhập tên người chơi (1 vs 1)", "Enter Player Names (1 vs 1)"), RC(np.x, np.y + 20, np.width, 50), 36, br);

            tx(L("Người chơi 1 (Quân X):", "Player 1 (X):"), 370, 202, 26, CO(190, 40, 40, 255));
            Rectangle box1 = RC(370, 240, 540, 56);
            pn(box1, CO(25, 14, 8, 255), CO(255, 250, 235, 255));
            tx(G.pName[1], box1.x + 16, box1.y + 12, 30, br);
            if (G.activeInput == 0 && ((int)(GetTime() * 2) % 2 == 0)) {
                Vector2 sz1 = MeasureTextEx(F, G.pName[1], 30, 1);
                DrawLineEx(V2(box1.x + 18 + sz1.x, box1.y + 10), V2(box1.x + 18 + sz1.x, box1.y + 46), 3, br);
            }

            tx(L("Người chơi 2 (Quân O):", "Player 2 (O):"), 370, 327, 26, CO(30, 95, 195, 255));
            Rectangle box2 = RC(370, 365, 540, 56);
            pn(box2, CO(25, 14, 8, 255), CO(255, 250, 235, 255));
            tx(G.pName[2], box2.x + 16, box2.y + 12, 30, br);
            if (G.activeInput == 1 && ((int)(GetTime() * 2) % 2 == 0)) {
                Vector2 sz2 = MeasureTextEx(F, G.pName[2], 30, 1);
                DrawLineEx(V2(box2.x + 18 + sz2.x, box2.y + 10), V2(box2.x + 18 + sz2.x, box2.y + 46), 3, br);
            }

            Rectangle btnBack = RC(370, 475, 220, 58);
            Rectangle btnPlay = RC(690, 475, 220, 58);
            btz(btnBack, L("Quay lại", "Back"), in(btnBack, p), 28.0f);
            btz(btnPlay, L("Vào trận!", "Play!"), in(btnPlay, p), 28.0f);
        }
        else {
            if (tl.id) {
                float a = 790.0f / tl.width, b0 = 230.0f / tl.height;
                float s = a < b0 ? a : b0;
                float w = tl.width * s, h = tl.height * s;
                DrawTexturePro(tl, RC(0, 0, (float)tl.width, (float)tl.height), RC((W - w) / 2, 40 + (170 - h) / 2, w, h), V2(0, 0), 0, WHITE);
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
            DrawRectangle(0, 0, W, H, CO(0, 0, 0, 130));
            pn(pr, CO(25, 14, 8, 255), CO(250, 216, 180, 255));
            drawLeafTex(pr.x + 8.0f, pr.y + 8.0f, 32.0f, 0, 0);
            drawLeafTex(pr.x + pr.width - 40.0f, pr.y + 8.0f, 32.0f, 1, 0);
            tc(T[lg][T_SE], RC(pr.x, 100, pr.width, 60), 48, br);
            for (int i = 0; i < 2; i++) {
                float cy = 220.0f + i * 90.0f;
                tx(T[lg][T_MU + i], 330, cy - 18, 32, br);
                sl(cy, i ? sv : mv, i ? sm : mm);
                sk(mr[i], i, i ? sm : mm, in(mr[i], p));
            }
            tx(T[lg][T_FS], 330, 400 - 18, 32, br);
            pn(cb, CO(25, 14, 8, 255), CO(255, 250, 235, 255));
            if (fu) {
                DrawLineEx(V2(cb.x + 9, cb.y + 23), V2(cb.x + 18, cb.y + 32), 5, CO(70, 150, 50, 255));
                DrawLineEx(V2(cb.x + 18, cb.y + 32), V2(cb.x + 36, cb.y + 11), 5, CO(70, 150, 50, 255));
            }
            tx(T[lg][T_LG], 330, 490 - 18, 32, br);
            bt(lb, T[lg][T_LN], in(lb, p));
            bt(bk, T[lg][T_BK], in(bk, p));
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(rt.texture, RC(0, 0, (float)W, -(float)H), RC(ox, oy, W * sc, H * sc), V2(0, 0), 0, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(rt);
    UnloadTexture(bg);
    UnloadTexture(tl);
    if (leafTex.id) UnloadTexture(leafTex);
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