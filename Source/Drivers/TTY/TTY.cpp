#include <Drivers/TTY/TTY.h>
#include <Memory/Mem_.hpp>
#include <Inferno/stdint.h>
#include <Inferno/Log.h>
#include <Memory/Paging.h>
// #define SSFN_CONSOLEBITMAP_TRUECOLOR
// #include <ssfn.h>

#include <Drivers/TTY/VGA_Font.h>

void* font;
Framebuffer* fb;

void SetFramebuffer(Framebuffer* _fb) {
	fb = _fb;
}

void SetFont(void* _font) {
	font = _font;
}

uint8_t* buffer;

void putpixel(unsigned int x, unsigned int y, unsigned int color) {
	buffer[(4*fb->PPSL*y+4*x)] = (color & 0xFF);
	buffer[(4*fb->PPSL*y+4*x)+1] = (color & 0xFF00) >> 8;
	buffer[(4*fb->PPSL*y+4*x)+2] = (color & 0xFF0000) >> 16;
	buffer[(4*fb->PPSL*y+4*x)+3] = (color & 0xFF000000) >> 24;
}

void swapBuffers() {
	memcpy(fb->Address, buffer, fb->Width * fb->Height * 4);
}

uint8_t* windowBuffer;

Window::Window() {}
Window::Window(int x, int y, int width, int height) {
	this->x = x;
	this->y = y;
	this->Width = width;
	this->Height = height;
	WindowBuffer = (uint8_t*)0x300000; // TODO: make this dynamic
	// for now just create a buffer after the framebuffer
	WindowBuffer = (uint8_t*)((uint64_t)fb->Address + fb->Width * fb->Height * 4);
	buffer = (uint8_t*)0x100000;
	// for now just create a buffer after the WindowBuffer
	buffer = (uint8_t*)((uint64_t)WindowBuffer + fb->Width * fb->Height * 4);
	memcpy(WindowBuffer, buffer+(4*fb->PPSL*y+4), 4*fb->PPSL*height+4*width);
}
void Window::Close() {
	DrawRectangle(0, 0, Width, Height, 0);
	Swap();
}

void Window::Draw(int x, int y, int color) {
	if (x < 0 || x >= Width || y < 0 || y >= Height) return;
	WindowBuffer[(4*fb->PPSL*y+4*x)] = (color & 0xFF);
	WindowBuffer[(4*fb->PPSL*y+4*x)+1] = (color & 0xFF00) >> 8;
	WindowBuffer[(4*fb->PPSL*y+4*x)+2] = (color & 0xFF0000) >> 16;
	WindowBuffer[(4*fb->PPSL*y+4*x)+3] = (color & 0xFF000000) >> 24;
}

void Window::drawChar(char ch, int x, int y, unsigned long long color, int scale) {
    // No more character filtering - all supported characters will be rendered
    uint8_t idx = ASCII_TO_FONT(ch);
	if (ch == '\r') {
		current_x-=8*scale+1;
		return;
	}

	// manually draw colon
	if (ch == ':') {
		Draw(x + 2*scale, y + 1*scale, color);
		Draw(x + 2*scale, y + 2*scale, color);
		Draw(x + 1*scale, y + 5*scale, color);
		Draw(x + 2*scale, y + 5*scale, color);
		return;
	}

	if (ch == ',') {
		Draw(x + 2*scale, y + 5*scale, color);
		Draw(x + 1*scale, y + 5*scale, color);
		return;
	}
    
    for (int row = 0; row < 8; row++) {
        uint8_t line = font8x8[idx][row];
        
        for (int col = 0; col < 8; col++) {
            if (line & (1 << (7 - col))) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        Draw(x + (col * scale) + sx, y + (row * scale) + sy, color);
                    }
                }
            }
        }
    }
}

void Window::drawString(const char* str, int x, int y, unsigned long long color, int scale, bool center) {
	if (center) {
		int len = 0;
		const char* c = str;
		while (*c) {
			len++;
			c++;
		}
		x = (Width - len * 8 * scale) / 2;
	}
    current_x = x;
    current_y = y;
    
    while (*str) {
        drawChar(*str++, current_x, current_y, color, scale);
        current_x += 8 * scale + 1; // Account for character width plus spacing
    }
}

void Window::DrawRectangle(int x, int y, int width, int height, int color) {
	for (int i = x; i < x + width; i++) {
		for (int j = y; j < y + height; j++) {
			if (i >= 0 && i < fb->Width && j >= 0 && j < fb->Height) {
				Draw(i, j, color);
			}
		}
	}
}

void Window::DrawRectircle(int x, int y, int width, int height, int radius, int color) {
	int x1 = x + radius;
	int y1 = y + radius;
	int x2 = x + width - radius;
	int y2 = y + height - radius;

	// Draw the center rectangle
	DrawRectangle(x1, y, width - 2 * radius, height, color);
	DrawRectangle(x, y1, width, height - 2 * radius, color);

	// Draw the rounded corners
	for (int i = -radius; i <= radius; i++) {
		for (int j = -radius; j <= radius; j++) {
			if (i * i + j * j <= radius * radius) {
				if (x1 + i >= 0 && x1 + i < fb->Width && y1 + j >= 0 && y1 + j < fb->Height) {
					Draw(x1 + i, y1 + j, color); // Top-left corner
				}
				if (x2 + i >= 0 && x2 + i < fb->Width && y1 + j >= 0 && y1 + j < fb->Height) {
					Draw(x2 + i, y1 + j, color); // Top-right corner
				}
				if (x1 + i >= 0 && x1 + i < fb->Width && y2 + j >= 0 && y2 + j < fb->Height) {
					Draw(x1 + i, y2 + j, color); // Bottom-left corner
				}
				if (x2 + i >= 0 && x2 + i < fb->Width && y2 + j >= 0 && y2 + j < fb->Height) {
					Draw(x2 + i, y2 + j, color); // Bottom-right corner
				}
			}
		}
	}
}

void Window::DrawCircle(int centerX, int centerY, int radius, int color) {
	int x = radius;
	int y = 0;
	int radiusError = 1 - x;

	while (x >= y) {
		// Draw the eight octants of the circle
		Draw(centerX + x, centerY + y, color);
		Draw(centerX - x, centerY + y, color);
		Draw(centerX + x, centerY - y, color);
		Draw(centerX - x, centerY - y, color);
		Draw(centerX + y, centerY + x, color);
		Draw(centerX - y, centerY + x, color);
		Draw(centerX + y, centerY - x, color);
		Draw(centerX - y, centerY - x, color);

		y++;
		if (radiusError < 0) {
			radiusError += 2 * y + 1;
		} else {
			x--;
			radiusError += 2 * (y - x + 1);
		}
	}
}

void Window::DrawLine(int x1, int x2, int y, int color) {
	for (int i = x1; i <= x2; i++) {
		Draw(i, y, color);
	}
}

void Window::DrawFilledCircle(int centerX, int centerY, int radius, int color) {
	int x = 0;
	int y = radius;
	int d = 3 - 2 * radius;

	// Draw the initial points and lines
	while (y >= x) {
		// Draw horizontal lines to fill the circle
		DrawLine(centerX - x, centerX + x, centerY - y, color);
		DrawLine(centerX - y, centerX + y, centerY - x, color);
		DrawLine(centerX - x, centerX + x, centerY + y, color);
		DrawLine(centerX - y, centerX + y, centerY + x, color);

		x++;
		if (d > 0) {
			y--;
			d = d + 4 * (x - y) + 10;
		} else {
			d = d + 4 * x + 6;
		}
	}
}

void Window::Swap() {
	memcpy(buffer+(4*fb->PPSL*y+4*x), WindowBuffer, 4*fb->PPSL*Height+4*Width);
}