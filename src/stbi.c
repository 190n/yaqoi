#define STB_IMAGE_IMPLEMENTATION

#define STBI_NO_STDIO
#define STBI_FAILURE_USERMSG

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#define STBI_ONLY_GIF
#define STBI_ONLY_PIC
#define STBI_ONLY_PNM

#include <stddef.h>

// defined in zig land
void *stbiMalloc(size_t size);
void *stbiRealloc(void *ptr, size_t new_size);
void stbiFree(void *ptr);

#define STBI_MALLOC(size)           stbiMalloc(size)
#define STBI_REALLOC(ptr, new_size) stbiRealloc(ptr, new_size)
#define STBI_FREE(ptr)              stbiFree(ptr)

#include <stb_image.h>
