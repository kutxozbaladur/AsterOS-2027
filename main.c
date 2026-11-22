#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/input.h>

void drawText(SDL_Renderer *r, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!font || !text || strlen(text) == 0) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void drawGlassPanel(SDL_Renderer *r, int x, int y, int w, int h, int radius, SDL_Color fill, SDL_Color border) {
    roundedBoxRGBA(r, x, y, x + w, y + h, radius, fill.r, fill.g, fill.b, fill.a);
    roundedRectangleRGBA(r, x, y, x + w, y + h, radius, border.r, border.g, border.b, border.a);
}

void stripHtml(const char *src, char *dst, int maxLen) {
    bool inTag = false;
    int j = 0;
    for (int i = 0; src[i] != '\0' && j < maxLen - 1; i++) {
        if (src[i] == '<') { inTag = true; continue; }
        if (src[i] == '>') { inTag = false; if (j > 0 && dst[j-1] != ' ') dst[j++] = ' '; continue; }
        if (!inTag) {
            char c = src[i];
            if (c == '\n' || c == '\r' || c == '\t') c = ' ';
            if (c == ' ' && j > 0 && dst[j-1] == ' ') continue;
            dst[j++] = c;
        }
    }
    dst[j] = '\0';
}

char keycodeToChar(int code, bool shift) {
    const char *mapLow  = "..1234567890-=\b\tqwertyuiop[]\n.asdfghjkl;'`.\\zxcvbnm,./";
    const char *mapHigh = "..!@#$%^&*()_+\b\tQWERTYUIOP{}\n.ASDFGHJKL:\"~.|ZXCVBNM<>?";
    if (code == KEY_SPACE) return ' ';
    if (code >= 2 && code <= 53) return shift ? *(mapHigh + code) : *(mapLow + code);
    return 0;
}

int main(void) {
    bool isFbMode = (getenv("DISPLAY") == NULL);
    int fbFd = -1, mouseFd = -1;
    int *kbdFds = (int *)malloc(sizeof(int) * 6);
    for (int i = 0; i < 6; i++) *(kbdFds + i) = -1;

    uint8_t *fbPtr = NULL;
    int W = 800, H = 600;
    int fbStride = W * 4, fbBpp = 32;

    if (isFbMode) {
        fbFd = open("/dev/fb0", O_RDWR);
        if (fbFd >= 0) {
            struct fb_var_screeninfo vinfo;
            struct fb_fix_screeninfo finfo;
            ioctl(fbFd, FBIOGET_VSCREENINFO, &vinfo);
            ioctl(fbFd, FBIOGET_FSCREENINFO, &finfo);
            W = vinfo.xres;
            H = vinfo.yres;
            fbBpp = vinfo.bits_per_pixel;
            fbStride = finfo.line_length;
            fbPtr = (uint8_t *)mmap(0, fbStride * H, PROT_READ | PROT_WRITE, MAP_SHARED, fbFd, 0);
        }
        mouseFd = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
        for (int i = 0; i < 6; i++) {
            char *p = (char *)calloc(32, 1);
            snprintf(p, 31, "/dev/input/event%d", i);
            *(kbdFds + i) = open(p, O_RDONLY | O_NONBLOCK | O_NDELAY);
            free(p);
        }
    } else {
        SDL_Init(SDL_INIT_VIDEO);
    }
    TTF_Init();

    SDL_Window *window = NULL;
    SDL_Surface *fbSurf = NULL;
    SDL_Renderer *renderer = NULL;

    if (isFbMode) {
        fbSurf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
        renderer = SDL_CreateSoftwareRenderer(fbSurf);
    } else {
        window = SDL_CreateWindow("AsterOS Desktop", 0, 0, W, H, SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_Surface *bgSurf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *bgRen = SDL_CreateSoftwareRenderer(bgSurf);
    SDL_SetRenderDrawBlendMode(bgRen, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(bgRen, 6, 26, 15, 255);
    SDL_RenderClear(bgRen);
    filledCircleRGBA(bgRen, W / 4, H - 150, 300, 22, 105, 52, 85);
    filledCircleRGBA(bgRen, W - 180, 150, 250, 18, 90, 45, 75);
    for (int x = 0; x < W; x += 2) {
        int y1 = (H / 2) + 60 + (int)(50.0 * sin(x * 0.006));
        vlineRGBA(bgRen, x, y1, H, 28, 125, 62, 60);
    }
    SDL_DestroyRenderer(bgRen);
    SDL_Texture *bgTex = SDL_CreateTextureFromSurface(renderer, bgSurf);

    TTF_Font *fontTitle = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 14);
    TTF_Font *fontReg   = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12);

    SDL_Color glassBg     = { 15, 58, 34, 215 };
    SDL_Color glassDark   = { 10, 40, 24, 238 };
    SDL_Color termBg      = { 6, 22, 14, 242 };
    SDL_Color activeGreen = { 46, 175, 85, 240 };
    SDL_Color borderGreen = { 90, 220, 125, 190 };
    SDL_Color white       = { 240, 250, 245, 255 };
    SDL_Color mutedText   = { 170, 210, 185, 255 };

    int dragWin = 0, dragOffX = 0, dragOffY = 0, activeFocus = 4;

    bool showSettings = true;
    int setX = 320, setY = 30, setW = 450, setH = 270, activeTab = 0;

    bool showFiles = false;
    int fileX = 35, fileY = 95, fileW = 330, fileH = 310;
    char *currPath = (char *)calloc(256, 1);
    strcpy(currPath, "/");

    bool showTerm = false;
    int termX = 90, termY = 170, termW = 470, termH = 270;
    char *termInput = (char *)calloc(256, 1);
    char *termOut1  = (char *)calloc(256, 1);
    char *termOut2  = (char *)calloc(256, 1);
    strcpy(termOut1, "AsterOS Shell v2.0.4 Ready.");
    strcpy(termOut2, "Type a command (e.g. ls, uname -a, ip a) and press Enter.");

    bool showWeb = false;
    int webX = 60, webY = 45, webW = 560, webH = 390;
    char *urlInput = (char *)calloc(256, 1);
    char *webLine1 = (char *)calloc(256, 1);
    char *webLine2 = (char *)calloc(256, 1);
    char *webLine3 = (char *)calloc(256, 1);
    strcpy(urlInput, "http://example.com");
    strcpy(webLine1, "AsterWeb Engine Ready.");
    strcpy(webLine2, "Type URL (e.g. http://example.com) and press Enter.");

    int mouseX = W / 2, mouseY = H / 2;
    bool prevLeft = false, leftDown = false, shiftDown = false;
    signed char *mdata = (signed char *)malloc(3);

    bool running = true;
    SDL_Event e;

    while (running) {
        bool leftClick = false, leftRelease = false;
        char typedChar = 0;
        bool backspacePressed = false, enterPressed = false;

        if (isFbMode) {
            if (mouseFd >= 0) {
                while (read(mouseFd, mdata, 3) == 3) {
                    bool btn = (*(mdata + 0) & 0x01) != 0;
                    mouseX += *(mdata + 1);
                    mouseY -= *(mdata + 2);
                    if (mouseX < 0) mouseX = 0;
                    if (mouseX >= W) mouseX = W - 1;
                    if (mouseY < 0) mouseY = 0;
                    if (mouseY >= H) mouseY = H - 1;
                    if (btn && !prevLeft) { leftClick = true; leftDown = true; }
                    if (!btn && prevLeft) { leftRelease = true; leftDown = false; }
                    prevLeft = btn;
                }
            }
            for (int i = 0; i < 6; i++) {
                int fd = *(kbdFds + i);
                if (fd >= 0) {
                    struct input_event ev;
                    while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
                        if (ev.type == EV_KEY) {
                            if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT) shiftDown = (ev.value != 0);
                            if (ev.value == 1) {
                                if (ev.code == KEY_BACKSPACE) backspacePressed = true;
                                else if (ev.code == KEY_ENTER || ev.code == KEY_KPENTER) enterPressed = true;
                                else {
                                    char c = keycodeToChar(ev.code, shiftDown);
                                    if (c != 0 && c != '\b' && c != '\n') typedChar = c;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) running = false;
                if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                    if (e.key.keysym.sym == SDLK_BACKSPACE) backspacePressed = true;
                    if (e.key.keysym.sym == SDLK_RETURN) enterPressed = true;
                }
                if (e.type == SDL_MOUSEMOTION) { mouseX = e.motion.x; mouseY = e.motion.y; }
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) { leftClick = true; leftDown = true; }
                if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) { leftRelease = true; leftDown = false; }
            }
        }

        if (showTerm && activeFocus == 3) {
            if (typedChar != 0 && strlen(termInput) < 120) {
                int len = strlen(termInput);
                *(termInput + len) = typedChar;
                *(termInput + len + 1) = '\0';
            }
            if (backspacePressed && strlen(termInput) > 0) *(termInput + strlen(termInput) - 1) = '\0';
            if (enterPressed && strlen(termInput) > 0) {
                strcpy(termOut1, ""); strcpy(termOut2, "");
                FILE *fp = popen(termInput, "r");
                if (fp) {
                    if (fgets(termOut1, 75, fp)) *(termOut1 + strcspn(termOut1, "\r\n")) = 0;
                    if (fgets(termOut2, 75, fp)) *(termOut2 + strcspn(termOut2, "\r\n")) = 0;
                    pclose(fp);
                }
                strcpy(termInput, "");
            }
        } else if (showWeb && activeFocus == 4) {
            if (typedChar != 0 && strlen(urlInput) < 120) {
                int len = strlen(urlInput);
                *(urlInput + len) = typedChar;
                *(urlInput + len + 1) = '\0';
            }
            if (backspacePressed && strlen(urlInput) > 0) *(urlInput + strlen(urlInput) - 1) = '\0';
            if (enterPressed && strlen(urlInput) > 0) {
                strcpy(webLine1, "Fetching page...");
                strcpy(webLine2, urlInput);
                strcpy(webLine3, "");
                char *target = (char *)calloc(300, 1);
                if (strncmp(urlInput, "http", 4) == 0) strcpy(target, urlInput);
                else snprintf(target, 299, "http://%s", urlInput);
                char *cmd = (char *)calloc(512, 1);
                snprintf(cmd, 511, "wget -qO- -T 5 \"%s\" 2>/dev/null", target);
                FILE *fp = popen(cmd, "r");
                if (fp) {
                    char *raw = (char *)calloc(3000, 1);
                    size_t n = fread(raw, 1, 2900, fp);
                    pclose(fp);
                    if (n > 0) {
                        char *clean = (char *)calloc(3000, 1);
                        stripHtml(raw, clean, 2900);
                        strncpy(webLine1, clean, 74);
                        if (strlen(clean) > 74) strncpy(webLine2, clean + 74, 74);
                        if (strlen(clean) > 148) strncpy(webLine3, clean + 148, 74);
                        free(clean);
                    } else {
                        strcpy(webLine1, "No data received (Note: BusyBox wget supports HTTP).");
                        strcpy(webLine2, "Try http://example.com or cern.ch");
                    }
                    free(raw);
                }
                free(cmd); free(target);
            }
        }

        if (leftRelease) dragWin = 0;
        if (leftDown && dragWin != 0) {
            if (dragWin == 1) { setX = mouseX - dragOffX; setY = mouseY - dragOffY; }
            if (dragWin == 2) { fileX = mouseX - dragOffX; fileY = mouseY - dragOffY; }
            if (dragWin == 3) { termX = mouseX - dragOffX; termY = mouseY - dragOffY; }
            if (dragWin == 4) { webX = mouseX - dragOffX; webY = mouseY - dragOffY; }
        }

        if (leftClick) {
            int dockW = 340, dockH = 50;
            int dockX = (W - dockW) / 2;
            int dockY = H - 64;
            if (mouseY >= dockY && mouseY <= dockY + dockH) {
                for (int i = 0; i < 5; i++) {
                    int ix = dockX + 18 + (i * 63);
                    if (mouseX >= ix && mouseX <= ix + 46) {
                        if (i == 1) showFiles = !showFiles;
                        if (i == 2) { showWeb = !showWeb; if (showWeb) activeFocus = 4; }
                        if (i == 3) showSettings = !showSettings;
                        if (i == 4) { showTerm = !showTerm; if (showTerm) activeFocus = 3; }
                    }
                }
            }

            if (showWeb && mouseX >= webX && mouseX <= webX + webW && mouseY >= webY && mouseY <= webY + webH) {
                activeFocus = 4;
                if (mouseX >= webX + webW - 28 && mouseX <= webX + webW - 10 && mouseY >= webY + 8 && mouseY <= webY + 26)
                    showWeb = false;
                else if (mouseY <= webY + 34) { dragWin = 4; dragOffX = mouseX - webX; dragOffY = mouseY - webY; }
            }

            if (showTerm && mouseX >= termX && mouseX <= termX + termW && mouseY >= termY && mouseY <= termY + termH) {
                activeFocus = 3;
                if (mouseX >= termX + termW - 28 && mouseX <= termX + termW - 10 && mouseY >= termY + 8 && mouseY <= termY + 26)
                    showTerm = false;
                else if (mouseY <= termY + 34) { dragWin = 3; dragOffX = mouseX - termX; dragOffY = mouseY - termY; }
            }

            if (showFiles && mouseX >= fileX && mouseX <= fileX + fileW && mouseY >= fileY && mouseY <= fileY + fileH) {
                if (mouseX >= fileX + fileW - 28 && mouseX <= fileX + fileW - 10 && mouseY >= fileY + 8 && mouseY <= fileY + 26)
                    showFiles = false;
                else if (mouseY <= fileY + 34) { dragWin = 2; dragOffX = mouseX - fileX; dragOffY = mouseY - fileY; }
                if (mouseX >= fileX + 12 && mouseX <= fileX + 75 && mouseY >= fileY + 42 && mouseY <= fileY + 64) {
                    char *lastSlash = strrchr(currPath, '/');
                    if (lastSlash && lastSlash != currPath) *lastSlash = '\0';
                    else strcpy(currPath, "/");
                }
                DIR *d = opendir(currPath);
                if (d) {
                    struct dirent *dir;
                    int idx = 0;
                    while ((dir = readdir(d)) != NULL && idx < 7) {
                        if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                            int fy = fileY + 72 + (idx * 30);
                            if (mouseX >= fileX + 12 && mouseX <= fileX + fileW - 12 && mouseY >= fy && mouseY <= fy + 24) {
                                char *nextPath = (char *)calloc(512, 1);
                                if (strcmp(currPath, "/") == 0) snprintf(nextPath, 511, "/%s", dir->d_name);
                                else snprintf(nextPath, 511, "%s/%s", currPath, dir->d_name);
                                struct stat st;
                                if (stat(nextPath, &st) == 0 && S_ISDIR(st.st_mode)) strcpy(currPath, nextPath);
                                free(nextPath);
                                break;
                            }
                            idx++;
                        }
                    }
                    closedir(d);
                }
            }

            if (showSettings && mouseX >= setX && mouseX <= setX + setW && mouseY >= setY && mouseY <= setY + setH) {
                if (mouseX >= setX + setW - 28 && mouseX <= setX + setW - 10 && mouseY >= setY + 8 && mouseY <= setY + 26)
                    showSettings = false;
                else if (mouseY <= setY + 34) { dragWin = 1; dragOffX = mouseX - setX; dragOffY = mouseY - setY; }
                if (mouseX >= setX + 12 && mouseX <= setX + 125) {
                    if (mouseY >= setY + 55 && mouseY <= setY + 83) activeTab = 0;
                    if (mouseY >= setY + 88 && mouseY <= setY + 116) activeTab = 1;
                    if (mouseY >= setY + 121 && mouseY <= setY + 149) activeTab = 2;
                }
            }
        }

        SDL_RenderCopy(renderer, bgTex, NULL, NULL);
        drawGlassPanel(renderer, 28, 28, 54, 54, 14, glassBg, borderGreen);
        drawText(renderer, fontReg, "Recycle Bin", 22, 88, white);

        if (showSettings) {
            drawGlassPanel(renderer, setX, setY, setW, setH, 16, glassBg, borderGreen);
            drawGlassPanel(renderer, setX, setY, setW, 34, 16, glassDark, borderGreen);
            drawText(renderer, fontTitle, "Settings", setX + 16, setY + 9, white);
            filledCircleRGBA(renderer, setX + setW - 20, setY + 17, 6, 220, 75, 75, 240);
            drawGlassPanel(renderer, setX + 10, setY + 44, 115, setH - 54, 12, glassDark, borderGreen);
            if (activeTab == 0) drawGlassPanel(renderer, setX + 16, setY + 55, 103, 26, 8, activeGreen, borderGreen);
            if (activeTab == 1) drawGlassPanel(renderer, setX + 16, setY + 88, 103, 26, 8, activeGreen, borderGreen);
            if (activeTab == 2) drawGlassPanel(renderer, setX + 16, setY + 121, 103, 26, 8, activeGreen, borderGreen);
            drawText(renderer, fontReg, "System", setX + 26, setY + 61, white);
            drawText(renderer, fontReg, "Network", setX + 26, setY + 94, white);
            drawText(renderer, fontReg, "Audio", setX + 26, setY + 127, white);
            int cx = setX + 138;
            if (activeTab == 0) {
                drawText(renderer, fontTitle, "AsterOS 2027 (x64-bit Live Boot)", cx, setY + 55, white);
                drawText(renderer, fontReg, "Kernel: AsterOS Core v2.0.4", cx, setY + 85, mutedText);
                drawText(renderer, fontReg, "All Systems Operational", cx, setY + 110, activeGreen);
            } else if (activeTab == 1) {
                drawText(renderer, fontTitle, "Network & Connectivity", cx, setY + 55, white);
                drawText(renderer, fontReg, "eth0: 10.0.2.15 (QEMU NAT Ready)", cx, setY + 85, activeGreen);
            } else {
                drawText(renderer, fontTitle, "Audio Output", cx, setY + 55, white);
                drawText(renderer, fontReg, "AsterOS HD Audio Enabled", cx, setY + 85, white);
            }
        }

        if (showFiles) {
            drawGlassPanel(renderer, fileX, fileY, fileW, fileH, 16, glassBg, borderGreen);
            drawGlassPanel(renderer, fileX, fileY, fileW, 34, 16, glassDark, borderGreen);
            drawText(renderer, fontTitle, "Aster Files", fileX + 14, fileY + 9, white);
            filledCircleRGBA(renderer, fileX + fileW - 20, fileY + 17, 6, 220, 75, 75, 240);
            drawGlassPanel(renderer, fileX + 12, fileY + 42, 60, 22, 6, activeGreen, borderGreen);
            drawText(renderer, fontReg, "< Back", fileX + 18, fileY + 46, white);
            drawText(renderer, fontReg, currPath, fileX + 82, fileY + 46, mutedText);
            DIR *d = opendir(currPath);
            if (d) {
                struct dirent *dir;
                int idx = 0;
                while ((dir = readdir(d)) != NULL && idx < 7) {
                    if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                        int fy = fileY + 72 + (idx * 30);
                        char *fullP = (char *)calloc(512, 1);
                        if (strcmp(currPath, "/") == 0) snprintf(fullP, 511, "/%s", dir->d_name);
                        else snprintf(fullP, 511, "%s/%s", currPath, dir->d_name);
                        struct stat st;
                        bool isDir = (stat(fullP, &st) == 0 && S_ISDIR(st.st_mode));
                        free(fullP);
                        drawGlassPanel(renderer, fileX + 12, fy, fileW - 24, 24, 6, glassDark, borderGreen);
                        char *label = (char *)calloc(160, 1);
                        snprintf(label, 159, "%s %s", isDir ? "[DIR]" : "[FILE]", dir->d_name);
                        drawText(renderer, fontReg, label, fileX + 20, fy + 5, isDir ? activeGreen : white);
                        free(label);
                        idx++;
                    }
                }
                closedir(d);
            }
        }

        if (showTerm) {
            drawGlassPanel(renderer, termX, termY, termW, termH, 16, termBg, borderGreen);
            drawGlassPanel(renderer, termX, termY, termW, 34, 16, glassDark, borderGreen);
            drawText(renderer, fontTitle, "AsterOS Terminal (Interactive)", termX + 14, termY + 9, white);
            filledCircleRGBA(renderer, termX + termW - 20, termY + 17, 6, 220, 75, 75, 240);
            drawText(renderer, fontReg, termOut1, termX + 18, termY + 52, mutedText);
            drawText(renderer, fontReg, termOut2, termX + 18, termY + 74, mutedText);
            drawGlassPanel(renderer, termX + 12, termY + termH - 44, termW - 24, 30, 8, glassDark, (activeFocus == 3) ? activeGreen : borderGreen);
            drawText(renderer, fontReg, "aster@os:/#", termX + 20, termY + termH - 36, activeGreen);
            drawText(renderer, fontReg, termInput, termX + 100, termY + termH - 36, white);
        }

        if (showWeb) {
            drawGlassPanel(renderer, webX, webY, webW, webH, 16, glassBg, borderGreen);
            drawGlassPanel(renderer, webX, webY, webW, 34, 16, glassDark, borderGreen);
            drawText(renderer, fontTitle, "AsterWeb Browser", webX + 14, webY + 9, white);
            filledCircleRGBA(renderer, webX + webW - 20, webY + 17, 6, 220, 75, 75, 240);
            drawGlassPanel(renderer, webX + 12, webY + 42, webW - 24, 28, 8, termBg, (activeFocus == 4) ? activeGreen : borderGreen);
            drawText(renderer, fontReg, urlInput, webX + 22, webY + 49, white);
            drawGlassPanel(renderer, webX + 12, webY + 78, webW - 24, webH - 90, 10, glassDark, borderGreen);
            drawText(renderer, fontReg, webLine1, webX + 22, webY + 95, white);
            drawText(renderer, fontReg, webLine2, webX + 22, webY + 120, mutedText);
            drawText(renderer, fontReg, webLine3, webX + 22, webY + 145, mutedText);
        }

        int dockW = 340, dockH = 50;
        int dockX = (W - dockW) / 2;
        int dockY = H - 64;
        drawGlassPanel(renderer, dockX, dockY, dockW, dockH, 20, glassDark, borderGreen);
        for (int i = 0; i < 5; i++) {
            int ix = dockX + 18 + (i * 63);
            int iy = dockY + 7;
            bool isAct = (i == 1 && showFiles) || (i == 2 && showWeb) || (i == 3 && showSettings) || (i == 4 && showTerm);
            drawGlassPanel(renderer, ix, iy, 46, 36, 12, isAct ? activeGreen : glassBg, borderGreen);
            const char *label = "::";
            if (i == 1) label = "Dir";
            if (i == 2) label = "Web";
            if (i == 3) label = "Set";
            if (i == 4) label = ">_";
            drawText(renderer, fontTitle, label, ix + 11, iy + 10, white);
        }
        drawGlassPanel(renderer, W - 130, H - 54, 112, 34, 14, glassDark, borderGreen);
        drawText(renderer, fontReg, "Wi-Fi  |  15:14", W - 116, H - 43, white);

        if (isFbMode) {
            filledTrigonRGBA(renderer, mouseX, mouseY, mouseX, mouseY + 16, mouseX + 11, mouseY + 11, 245, 255, 250, 255);
            trigonRGBA(renderer, mouseX, mouseY, mouseX, mouseY + 16, mouseX + 11, mouseY + 11, 10, 40, 24, 255);
        }

        if (isFbMode && fbPtr && fbSurf) {
            SDL_Surface *outSurf = fbSurf;
            if (fbBpp == 24) outSurf = SDL_ConvertSurfaceFormat(fbSurf, SDL_PIXELFORMAT_BGR24, 0);
            else if (fbBpp == 16) outSurf = SDL_ConvertSurfaceFormat(fbSurf, SDL_PIXELFORMAT_RGB565, 0);
            uint8_t *src = (uint8_t *)outSurf->pixels;
            int copyBytes = (outSurf->pitch < fbStride) ? outSurf->pitch : fbStride;
            for (int y = 0; y < H; y++) {
                memcpy(fbPtr + (y * fbStride), src + (y * outSurf->pitch), copyBytes);
            }
            if (outSurf != fbSurf) SDL_FreeSurface(outSurf);
        } else {
            SDL_RenderPresent(renderer);
        }
        SDL_Delay(5);
    }

    free(mdata); free(kbdFds); free(currPath); free(termInput); free(termOut1); free(termOut2);
    free(urlInput); free(webLine1); free(webLine2); free(webLine3);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
