
#ifndef IMAGE_PROCESSS_H_
#define IMAGE_PROCESSS_H_

#include <SDL/SDL.h>
#include "parameter.h"
#include "bmp.h"

/* This macro simplifies accessing a given pixel component on a surface. */
#define pel(surf, x, y, rgb) ((unsigned char *)(surf->pixels))[y*(surf->pitch)+x*3+rgb]

union SDL_Event event;
struct SDL_Surface *SDL_scr;

int GenBmpFile(const unsigned char *pData, unsigned char bitCountPerPix, unsigned int width, unsigned int height, const char *filename);
int GenJpgFile(struct buffer *buffers,const char *filename);
int SDL_display_init();
int SDL_display(const unsigned char *pData, int bitperpix);
int SDL_display_wait2close(void);

#endif 
