#include "raylib.h"

int main(void) {
    // Tasarımınızdaki 640x360 çözünürlük
    const int screenWidth = 640;
    const int screenHeight = 360;

    InitWindow(screenWidth, screenHeight, "AsterOS 2027 - Core v2.0.4");
    SetTargetFPS(60);

    // Tasarımınızdaki renk paleti
    Color bgDarkGreen = { 8, 32, 18, 255 };
    Color panelGreen = { 18, 68, 38, 230 };
    Color activeGreen = { 45, 160, 75, 255 };
    Color borderGreen = { 70, 200, 100, 180 };

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(bgDarkGreen);

        // Arka plan akışkan dalga efekti temsili
        DrawCircleGradient(180, 260, 220, (Color){ 30, 130, 65, 120 }, BLANK);
        DrawCircleGradient(500, 100, 180, (Color){ 20, 100, 50, 90 }, BLANK);

        // Sol üst: Recycle Bin
        DrawRectangleLines(20, 20, 36, 36, borderGreen);
        DrawText("Recycle Bin", 12, 62, 10, RAYWHITE);

        // Orta: Settings Penceresi
        Rectangle win = { 280, 15, 345, 250 };
        DrawRectangleRec(win, panelGreen);
        DrawRectangleLinesEx(win, 1, borderGreen);
        
        // Pencere Üst Çubuğu
        DrawRectangle(280, 15, 345, 22, (Color){ 10, 45, 24, 255 });
        DrawText("Settings", 290, 21, 12, RAYWHITE);
        DrawText("-  []  X", 575, 21, 12, RAYWHITE);

        // Sol Menü (System)
        DrawRectangle(281, 38, 80, 226, (Color){ 12, 50, 28, 220 });
        DrawRectangle(284, 58, 74, 16, activeGreen);
        DrawText("System", 302, 61, 10, RAYWHITE);
        DrawText("Network", 300, 82, 10, LIGHTGRAY);
        DrawText("Audio", 306, 100, 10, LIGHTGRAY);

        // Sağ İçerik Alanı
        DrawText("System Information and Management", 370, 44, 12, RAYWHITE);
        DrawText("Operating System: AsterOS 2027 (x64-bit)", 370, 70, 10, RAYWHITE);
        DrawText("Kernel: AsterOS Core v2.0.4", 370, 85, 10, RAYWHITE);
        DrawText("Version: 27K1 (9)", 370, 100, 10, RAYWHITE);
        DrawText("Resolution: 640x360 (Windowed Mode)", 370, 115, 10, GREEN);

        // Alt Görev Çubuğu (Dock)
        DrawRectangleRounded((Rectangle){ 235, 315, 170, 32 }, 0.3f, 8, panelGreen);
        DrawRectangleRoundedLines((Rectangle){ 235, 315, 170, 32 }, 0.3f, 8, borderGreen);
        DrawText("[#]  [=]  (+)  [O]  >_", 248, 325, 12, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
