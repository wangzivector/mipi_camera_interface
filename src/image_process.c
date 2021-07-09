#include "image_process.h"
#define mask32(BYTE) (*(uint32_t *)(uint8_t [4]){ [BYTE] = 0xff })

unsigned char * display_buffer;

typedef enum {
    MIDDLE_COM_SAVE,
    FINAL_SEP_SAVE,
    FINAL_COM_SAVE,

} IMAGE_SAVE_MODE;

IMAGE_SAVE_MODE save_mode  = MIDDLE_COM_SAVE;


struct buffer tempsave[300];
int tempsave_index = 0;
FILE *fp_jpg;

int StartJpgFile(void)
{
    tempsave_index = 0;

    switch (save_mode){
        case MIDDLE_COM_SAVE:
        // break;
        case FINAL_COM_SAVE:
            fp_jpg = fopen("./image/images_0v9281.raw", "wb");
            if(!fp_jpg)
            {
                printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
                return 0;
            }
        break;

        case FINAL_SEP_SAVE:
        break;

        default: printf("wrong mode!");
    }
}

int LoadJpgFile(const struct buffer *buf_img){
    struct timeval begin, end;
    gettimeofday(&begin, 0);

    switch (save_mode){
        case MIDDLE_COM_SAVE:
            fwrite(buf_img->start, buf_img->length, 1, fp_jpg);
            // sync();
        break;

        case FINAL_SEP_SAVE:
        // break;
        case FINAL_COM_SAVE:
            tempsave[tempsave_index].start = malloc(buf_img->length);
            tempsave[tempsave_index].length = buf_img->length;
            memcpy(tempsave[tempsave_index].start, buf_img->start,buf_img->length);
            tempsave_index ++;
        break;

        default: printf("wrong mode!");
    }
        gettimeofday(&end, 0);
        printf("\nLoadJpgFile---- time : %.02f ms\n", ((end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)*1e-6)*1000.0);
}

int FinishJpgFile(void)
{
    int free_index;
    char picname[100];
    struct timeval begin, end;
    gettimeofday(&begin, 0);

    for(free_index = 0; free_index < tempsave_index; free_index ++)
    {
        switch (save_mode){
            case MIDDLE_COM_SAVE:
            break;

            case FINAL_SEP_SAVE:
                sprintf(picname, "%sov9281_%d*%d_%03d.bmp",save_folder, frame_width, frame_height, free_index);
                printf(" image saved: %s\n", picname);
                GenBmpFile(tempsave[free_index].start, 8, frame_width,frame_height, picname); 
            break;

            case FINAL_COM_SAVE:
                printf("\n %d writing raw... %d   ", free_index,
                    fwrite(tempsave[free_index].start, tempsave[free_index].length, 1, fp_jpg));
                // sync();

            break;

            default: printf("wrong mode!");
        }

        gettimeofday(&end, 0);
        printf("---- time : %.02f ms\n", ((end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)*1e-6)*1000.0);
        gettimeofday(&begin, 0);

        free(tempsave[free_index].start);
    }

    switch (save_mode){
        case MIDDLE_COM_SAVE:
        // break;
        case FINAL_COM_SAVE:
            printf("\n closed composed files. \n");
            fclose(fp_jpg);
        break;

        case FINAL_SEP_SAVE:
        break;

        default: printf("wrong mode!");
    }
    
}

// ---
static _Bool SDL_process(const struct buffer *buf_img) // here you should replace the buffer from framebuffer to displaybuffer
{
    int x, y;
    int ByteperPix;
    int BiasofPix0, BiasofPix1, BiasofPix2;

    for(SDL_Event event; SDL_PollEvent(&event);)
        if(event.type == SDL_QUIT) return 0;

    if (buf_img->length == frame_width*frame_height)

        memcpy(display_buffer, buf_img->start, buf_img->length);
    else if (buf_img->length == 3*frame_width*frame_height)
        memcpy(display_buffer, buf_img->start, buf_img->length);

    return 1;
}

int SDL_display_init(int bitperpix)
{
        SDL_Init(SDL_INIT_VIDEO);

        /* Get a 640x480, 24-bit software screen surface */
        SDL_scr = SDL_SetVideoMode(frame_width, frame_height, 24, SDL_HWSURFACE);
        assert(SDL_scr);
}

int SDL_display(const unsigned char *pData, int bitperpix)
{
    int x, y;
    int ByteperPix;
    int BiasofPix0, BiasofPix1, BiasofPix2; 
    if(bitperpix == 24)
    {
        ByteperPix = 3;
        BiasofPix0 = 0;
        BiasofPix1 = 1;
        BiasofPix2 = 2;
    }
    else if(bitperpix == 8){
        ByteperPix = 1;
        BiasofPix0 = 0;
        BiasofPix1 = 0;
        BiasofPix2 = 0;   
    }else
    {
        return -1;
    }

    /* Ensures we have exclusive access to the pixels */
    SDL_LockSurface(SDL_scr);
    
    for(y = 0; y < SDL_scr->h; y++)
        for(x = 0; x < SDL_scr->w; x++)
        {
            // #define pel(surf, x, y, rgb) ((unsigned char *)(surf->pixels))[y*(surf->pitch)+x*3+rgb]
            pel(SDL_scr, x, y, 0) = *(pData+((y*frame_width)+x)*ByteperPix +BiasofPix0); //red
            pel(SDL_scr, x, y, 1) = *(pData+((y*frame_width)+x)*ByteperPix +BiasofPix1); //green
            pel(SDL_scr, x, y, 2) = *(pData+((y*frame_width)+x)*ByteperPix +BiasofPix2); //blue
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


/// second edition


static _Bool SDL_init_app(const char * name, SDL_Surface * icon, uint32_t flags)
{
    atexit(SDL_Quit);
    if(SDL_Init(flags) < 0)
        return 0;

    SDL_WM_SetCaption(name, name);
    SDL_WM_SetIcon(icon, NULL);

    return 1;
}

static uint8_t * SDL_init_data(uint8_t * data)
{
    memset(data, 0xff, frame_width*frame_height*3);
    // for(size_t i = frame_width * frame_height * 3; i--; )
    //     data[i] = (i % 3 == 0) ? (i / 3) % frame_width :
    //         (i % 3 == 1) ? (i / 3) / frame_width : 0;

    return data;
}


static void SDL_render(SDL_Surface * sf)
{
    SDL_Surface * screen = SDL_GetVideoSurface();
    if(SDL_BlitSurface(sf, NULL, screen, NULL) == 0)
        SDL_UpdateRect(screen, 0, 0, 0, 0);
}

static int SDL_filter(const SDL_Event * event)
{ return event->type == SDL_QUIT; }


int SDL_init(void)
{
    display_buffer = malloc(frame_width*frame_height*3);
    _Bool ok =
        SDL_init_app("SDL image display", NULL, SDL_INIT_VIDEO) &&
        SDL_SetVideoMode(frame_width, frame_height, 3*8, SDL_HWSURFACE);

    assert(ok);

    data_sf = SDL_CreateRGBSurfaceFrom(
    SDL_init_data(display_buffer), frame_width, frame_height, 3*8, frame_width * 3,
    mask32(0), mask32(0), mask32(0), 0);
    SDL_SetEventFilter(SDL_filter);
}

int SDL_display_process(const struct buffer *buf_img)
{
    SDL_process(buf_img); 
    // SDL_Delay(10);
    SDL_render(data_sf);
    return 0;
}

void SDL_deinit(void)
{
    free(display_buffer);
}

//------------save image picture captured--------///

int GenBmpFile(const unsigned char *pData, unsigned char bitCountPerPix, unsigned int width, unsigned int height, const char *filename)
{
    int rgbd_c;
    int sizeof_RGBQUAD = 0;
    FILE *fp = fopen(filename, "wb");
    if(!fp)
    {
        printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
        return 0;
    }
    unsigned int bmppitch = ((width*bitCountPerPix + 31) >> 5) << 2;
    unsigned int filesize = bmppitch*height;
    // unsigned int filesize = width*height*3;
    
    if(bitCountPerPix == 8) sizeof_RGBQUAD = 4;

    BITMAPFILE bmpfile;
    bmpfile.bfHeader.bfType = 0x4D42;
    bmpfile.bfHeader.bfReserved1 = 0;
    bmpfile.bfHeader.bfReserved2 = 0;
    bmpfile.bfHeader.bfSize = filesize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof_RGBQUAD*256;
    bmpfile.bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof_RGBQUAD*256;
 
    bmpfile.biInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biWidth = width;
    bmpfile.biInfo.bmiHeader.biHeight = - height;
    bmpfile.biInfo.bmiHeader.biPlanes = (int)(bitCountPerPix/8);
    // bmpfile.biInfo.bmiHeader.biPlanes = 3;
    bmpfile.biInfo.bmiHeader.biBitCount = bitCountPerPix;
    bmpfile.biInfo.bmiHeader.biCompression = 0;
    bmpfile.biInfo.bmiHeader.biSizeImage = filesize;
    bmpfile.biInfo.bmiHeader.biXPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biYPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biClrUsed = 0;
    bmpfile.biInfo.bmiHeader.biClrImportant = 0;
 
    fwrite(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, fp);

    if(bitCountPerPix == 8){
        RGBQUAD rgbquad[256];
        for(rgbd_c = 0; rgbd_c<256; rgbd_c++)
        {
            rgbquad[rgbd_c].rgbBlue = rgbd_c;
            rgbquad[rgbd_c].rgbGreen = rgbd_c;
            rgbquad[rgbd_c].rgbRed = rgbd_c;
            rgbquad[rgbd_c].rgbReserved = 0;
        }
        rgbd_c = fwrite(&rgbquad, sizeof(RGBQUAD), 256, fp);
    }
    fwrite(pData,filesize,1, fp);
    sync();
    fclose(fp);
 
    return 1;
}