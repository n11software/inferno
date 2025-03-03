#pragma once

#include <stdint.h>
#include <Inferno/types.h>

size_t strlen(const char* str);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t n);
char* strtok(char* str, const char* delimiters);
char* strcpy(char* dest, const char* src);
char* strchr(const char* str, int c);
char* strrchr(const char* str, int c);
extern void* memcpy(void* dest, const void* src, size_t n);
