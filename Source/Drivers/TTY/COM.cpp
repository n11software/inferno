//========= Copyright N11 Software, All rights reserved. ============//
//
// File: COM.cpp
// Purpose:
// Maintainer: FiReLScar
//
//===================================================================//

#include <Inferno/stdarg.h>
#include <Inferno/stdint.h>
#include <Inferno/IO.h>
#include <Drivers/TTY/COM.h>
#include <Drivers/Graphics/Framebuffer.h>
#include <Drivers/TTY/TTY.h>
#include <Drivers/TTY/VGA_Font.h>  // Add this include
#include <Memory/Mem_.hpp>

#include <Inferno/Log.h>

bool COM1Active = false;
Framebuffer* COM1Framebuffer;
Window COM1Window;
void* font_;
bool gui_;

void setFramebuffer(Framebuffer* fb, bool gui) {
	gui_ = gui;
	COM1Framebuffer = fb;
}

void setFont(void* f) {
	font_ = f;
}

Window window;
int width = 400;
int height = 300;
int x,y;

char *strcpy(char* dest, const char* src) {
	if (dest == nullptr) {
		return nullptr;
	}

	char *ptr = dest;

	while (*src != '\0') {
		*dest = *src;
		dest++;
		src++;
	}

	*dest = '\0';

	return ptr;
}

#define LINE_HEIGHT 12
#define CONSOLE_COLS 41  // (400 - 26) / 8 to stay within width-26
#define CONSOLE_ROWS 21  // (300 - 48) / LINE_HEIGHT to stay within height-8-14-26
char consoleBuffer[CONSOLE_ROWS][CONSOLE_COLS + 1];  // +1 for null terminator
int currentRow = 0;
int currentCol = 0;

// Add color buffer per line
uint32_t lineColors[CONSOLE_ROWS] = {0};

void clearConsoleBuffer() {
    for(int i = 0; i < CONSOLE_ROWS; i++) {
        for(int j = 0; j < CONSOLE_COLS; j++) {
            consoleBuffer[i][j] = ' ';
        }
        consoleBuffer[i][CONSOLE_COLS] = '\0';  // Null terminate each line
    }
    currentRow = 0;
    currentCol = 0;
    for(int i = 0; i < CONSOLE_ROWS; i++) {
        lineColors[i] = gui_ ? 0x000000 : 0xFFFFFF;  // Default: black for GUI, white for non-GUI
    }
    // Clear entire console area
    window.DrawRectircle(8, 37, width-26, height-8-14-26, 7, 0xf4f4f4);
    window.Swap();
    swapBuffers();
}

void redrawCurrentLine() {
    // Clear the entire line area including margins
    int y = 37 + (currentRow * LINE_HEIGHT);
    window.DrawRectircle(8, y, width-26, LINE_HEIGHT, 7, 0xf4f4f4);
    
    // Ensure null termination
    consoleBuffer[currentRow][currentCol] = '\0';
    
    // Only draw if there's content
    if(currentCol > 0) {
        window.drawString(consoleBuffer[currentRow], 15, y, 0x000000, 1);
    }
    
    window.Swap();
    swapBuffers();
}

void redrawFromLine(int startRow) {
    // Clear entire console area first
    window.DrawRectircle(8, 37 + (startRow * LINE_HEIGHT), width-26, height-8-14-26-(startRow * LINE_HEIGHT), 7, 0xf4f4f4);
    
    // Draw each line as a string
    for(int i = startRow; i <= currentRow; i++) {
        if(consoleBuffer[i][0] != '\0' && consoleBuffer[i][0] != ' ') {
            window.drawString(consoleBuffer[i], 15, 37 + (i * LINE_HEIGHT), 0x000000, 1);
        }
    }
    window.Swap();
    swapBuffers();
}

void scrollBuffer() {
    // Move text and colors up
    for(int i = 0; i < CONSOLE_ROWS - 1; i++) {
        strcpy(consoleBuffer[i], consoleBuffer[i + 1]);
        lineColors[i] = lineColors[i + 1];
    }
    // Clear the last line
    for(int j = 0; j < CONSOLE_COLS; j++) {
        consoleBuffer[CONSOLE_ROWS - 1][j] = ' ';
    }
    consoleBuffer[CONSOLE_ROWS - 1][CONSOLE_COLS] = '\0';
    lineColors[CONSOLE_ROWS - 1] = gui_ ? 0x000000 : 0xFFFFFF;  // Reset to default color
    redrawFromLine(0);
}

void InitializeSerialDevice() {
    outb(0x3f8 + 1, 0x00);
    outb(0x3f8 + 3, 0x80);
    outb(0x3f8 + 0, 0x03);
    outb(0x3f8 + 1, 0x00);
    outb(0x3f8 + 3, 0x03);
    outb(0x3f8 + 2, 0xC7);
    outb(0x3f8 + 4, 0x0B);
    outb(0x3f8 + 4, 0x1E);
    outb(0x3f8 + 0, 0xAE);
    if (inb(0x3f8 + 0) != 0xAE) return;
    outb(0x3f8 + 4, 0x0F);
    COM1Active = true;
    prInfo("kernel", "serial device initalized");

    if (!gui_) {
        // Clear entire framebuffer to black
        uint32_t* fb_addr = (uint32_t*)COM1Framebuffer->Address;
        for (int i = 0; i < COM1Framebuffer->Width * COM1Framebuffer->Height; i++) {
            fb_addr[i] = 0;  // Black background
        }
        return;
    }

    if (COM1Framebuffer != nullptr) {
        prInfo("kernel", "serial framebuffer initialized");
    } else return;
	if (font_ != nullptr) {
		prInfo("kernel", "serial font initialized");
	} else return;
	// Make window
	SetFont(font_);
	SetFramebuffer(COM1Framebuffer);
	// create window
    x = (COM1Framebuffer->Width - width) / 2;
    y = (COM1Framebuffer->Height - height) / 2;
	window = Window(x, y, width, height);
	SetFramebuffer(COM1Framebuffer);
	prInfo("tty", "initialized TTY");
	void* backBuffer = (void*)0x100000;
    memset(backBuffer, 0x42, COM1Framebuffer->Width * COM1Framebuffer->Height * 4);

	window.DrawRectircle(0, 0, width, height, 7, 0xffffff);
    window.DrawRectircle(8, 14+16, width-16, height-8-14-16, 7, 0xf4f4f4);
	// add title
	window.drawString("Inferno", 8+((width-16)/2)-30, 9, 0x000000, 2, true);
	
	window.Swap();
	swapBuffers();
    clearConsoleBuffer();

    if (gui_) {
        window.DrawRectircle(0, 0, width, height, 7, 0xFFFFFF);  // White background
		window.DrawFilledCircle(14, 14, 6, 0xff0000);
		window.DrawFilledCircle(14+16, 14, 6, 0x00f000);
		window.DrawFilledCircle(14+16+16, 14, 6, 0xffd629);
        window.DrawRectircle(8, 14+16, width-16, height-8-14-16, 7, 0xF4F4F4);  // Light gray console area
        window.drawString("Inferno", 8+((width-16)/2)-30, 9, 0x000000, 2, true);
        window.Swap();
        swapBuffers();
    }
}

int SerialRecieveEvent() { return inb(0x3f8 + 5) & 1; }

char AwaitSerialResponse() {
    while (SerialRecieveEvent() == 0);
    return inb(0x3f8);
}

int SerialWait() { return inb(0x3f8 + 5) & 0x20; }

char* ConsoleOutput;
int consoleX = 5;
int consoleY = 5;

// Add these variables for non-GUI mode
int screen_x = 0;
int screen_y = 0;

// Add current color for non-GUI mode
uint32_t current_color = 0xFFFFFF;

// Add these at file scope
static bool in_escape_seq = false;
static int escape_code = 0;

void directPutchar(char c) {
    if (c == '\033') {
        in_escape_seq = true;
        escape_code = 0;
        return;
    }

    if (in_escape_seq) {
        if (c >= '0' && c <= '9') {
            escape_code = escape_code * 10 + (c - '0');
            return;
        }
        if (c == 'm') {
            switch (escape_code) {
                case 0: current_color = 0xFFFFFF; break;
                case 31: current_color = 0xFF0000; break;
                case 32: current_color = 0x00FF00; break;
                case 33: current_color = 0xFFFF00; break;
                case 34: current_color = 0x0000FF; break;
                case 35: current_color = 0xFF00FF; break;
                case 36: current_color = 0x00FFFF; break;
                case 37: current_color = 0xFFFFFF; break;
            }
            in_escape_seq = false;
            return;
        }
        // If we get here with unknown escape sequence, ignore it
        if (c == '[') return;
        in_escape_seq = false;
    }

    // Draw character only if not in escape sequence
    const int CHAR_WIDTH = 8;
    const int CHAR_HEIGHT = 12;
    const int MAX_X = COM1Framebuffer->Width - CHAR_WIDTH;
    const int MAX_Y = COM1Framebuffer->Height - CHAR_HEIGHT;

    if (c == '\n') {
        screen_x = 0;
        screen_y += CHAR_HEIGHT;
        if (screen_y >= MAX_Y) {
            // Scroll the screen up by copying lines
            uint32_t* fb_addr = (uint32_t*)COM1Framebuffer->Address;
            int line_width = COM1Framebuffer->PPSL;
            
            // Copy each line up
            for (int y = CHAR_HEIGHT; y < COM1Framebuffer->Height; y++) {
                for (int x = 0; x < COM1Framebuffer->Width; x++) {
                    fb_addr[(y - CHAR_HEIGHT) * line_width + x] = fb_addr[y * line_width + x];
                }
            }
            
            // Clear the new line
            for (int y = MAX_Y; y < COM1Framebuffer->Height; y++) {
                for (int x = 0; x < COM1Framebuffer->Width; x++) {
                    fb_addr[y * line_width + x] = 0;
                }
            }
            
            screen_y = MAX_Y - CHAR_HEIGHT;
        }
        return;
    }

    // Draw character
    uint8_t* fb_addr = (uint8_t*)COM1Framebuffer->Address;
    uint8_t idx = ASCII_TO_FONT(c);

	if (c == '\r') {
		screen_x = 0;
		return;
	}
    
    for (int row = 0; row < 8; row++) {
        uint8_t line = font8x8[idx][row];
        for (int col = 0; col < 8; col++) {
            if (line & (1 << (7 - col))) {
                int x = screen_x + col;
                int y = screen_y + row;
                if (x < COM1Framebuffer->Width && y < COM1Framebuffer->Height) {
                    uint32_t* pixel = (uint32_t*)(fb_addr + (y * COM1Framebuffer->PPSL * 4) + (x * 4));
                    *pixel = current_color;  // Use current color instead of fixed white
                }
            }
        }
    }
    
    screen_x += CHAR_WIDTH;
    if (screen_x >= MAX_X) {
        screen_x = 0;
        screen_y += CHAR_HEIGHT;
        if (screen_y >= MAX_Y) {
            screen_y = 0;  // Wrap to top if we run out of scroll space
        }
    }
}

void kputchar(char a) {
    if (!COM1Active) return;
    while (SerialWait() == 0);
    outb(0x3f8, a);

    static bool in_escape = false;
    static int code = 0;

    if (!gui_) {
        if (COM1Framebuffer != nullptr) {
            directPutchar(a);
        }
        return;
    }

    if (in_escape) {
        if (a >= '0' && a <= '9') {
            code = code * 10 + (a - '0');
        } else if (a == 'm') {
            switch (code) {
                case 0: lineColors[currentRow] = gui_ ? 0x000000 : 0xFFFFFF; break; // Reset to default
                case 31: lineColors[currentRow] = 0xFF0000; break;
                case 32: lineColors[currentRow] = 0x00FF00; break;
                case 33: lineColors[currentRow] = 0xFFFF00; break;
                case 34: lineColors[currentRow] = 0x0000FF; break;
                case 35: lineColors[currentRow] = 0xFF00FF; break;
                case 36: lineColors[currentRow] = 0x00FFFF; break;
                case 37: lineColors[currentRow] = 0xFFFFFF; break;
            }
            in_escape = false;
            code = 0;
        }
        return;
    }

    if (a == '\033') {
        in_escape = true;
        code = 0;
        return;
    }

    if (a == '\n') {
        // Draw current line with its color
        window.DrawRectircle(8, 37 + (currentRow * LINE_HEIGHT), width-26, LINE_HEIGHT, 7, 0xf4f4f4);
        window.drawString(consoleBuffer[currentRow], 15, 37 + (currentRow * LINE_HEIGHT), lineColors[currentRow], 1);
        currentRow++;
        currentCol = 0;
        lineColors[currentRow] = lineColors[currentRow-1];  // Inherit color from previous line
        if(currentRow >= CONSOLE_ROWS) {
            scrollBuffer();
            currentRow = CONSOLE_ROWS - 1;
        }
        window.Swap();
        swapBuffers();
    } else {
        if(currentCol >= CONSOLE_COLS-1) {
            // Line wrap
            consoleBuffer[currentRow][currentCol] = a;
            consoleBuffer[currentRow][CONSOLE_COLS] = '\0';
            
            // Draw the completed line
            window.DrawRectircle(8, 37 + (currentRow * LINE_HEIGHT), width-26, LINE_HEIGHT, 7, 0xf4f4f4);
            window.drawString(consoleBuffer[currentRow], 15, 37 + (currentRow * LINE_HEIGHT), lineColors[currentRow], 1);
            
            currentRow++;
            currentCol = 0;
            
            if(currentRow >= CONSOLE_ROWS) {
                scrollBuffer();
                currentRow = CONSOLE_ROWS - 1;
            }
            
            window.Swap();
            swapBuffers();
        } else {
            // Just store the character without drawing
            consoleBuffer[currentRow][currentCol++] = a;
        }
    }

    if (gui_) {
        window.drawString(consoleBuffer[currentRow], 15, 37 + (currentRow * LINE_HEIGHT), lineColors[currentRow], 1);
    }
}

char* strchr(const char* str, int c) {
    while (*str != 0) {
        if (*str == c) return (char*)str;
        str++;
    }
    return 0;
}

int strlen(const char* str) {
    const char* char_ptr;
    const unsigned long int* longword_ptr;
    unsigned long int longword, himagic, lomagic;
    for (char_ptr = str; ((unsigned long int)char_ptr & (sizeof(longword) - 1)) != 0; char_ptr++) if (*char_ptr == '\0') return char_ptr - str;
    longword_ptr = (unsigned long int*)char_ptr;
    himagic = 0x80808080L;
    lomagic = 0x01010101L;
    if (sizeof(longword) > 4) {
        himagic = ((himagic << 16) << 16) | himagic;
        lomagic = ((lomagic << 16) << 16) | lomagic;
    }
    if (sizeof(longword) > 8) asm("int $0x0E");
    for (;;) {
        longword = *longword_ptr++;
        if (((longword - lomagic) & ~longword & himagic) != 0) {
            const char* cp = (const char*)(longword_ptr - 1);
            if (cp[0] == 0) return cp - str;
            if (cp[1] == 0) return cp - str + 1;
            if (cp[2] == 0) return cp - str + 2;
            if (cp[3] == 0) return cp - str + 3;
            if (sizeof(longword) > 4) {
                if (cp[4] == 0) return cp - str + 4;
                if (cp[5] == 0) return cp - str + 5;
                if (cp[6] == 0) return cp - str + 6;
                if (cp[7] == 0) return cp - str + 7;
            }
        }
    }
}

struct PrintData {
    bool is_format = false, alt = false, right = false;
    int size = 0, spacing = 0, sign = 0, base = 0, baseprefix = 0, printedchrs = 0;
    char fill = ' ';
    long long num = 0;
};

void _fill(PrintData* pd, int spaces) {
    char f = pd->fill;
    for (int i = 0; i < spaces; i++) {
        kputchar(f);
        pd->printedchrs++;
    }
}

void _reset(PrintData* pd) {
    pd->is_format = false;
    pd->size = 0;
    pd->num = 0;
    pd->sign = 0;
    pd->spacing = 0;
    pd->right = false;
    pd->fill = ' ';
    pd->base = 0;
    pd->baseprefix = 0;
    pd->alt = false;
}

void _print(PrintData* pd, const char* str) {
    for (int i = 0; str[i] != 0; i++) kputchar(str[i]);

    pd->printedchrs += strlen(str);
}

void _kprintf(PrintData* pd, const char* prefix, const char* prefix2, const char* str) {
    int len = strlen(prefix) + strlen(prefix2) + strlen(str);
    int spaces = pd->spacing;
    if (spaces > len) spaces -= len;
    else spaces = 0;
    if (spaces > 0 && pd->right == 0 && pd->fill != '0') _fill(pd, spaces);
    for (int i = 0; i < strlen(prefix); i++) {
        kputchar(prefix[i]);
        pd->printedchrs++;
    }
    for (int i = 0; i < strlen(prefix2); i++) {
        kputchar(prefix2[i]);
        pd->printedchrs++;
    }
    if (spaces > 0 && pd->right == 0 && pd->fill == '0') _fill(pd, spaces);
    _print(pd, str);
    if (spaces > 0 && pd->right != 0) _fill(pd, spaces);
}

int kprintf(const char* fmt, ...) {
    if (!COM1Active) InitializeSerialDevice();
    PrintData pd;
    int chars;
    va_list args;
    va_start(args, fmt);
    for (int i = 0; fmt[i]; i++) {
        if (!pd.is_format && fmt[i] != '%') kputchar(fmt[i]);
        else if (!pd.is_format) pd.is_format = true;
        else if (strchr("#-10123456789", fmt[i])) {
            switch (fmt[i]) {
                case '#':
                    pd.alt = true;
                    break;
                case '-':
                    pd.right = true;
                    break;
                case 'l':
                    if (pd.size == 1)
                        pd.size = 2;
                    else
                        pd.size = 1;
                    break;
                case '0':
                    if (pd.spacing > 0)
                        pd.spacing *= 10;
                    else
                        pd.fill = '0';
                    break;
                default:
                    pd.spacing *= 10 + (fmt[i] - '0');
                    break;
            }
        } else if (strchr("doupx", fmt[i])) {
            // Get format
            if (fmt[i] == 'p') pd.num = (unsigned long long)va_arg(args, void*);
            else if (fmt[i] == 'd') {
                long long signednum = 0;
                switch (pd.size) {
                    case 0:
                        signednum = va_arg(args, int);
                        break;
                    case 1:
                        signednum = va_arg(args, long);
                        break;
                    case 2:
                        signednum = va_arg(args, long long);
                        break;
                }

                if (signednum < 0) {
                    pd.sign = -1;
                    pd.num = -signednum;
                } else pd.num = signednum;
            } else {
                switch (pd.size) {
                    case 0:
                        pd.num = va_arg(args, unsigned int);
                        break;
                    case 1:
                        pd.num = va_arg(args, unsigned long);
                        break;
                    case 2:
                        pd.num = va_arg(args, unsigned long long);
                        break;
                }
            }

            // Get base
            switch (fmt[i]) {
                case 'd':
                case 'u':
                    pd.base = 10;
                    break;
                case 'x':
                case 'p':
                    pd.base = 16;
                    break;
                case 'o':
                    pd.base = 8;
                    break;
            }
            if (pd.alt || fmt[i] == 'p') pd.baseprefix = 1;
            const char* const digits = "0123456789abcdef";
            char buf[((sizeof(long long) * 8) / 3 + 2)];
            char* x = buf + sizeof(buf) - 1;
            unsigned long long xnum = pd.num;
            const char *bprefix, *sprefix;
            *x--=0;
            do {
                *x = digits[xnum % pd.base];
                x--;
                xnum = xnum / pd.base;
            } while (xnum > 0);
            x++;
            if (pd.baseprefix && pd.base == 16) bprefix = "0x";
            else if (pd.baseprefix && pd.base == 8) bprefix = "0";
            else bprefix = "";
            sprefix = pd.sign ? "-" : "";
            _kprintf(&pd, sprefix, bprefix, x);
            _reset(&pd);
        } else if (fmt[i] == 's') {
            const char* str = va_arg(args, const char*);
            if (str == (void*)0) str = "(null)";
            _kprintf(&pd, "", "", str);
            _reset(&pd);
        } else if (fmt[i] == 'M') {
            unsigned long long* i = va_arg(args, unsigned long long*);
            unsigned char x = 0;
            unsigned long long y = (unsigned long long)i;
            while (y >= 1024) {
                y = y / 1024;
                x++;
            }

            char* sizeType = (char*)"";
            switch (x) {
                case 0:
                    sizeType = (char*)"B";
                    break;
                case 1:
                    sizeType = (char*)"KB";
                    break;
                case 2:
                    sizeType = (char*)"MB";
                    break;
                case 3:
                    sizeType = (char*)"GB";
                    break;
                case 4:
                    sizeType = (char*)"TB";
                    break;
                default:
                    sizeType = (char*)"B";
                    y = (unsigned long long)i;
                    break;
            }
            kprintf("%d%s", y, sizeType);
            // _kprintf(&pd, "", "", str);
            _reset(&pd);
        } else if (fmt[i] == 'm') {
            unsigned long long* i = va_arg(args, unsigned long long*);
            unsigned char x = 0;
            unsigned long long y = (unsigned long long)i;
            while (y >= 1024) {
                y = y / 1024;
                x++;
            }

            char* sizeType = (char*)"";
            switch (x) {
                case 0:
                    sizeType = (char*)"b";
                    break;
                case 1:
                    sizeType = (char*)"KiB";
                    break;
                case 2:
                    sizeType = (char*)"MiB";
                    break;
                case 3:
                    sizeType = (char*)"GiB";
                    break;
                case 4:
                    sizeType = (char*)"TiB";
                    break;
                default:
                    sizeType = (char*)"b";
                    y = (unsigned long long)i;
                    break;
            }
            kprintf("%d%s", y, sizeType);
            // _kprintf(&pd, "", "", str);
            _reset(&pd);
        } else if (fmt[i] == 'h') {
            bool* i = va_arg(args, bool*);
            if ((bool)i) kprintf("true");
            else kprintf("false");
            // _kprintf(&pd, "", "", str);
            _reset(&pd);
        } else {
            char x[2];
            if (fmt[i] == 'c') x[0] = va_arg(args, int);
            else x[0] = fmt[i];
            x[1] = 0;
            _kprintf(&pd, "", "", x);
            _reset(&pd);
        }
    }
    va_end(args);
    return pd.printedchrs;
}
