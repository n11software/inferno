#include <Drivers/TTY/TTY.h>
#include <Memory/Mem_.hpp>
#include <Inferno/stdint.h>
#include <Inferno/Log.h>
#include <Drivers/TTY/VGA_Font.h>

// Global cursor position for direct framebuffer text output
int fb_cursor_x = 0;
int fb_cursor_y = 0;

// Framebuffer pointer
Framebuffer* fb;
void* font;

// Initialize the screen
void InitializeScreen(Framebuffer* _fb) {
    fb = _fb;
    // Clear the screen directly
    for (int i = 0; i < fb->Width * fb->Height * 4; i++) {
        *((uint8_t*)(fb->Address) + i) = 0;
    }
    fb_cursor_x = 0;
    fb_cursor_y = 0;
}

// Function to draw a pixel directly to the framebuffer
void fbDrawPixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= fb->Width || y < 0 || y >= fb->Height) return;
    
    uint8_t* pixel = (uint8_t*)(fb->Address) + (4*fb->PPSL*y + 4*x);
    pixel[0] = (color & 0xFF);
    pixel[1] = (color & 0xFF00) >> 8;
    pixel[2] = (color & 0xFF0000) >> 16;
    pixel[3] = (color & 0xFF000000) >> 24;
}

// Function to draw a character directly to the framebuffer
void fbDrawChar(char ch, int x, int y, uint32_t color, int scale) {
    uint8_t idx = ASCII_TO_FONT(ch);
    
    // Special handling for colon
    if (ch == ':') {
        fbDrawPixel(x + 2*scale, y + 1*scale, color);
        fbDrawPixel(x + 2*scale, y + 2*scale, color);
        fbDrawPixel(x + 2*scale, y + 5*scale, color);
        fbDrawPixel(x + 1*scale, y + 5*scale, color);
        return;
    }
    
    // Special handling for comma
    if (ch == ',') {
        fbDrawPixel(x + 2*scale, y + 5*scale, color);
        fbDrawPixel(x + 1*scale, y + 5*scale, color);
        return;
    }
    
    for (int row = 0; row < 8; row++) {
        uint8_t line = font8x8[idx][row];
        
        for (int col = 0; col < 8; col++) {
            if (line & (1 << (7 - col))) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        fbDrawPixel(x + (col * scale) + sx, y + (row * scale) + sy, color);
                    }
                }
            }
        }
    }
}

// Function to scroll the screen up one line
void fbScrollScreen(int scale) {
    int lineHeight = 9 * scale; // 8 for font height plus 1 for spacing
    
    // Move everything up one line directly in the framebuffer
    uint8_t* fb_ptr = (uint8_t*)(fb->Address);
    
    // Copy lines up
    for (int y = lineHeight; y < fb->Height; y++) {
        for (int x = 0; x < fb->Width; x++) {
            int dst = 4*(fb->PPSL*(y-lineHeight) + x);
            int src = 4*(fb->PPSL*y + x);
            fb_ptr[dst] = fb_ptr[src];
            fb_ptr[dst+1] = fb_ptr[src+1];
            fb_ptr[dst+2] = fb_ptr[src+2];
            fb_ptr[dst+3] = fb_ptr[src+3];
        }
    }
    
    // Clear the bottom line
    for (int y = fb->Height - lineHeight; y < fb->Height; y++) {
        for (int x = 0; x < fb->Width; x++) {
            int pos = 4*(fb->PPSL*y + x);
            fb_ptr[pos] = 0;
            fb_ptr[pos+1] = 0;
            fb_ptr[pos+2] = 0;
            fb_ptr[pos+3] = 0;
        }
    }
    
    // Update cursor position
    fb_cursor_y -= lineHeight;
}

// Function to print a character to the screen at the current cursor position
void fbPrintChar(char c, uint32_t color, int scale) {
    if (!fb) return;
    
    int char_width = 8 * scale + 1;  // Character width plus spacing
    int char_height = 8 * scale + 1; // Character height plus spacing
    
    // Handle special characters
    if (c == '\n') {
        fb_cursor_x = 0;
        fb_cursor_y += char_height;
    }
    else if (c == '\r') {
        fb_cursor_x = 0;
    }
    else if (c == '\t') {
        fb_cursor_x += char_width * 4;
        fb_cursor_x -= fb_cursor_x % (char_width * 4);
    }
    else if (c == '\b') {
        // Handle backspace - move cursor back and clear the character
        if (fb_cursor_x >= char_width) {
            fb_cursor_x -= char_width;
            // Clear character space
            for (int y = fb_cursor_y; y < fb_cursor_y + char_height; y++) {
                for (int x = fb_cursor_x; x < fb_cursor_x + char_width; x++) {
                    fbDrawPixel(x, y, 0);
                }
            }
        }
    }
    else {
        // Check if we need to wrap to the next line
        if (fb_cursor_x + char_width > fb->Width) {
            fb_cursor_x = 0;
            fb_cursor_y += char_height;
        }
        
        // Check if we need to scroll
        if (fb_cursor_y + char_height > fb->Height) {
            fbScrollScreen(scale);
        }
        
        // Draw the character directly to framebuffer
        fbDrawChar(c, fb_cursor_x, fb_cursor_y, color, scale);
        fb_cursor_x += char_width;
    }
}

// Function to print a string to the screen at the current cursor position
void fbPrintString(const char* str, uint32_t color, int scale) {
    if (!str) return;
    
    while (*str) {
        fbPrintChar(*str++, color, scale);
    }
}

// Function to clear the screen
void fbClearScreen() {
    if (!fb) return;
    
    uint8_t* fb_ptr = (uint8_t*)(fb->Address);
    for (int i = 0; i < fb->Width * fb->Height * 4; i++) {
        fb_ptr[i] = 0;
    }
    fb_cursor_x = 0;
    fb_cursor_y = 0;
}

// Function to set cursor position
void fbSetCursor(int x, int y) {
    fb_cursor_x = x;
    fb_cursor_y = y;
}

void SetFramebuffer(Framebuffer* _fb) {
    fb = _fb;
}

void SetFont(void* _font) {
    font = _font;
}

// Direct framebuffer pixel placement
void putpixel(unsigned int x, unsigned int y, unsigned int color) {
    if (x >= fb->Width || y >= fb->Height) return;
    
    uint8_t* pixel = (uint8_t*)(fb->Address) + (4*fb->PPSL*y + 4*x);
    pixel[0] = (color & 0xFF);
    pixel[1] = (color & 0xFF00) >> 8;
    pixel[2] = (color & 0xFF0000) >> 16;
    pixel[3] = (color & 0xFF000000) >> 24;
}

// Modified Window class implementation to draw directly to framebuffer
Window::Window() {}

Window::Window(int x, int y, int width, int height) {
    this->x = x;
    this->y = y;
    this->Width = width;
    this->Height = height;
    
    // Clear the window area
    DrawRectangle(0, 0, width, height, 0);
}

void Window::Close() {
    DrawRectangle(0, 0, Width, Height, 0);
}

void Window::Draw(int x, int y, int color) {
    if (x < 0 || x >= Width || y < 0 || y >= Height) return;
    
    // Calculate global coordinates
    int global_x = this->x + x;
    int global_y = this->y + y;
    
    if (global_x >= 0 && global_x < fb->Width && global_y >= 0 && global_y < fb->Height) {
        uint8_t* pixel = (uint8_t*)(fb->Address) + (4*fb->PPSL*global_y + 4*global_x);
        pixel[0] = (color & 0xFF);
        pixel[1] = (color & 0xFF00) >> 8;
        pixel[2] = (color & 0xFF0000) >> 16;
        pixel[3] = (color & 0xFF000000) >> 24;
    }
}

void Window::drawChar(char ch, int x, int y, unsigned long long color, int scale) {
    // No more character filtering - all supported characters will be rendered
    uint8_t idx = ASCII_TO_FONT(ch);
    if (ch == '\r') {
        current_x -= 8*scale+1;
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

// Rest of the Window methods (drawString, DrawRectangle, etc.) remain the same
// but they'll use the Draw method which now writes directly to the framebuffer
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
    // No need to swap with direct framebuffer access
    // This is kept for API compatibility
}