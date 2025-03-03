#include <Inferno/string.h>

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Add a strncmp implementation
int strncmp(const char* str1, const char* str2, size_t n) {
    if (n == 0) {
        return 0;
    }
    
    while (*str1 && (*str1 == *str2) && --n) {
        str1++;
        str2++;
    }
    
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Implement strlen if it's not already defined
size_t strlen(const char* str) {
    const char* s = str;
    while (*s) {
        s++;
    }
    return s - str;
}

// Add static buffer for strtok
static char* strtok_next = 0;

// Add a strtok implementation
char* strtok(char* str, const char* delimiters) {
    char* token_start;
    
    // Determine start point
    if (str != 0) {
        // New string
        token_start = str;
    } else {
        // Continue from previous
        token_start = strtok_next;
    }
    
    // Return NULL if there are no more tokens
    if (token_start == 0) {
        return 0;
    }
    
    // Find a token
    char* token_end = token_start;
    while (*token_end) {
        // Check if this character is a delimiter
        const char* delim = delimiters;
        bool is_delim = false;
        
        while (*delim) {
            if (*token_end == *delim) {
                is_delim = true;
                break;
            }
            delim++;
        }
        
        if (is_delim) {
            break;
        }
        
        token_end++;
    }
    
    // Terminate the token and set next starting point
    if (*token_end) {
        *token_end = '\0';
        strtok_next = token_end + 1;
    } else {
        strtok_next = 0;
    }
    
    return token_start;
}

// Add strcpy implementation if it's not already defined
char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}

// Add strchr implementation
char* strchr(const char* str, int c) {
    while (*str != 0) {
        if (*str == c) {
            return (char*)str;
        }
        str++;
    }
    return 0;
}

// Add strrchr implementation (find last occurrence of character in string)
char* strrchr(const char* str, int c) {
    const char* last = NULL;
    
    // If the string is empty, return NULL
    if (!str) {
        return NULL;
    }
    
    // Search for all occurrences and keep track of the last one
    while (*str) {
        if (*str == c) {
            last = str;
        }
        str++;
    }
    
    // Also check the null terminator if c is 0
    if (c == 0) {
        return (char*)str;
    }
    
    // Return the last occurrence found (or NULL if none)
    return (char*)last;
}

// memcpy is defined in Memory/Mem_.cpp
