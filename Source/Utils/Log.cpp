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

	kprintf("[INFO] %s: ", subsystem);
	for (const char* p = message; *p; ++p) {
		if (*p == '%') {
			++p;
			switch (*p) {
			case 'd':
				kprintf("%d", va_arg(args, int));
				break;
			case 's' :
				kprintf("%s", va_arg(args, char*));
				break;
			case 'm' :
				kprintf("%m", va_arg(args, char*));
				break;
			case 'M' :
				kprintf("%M", va_arg(args, char*));
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

	kprintf("[ERROR] %s: ", subsystem);
	for (const char* p = message; *p; ++p) {
		if (*p == '%') {
			++p;
			switch (*p) {
			case 'd':
				kprintf("%d", va_arg(args, int));
				break;
			case 's':
				kprintf("%s", va_arg(args, char*));
				break;
			case 'X':
				kprintf("%X", va_arg(args, int));
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

	kprintf("[WARNING] %s: ", subsystem);
	for (const char* p = message; *p; ++p) {
		if (*p == '%') {
			++p;
			switch (*p) {
			case 'd':
				kprintf("%d", va_arg(args, int));
				break;
			case 's':
				kprintf("%s", va_arg(args, char*));
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

	kprintf("[DEBUG] %s: ", subsystem);
	for (const char* p = message; *p; ++p) {
		if (*p == '%') {
			++p;
			switch (*p) {
			case 'd':
				kprintf("%d", va_arg(args, int));
				break;
			case 's':
				kprintf("%s", va_arg(args, char*));
				break;
			case 'x':
				kprintf("%x", va_arg(args, int));
				break;
			}
		} else {
			kputchar(*p);
		}
	}
	kprintf("\n\r");

	va_end(args);
}
