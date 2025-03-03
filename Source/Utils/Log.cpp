//========= Copyright N11 Software, All rights reserved. ============//
//
// File: log.cpp
// Purpose: Message logging functions
// Maintainer: aristonl
//
//===================================================================//

#include <Inferno/Log.h>

#include <Drivers/TTY/COM.h>
#include <Inferno/stdarg.h>

void prInfo(const char* subsystem, const char* message, ...) {
	va_list args;
	va_start(args, message);

	// change color
	

	kprintf("\033[32m[INFO]\033[0m %s: ", subsystem);
	for (const char* p = message; *p; ++p) {
		if (*p == '%') {
			++p;
			// Check for width/padding specifiers
			int width = 0;
			char padding = ' ';
			
			if (*p == '0') {
				padding = '0';
				++p;
			}
			
			// Read width value
			while (*p >= '0' && *p <= '9') {
				width = width * 10 + (*p - '0');
				++p;
			}
			
			// Check for 'l' or 'll' (long or long long)
			int longCount = 0;
			while (*p == 'l') {
				longCount++;
				++p;
			}
			
			// Now process the actual format character
			switch (*p) {
			case 'd':
				if (longCount >= 2)
					kprintf("%lld", va_arg(args, long long));
				else if (longCount == 1)
					kprintf("%ld", va_arg(args, long));
				else
					kprintf("%d", va_arg(args, int));
				break;
			case 's':
				kprintf("%s", va_arg(args, char*));
				break;
			case 'm':
				kprintf("%m", va_arg(args, char*));
				break;
			case 'M':
				kprintf("%M", va_arg(args, char*));
				break;
			case 'x':
				if (width == 2)
					kprintf("%02x", va_arg(args, int));
				else if (width == 4)
					kprintf("%04x", va_arg(args, int));
				else if (width == 8)
					kprintf("%08x", va_arg(args, int));
				else if (longCount >= 2)
					kprintf("%llx", va_arg(args, unsigned long long));
				else if (longCount == 1)
					kprintf("%lx", va_arg(args, unsigned long));
				else
					kprintf("%x", va_arg(args, unsigned int));
				break;
			case 'u':
				if (longCount >= 2)
					kprintf("%llu", va_arg(args, unsigned long long));
				else if (longCount == 1)
					kprintf("%lu", va_arg(args, unsigned long));
				else
					kprintf("%u", va_arg(args, unsigned int));
				break;
			case 'p':
				kprintf("%p", va_arg(args, void*));
				break;
			default:
				// For unknown formats, just print the character
				kputchar(*p);
				break;
			}
		} else {
			kputchar(*p);
		}
	}
	kprintf("\n\r");

	va_end(args);
}

void prErr(const char* subsystem, const char* message, ...) {
	va_list args;
	va_start(args, message);

	kprintf("\033[31m[ERROR]\033[0m %s: ", subsystem);
	for (const char* p = message; *p; ++p) {
		if (*p == '%') {
			++p;
			// Check for width/padding specifiers
			int width = 0;
			char padding = ' ';
			
			if (*p == '0') {
				padding = '0';
				++p;
			}
			
			// Read width value
			while (*p >= '0' && *p <= '9') {
				width = width * 10 + (*p - '0');
				++p;
			}
			
			// Check for 'l' or 'll' (long or long long)
			int longCount = 0;
			while (*p == 'l') {
				longCount++;
				++p;
			}
			
			// Now process the actual format character
			switch (*p) {
			case 'd':
				if (longCount >= 2)
					kprintf("%lld", va_arg(args, long long));
				else if (longCount == 1)
					kprintf("%ld", va_arg(args, long));
				else
					kprintf("%d", va_arg(args, int));
				break;
			case 's':
				kprintf("%s", va_arg(args, char*));
				break;
			case 'x':
			case 'X':
				if (width == 2)
					kprintf("%02x", va_arg(args, int));
				else if (width == 4)
					kprintf("%04x", va_arg(args, int));
				else if (width == 8)
					kprintf("%08x", va_arg(args, int));
				else if (longCount >= 2)
					kprintf("%llx", va_arg(args, unsigned long long));
				else if (longCount == 1)
					kprintf("%lx", va_arg(args, unsigned long));
				else
					kprintf("%x", va_arg(args, unsigned int));
				break;
			case 'u':
				if (longCount >= 2)
					kprintf("%llu", va_arg(args, unsigned long long));
				else if (longCount == 1)
					kprintf("%lu", va_arg(args, unsigned long));
				else
					kprintf("%u", va_arg(args, unsigned int));
				break;
			case 'p':
				kprintf("%p", va_arg(args, void*));
				break;
			default:
				// For unknown formats, just print the character
				kputchar(*p);
				break;
			}
		} else {
			kputchar(*p);
		}
	}
	kprintf("\n\r");

	va_end(args);
}

void prWarn(const char* subsystem, const char* message, ...) {
	va_list args;
	va_start(args, message);

	kprintf("\033[33m[WARN]\033[0m %s: ", subsystem);
	for (const char* p = message; *p; ++p) {
		if (*p == '%') {
			++p;
			// Check for width/padding specifiers
			int width = 0;
			char padding = ' ';
			
			if (*p == '0') {
				padding = '0';
				++p;
			}
			
			// Read width value
			while (*p >= '0' && *p <= '9') {
				width = width * 10 + (*p - '0');
				++p;
			}
			
			// Check for 'l' or 'll' (long or long long)
			int longCount = 0;
			while (*p == 'l') {
				longCount++;
				++p;
			}
			
			// Now process the actual format character
			switch (*p) {
			case 'd':
				if (longCount >= 2)
					kprintf("%lld", va_arg(args, long long));
				else if (longCount == 1)
					kprintf("%ld", va_arg(args, long));
				else
					kprintf("%d", va_arg(args, int));
				break;
			case 's':
				kprintf("%s", va_arg(args, char*));
				break;
			case 'x':
				if (width == 2)
					kprintf("%02x", va_arg(args, int));
				else if (width == 4)
					kprintf("%04x", va_arg(args, int));
				else if (width == 8)
					kprintf("%08x", va_arg(args, int));
				else if (longCount >= 2)
					kprintf("%llx", va_arg(args, unsigned long long));
				else if (longCount == 1)
					kprintf("%lx", va_arg(args, unsigned long));
				else
					kprintf("%x", va_arg(args, unsigned int));
				break;
			case 'u':
				if (longCount >= 2)
					kprintf("%llu", va_arg(args, unsigned long long));
				else if (longCount == 1)
					kprintf("%lu", va_arg(args, unsigned long));
				else
					kprintf("%u", va_arg(args, unsigned int));
				break;
			case 'p':
				kprintf("%p", va_arg(args, void*));
				break;
			default:
				// For unknown formats, just print the character
				kputchar(*p);
				break;
			}
		} else {
			kputchar(*p);
		}
	}
	kprintf("\n\r");

	va_end(args);
}

void prDebug(const char* subsystem, const char* message, ...) {
	va_list args;
	va_start(args, message);

	kprintf("\033[36m[DEBUG]\033[0m %s: ", subsystem);
	for (const char* p = message; *p; ++p) {
		if (*p == '%') {
			++p;
			// Check for width/padding specifiers
			int width = 0;
			char padding = ' ';
			
			if (*p == '0') {
				padding = '0';
				++p;
			}
			
			// Read width value
			while (*p >= '0' && *p <= '9') {
				width = width * 10 + (*p - '0');
				++p;
			}
			
			// Check for 'l' or 'll' (long or long long)
			int longCount = 0;
			while (*p == 'l') {
				longCount++;
				++p;
			}
			
			// Now process the actual format character
			switch (*p) {
			case 'd':
				if (longCount >= 2)
					kprintf("%lld", va_arg(args, long long));
				else if (longCount == 1)
					kprintf("%ld", va_arg(args, long));
				else
					kprintf("%d", va_arg(args, int));
				break;
			case 's':
				kprintf("%s", va_arg(args, char*));
				break;
			case 'x':
				if (width == 2)
					kprintf("%02x", va_arg(args, int));
				else if (width == 4)
					kprintf("%04x", va_arg(args, int));
				else if (width == 8)
					kprintf("%08x", va_arg(args, int));
				else if (longCount >= 2)
					kprintf("%llx", va_arg(args, unsigned long long));
				else if (longCount == 1)
					kprintf("%lx", va_arg(args, unsigned long));
				else
					kprintf("%x", va_arg(args, unsigned int));
				break;
			case 'u':
				if (longCount >= 2)
					kprintf("%llu", va_arg(args, unsigned long long));
				else if (longCount == 1)
					kprintf("%lu", va_arg(args, unsigned long));
				else
					kprintf("%u", va_arg(args, unsigned int));
				break;
			case 'p':
				kprintf("%p", va_arg(args, void*));
				break;
			default:
				// For unknown formats, just print the character
				kputchar(*p);
				break;
			}
		} else {
			kputchar(*p);
		}
	}
	kprintf("\n\r");

	va_end(args);
}
