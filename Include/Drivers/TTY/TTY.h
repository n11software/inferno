#pragma once
#include <Drivers/Graphics/Framebuffer.h>
#include <Inferno/stdint.h>

// Direct framebuffer functions
void InitializeScreen(Framebuffer* _fb);
void fbDrawPixel(int x, int y, uint32_t color);
void fbDrawChar(char ch, int x, int y, uint32_t color, int scale);
void fbScrollScreen(int scale);
void fbPrintChar(char c, uint32_t color = 0xFFFFFFFF, int scale = 1);
void fbPrintString(const char* str, uint32_t color = 0xFFFFFFFF, int scale = 1);
void fbClearScreen();
void fbSetCursor(int x, int y);

// Existing functions
void SetFramebuffer(Framebuffer* _fb);
void SetFont(void* _font);
void putpixel(unsigned int x, unsigned int y, unsigned int color);

// Global variables
extern int fb_cursor_x;
extern int fb_cursor_y;
extern Framebuffer* fb;
extern void* font;

// Macro for ASCII to font index conversion
#ifndef ASCII_TO_FONT
#define ASCII_TO_FONT(c) ((c) < 32 ? 0 : ((c) > 127 ? 0 : ((c) - 32)))
#endif

// Window declarations
class Window {
    public:
        Window();
        Window(int x, int y, int width, int height);
        void Close();
        void Draw(int x, int y, int color);
        void drawChar(char ch, int x, int y, unsigned long long color, int scale = 2);
        void drawString(const char* str, int x, int y, unsigned long long color, int scale = 2, bool center = false);
        void DrawRectangle(int x, int y, int width, int height, int color);
        void DrawRectircle(int x, int y, int width, int height, int radius, int color);
        void DrawCircle(int centerX, int centerY, int radius, int color);
        void DrawLine(int x1, int x2, int y, int color);
        void DrawFilledCircle(int centerX, int centerY, int radius, int color);
        void Swap(); // Kept for API compatibility but now does nothing
    private:
        int Width, Height, x, y;
        int current_x = 0;
        int current_y = 0;
};