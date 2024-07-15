#pragma once
#include <Drivers/Graphics/Framebuffer.h>
#include <Memory/Paging.h>

void SetFramebuffer(Framebuffer* _fb);
void SetFont(void* _font);
void test();