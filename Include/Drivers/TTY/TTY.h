#pragma once
#include <Drivers/Graphics/Framebuffer.h>
#include <Inferno/stdint.h>

void SetFramebuffer(Framebuffer* _fb);
void SetFont(void* _font);
void putpixel(unsigned int x, unsigned int y, unsigned int color);
void swapBuffers();

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
        void Swap();
    private:
        int Width, Height, x, y;
        uint8_t* WindowBuffer;
        int current_x = 0;
        int current_y = 0;
};