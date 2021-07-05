# include "image_process.h"

//------------save image picture captured--------///
int GenBmpFile(const unsigned char *pData, unsigned char bitCountPerPix, unsigned int width, unsigned int height, const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if(!fp)
    {
        printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
        return 0;
    }
    unsigned int bmppitch = ((width*bitCountPerPix + 31) >> 5) << 2;
    unsigned int filesize = bmppitch*height;
    // unsigned int filesize = width*height*3;
 
    BITMAPFILE bmpfile;
    bmpfile.bfHeader.bfType = 0x4D42;
    bmpfile.bfHeader.bfSize = filesize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpfile.bfHeader.bfReserved1 = 0;
    bmpfile.bfHeader.bfReserved2 = 0;
    bmpfile.bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
 
    bmpfile.biInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biWidth = width;
    bmpfile.biInfo.bmiHeader.biHeight = -height;
    bmpfile.biInfo.bmiHeader.biPlanes = 3;
    bmpfile.biInfo.bmiHeader.biBitCount = bitCountPerPix;
    bmpfile.biInfo.bmiHeader.biCompression = 0;
    bmpfile.biInfo.bmiHeader.biSizeImage = filesize;
    bmpfile.biInfo.bmiHeader.biXPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biYPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biClrUsed = 0;
    bmpfile.biInfo.bmiHeader.biClrImportant = 0;
 
    fwrite(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, fp);
    fwrite(pData,filesize,1, fp);
    fclose(fp);
 
    return 1;
}

int GenJpgFile(struct buffer *buffers,const char *filename){
    FILE *fp = NULL;
    fp = fopen(filename, "w");
    if(fp != NULL){
        fwrite(buffers->start, 1,buffers->length, fp);
        sync();
        fclose(fp);
        return 1;
    }
    return 0; 
}

int SDL_display_init(void)
{
        SDL_Init(SDL_INIT_VIDEO);

        /* Get a 640x480, 24-bit software screen surface */
        SDL_scr = SDL_SetVideoMode(frame_width, frame_height, 24, SDL_SWSURFACE);
        assert(SDL_scr);
}

int SDL_display(const unsigned char *pData)
{
    int x, y;

    /* Ensures we have exclusive access to the pixels */
    SDL_LockSurface(SDL_scr);
    
    for(y = 0; y < SDL_scr->h; y++)
        for(x = 0; x < SDL_scr->w; x++)
        {
            // #define pel(surf, x, y, rgb) ((unsigned char *)(surf->pixels))[y*(surf->pitch)+x*3+rgb]
            pel(SDL_scr, x, y, 0) = *(pData+((y*frame_width)+x)*3 +0); //red
            pel(SDL_scr, x, y, 1) = *(pData+((y*frame_width)+x)*3 +1); //green
            pel(SDL_scr, x, y, 2) = *(pData+((y*frame_width)+x)*3 +2); //blue
            // printf("%d", *(pData + (y * frame_width) + x + 0));
        }
    SDL_UnlockSurface(SDL_scr);

    /* Copies the `scr' surface to the _actual_ screen */
    SDL_UpdateRect(SDL_scr, 0, 0, 0, 0);
    return 0;
}

int SDL_display_wait2close(void)
{
    /* Now we wait for an event to arrive */
    while(SDL_WaitEvent(&event))
    {
        /* Any of these event types will end the program */
        if (event.type == SDL_QUIT
         || event.type == SDL_KEYDOWN
         || event.type == SDL_KEYUP)
            break;
    }
    SDL_Quit();
    return EXIT_SUCCESS;
}