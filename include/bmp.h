

/*
 * bmp.h
 *
 *  Created on: Apr 26, 2016
 *      Author: anzyelay
 */
 
#ifndef BMP_H_
#define BMP_H_
 
#pragma pack(push, 1)

 
typedef struct tagBITMAPFILEHEADER
{
 unsigned short bfType;
 unsigned int bfSize;
 unsigned short bfReserved1;
 unsigned short bfReserved2;
 unsigned int bfOffBits;
} BITMAPFILEHEADER;
 
typedef struct tagBITMAPINFOHEADER
{
 unsigned int biSize;
 unsigned int biWidth;
 unsigned int biHeight;
 unsigned short biPlanes;
 unsigned short biBitCount;
 unsigned int biCompression;
 unsigned int biSizeImage;
 unsigned int biXPelsPerMeter;
 unsigned int biYPelsPerMeter;
 unsigned int biClrUsed;
 unsigned int biClrImportant;
} BITMAPINFOHEADER;
 
typedef struct tagRGBQUAD
{
 unsigned char rgbBlue;
 unsigned char rgbGreen;
 unsigned char rgbRed;
 unsigned char rgbReserved;
} RGBQUAD;
 
typedef struct tagBITMAPINFO
{
 BITMAPINFOHEADER bmiHeader;
 RGBQUAD bmiColors[1];
} BITMAPINFO;
 
 
typedef struct tagBITMAP
{
 BITMAPFILEHEADER bfHeader;
 BITMAPINFO biInfo;
}BITMAPFILE;
 
#pragma pack(pop)
 
int GenBmpFile(const unsigned char *pData, unsigned char bitCountPerPix, unsigned int width, unsigned int height, const char *filename);
 
// //YUV422转换为RGB32表
// const int rvarrxyp[]={
// -180,-179,-177,-176,-175,-173,-172,-170,-169,-167,-166,-165,-163,-162,-160,-159,-158,-156,-155,-153,-152,-151,
// -149,-148,-146,-145,-144,-142,-141,-139,-138,-137,-135,-134,-132,-131,-129,-128,-127,-125,-124,-122,-121,-120,
// -118,-117,-115,-114,-113,-111,-110,-108,-107,-106,-104,-103,-101,-100,-99,-97,-96,-94,-93,-91,-90,-89,-87,-86,
// -84,-83,-82,-80,-79,-77,-76,-75,-73,-72,-70,-69,-68,-66,-65,-63,-62,-61,-59,-58,-56,-55,-53,-52,-51,-49,-48,
// -46,-45,-44,-42,-41,-39,-38,-37,-35,-34,-32,-31,-30,-28,-27,-25,-24,-23,-21,-20,-18,-17,-15,-14,-13,-11,-10,
// -8,-7,-6,-4,-3,-1,0,1,3,4,6,7,8,10,11,13,14,15,17,18,20,21,23,24,25,27,28,30,31,32,34,35,37,38,39,41,42,44,45,
// 46,48,49,51,52,53,55,56,58,59,61,62,63,65,66,68,69,70,72,73,75,76,77,79,80,82,83,84,86,87,89,90,91,93,94,96,
// 97,99,100,101,103,104,106,107,108,110,111,113,114,115,117,118,120,121,122,124,125,127,128,129,131,132,134,135,
// 137,138,139,141,142,144,145,146,148,149,151,152,153,155,156,158,159,160,162,163,165,166,167,169,170,172,173,175,
// 176,177,179};
 
// const int guarrxyp[]={
// -44,-44,-44,-43,-43,-42,-42,-42,-41,-41,-41,-40,-40,-40,-39,-39,-39,-38,-38,-38,-37,-37,-37,-36,-36,-36,-35,-35,
// -35,-34,-34,-34,-33,-33,-32,-32,-32,-31,-31,-31,-30,-30,-30,-29,-29,-29,-28,-28,-28,-27,-27,-27,-26,-26,-26,-25,
// -25,-25,-24,-24,-23,-23,-23,-22,-22,-22,-21,-21,-21,-20,-20,-20,-19,-19,-19,-18,-18,-18,-17,-17,-17,-16,-16,-16,
// -15,-15,-15,-14,-14,-13,-13,-13,-12,-12,-12,-11,-11,-11,-10,-10,-10,-9,-9,-9,-8,-8,-8,-7,-7,-7,-6,-6,-6,-5,-5,-4,
// -4,-4,-3,-3,-3,-2,-2,-2,-1,-1,-1,0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,10,10,11,11,11,12,12,
// 12,13,13,13,14,14,15,15,15,16,16,16,17,17,17,18,18,18,19,19,19,20,20,20,21,21,21,22,22,22,23,23,23,24,24,25,25,25,
// 26,26,26,27,27,27,28,28,28,29,29,29,30,30,30,31,31,31,32,32,32,33,33,34,34,34,35,35,35,36,36,36,37,37,37,38,38,38,
// 39,39,39,40,40,40,41,41,41,42,42,42,43,43,44,44};
 
// const int gvarrxyp[]={
// -92,-91,-90,-90,-89,-88,-87,-87,-86,-85,-85,-84,-83,-82,-82,-81,-80,-80,-79,-78,-77,-77,-76,-75,-75,-74,-73,-72,
// -72,-71,-70,-70,-69,-68,-67,-67,-66,-65,-65,-64,-63,-62,-62,-61,-60,-60,-59,-58,-57,-57,-56,-55,-54,-54,-53,-52,-52,
// -51,-50,-49,-49,-48,-47,-47,-46,-45,-44,-44,-43,-42,-42,-41,-40,-39,-39,-38,-37,-37,-36,-35,-34,-34,-33,-32,-32,-31,
// -30,-29,-29,-28,-27,-27,-26,-25,-24,-24,-23,-22,-22,-21,-20,-19,-19,-18,-17,-16,-16,-15,-14,-14,-13,-12,-11,-11,-10,
// -9,-9,-8,-7,-6,-6,-5,-4,-4,-3,-2,-1,-1,0,1,1,2,3,4,4,5,6,6,7,8,9,9,10,11,11,12,13,14,14,15,16,16,17,18,19,19,20,21,
// 22,22,23,24,24,25,26,27,27,28,29,29,30,31,32,32,33,34,34,35,36,37,37,38,39,39,40,41,42,42,43,44,44,45,46,47,47,48,49,
// 49,50,51,52,52,53,54,54,55,56,57,57,58,59,60,60,61,62,62,63,64,65,65,66,67,67,68,69,70,70,71,72,72,73,74,75,75,76,77,
// 77,78,79,80,80,81,82,82,83,84,85,85,86,87,87,88,89,90,90,91};
 
// const int buarrxyp[]={
// -228,-226,-224,-222,-221,-219,-217,-215,-213,-212,-210,-208,-206,-205,-203,-201,-199,-197,-196,-194,-192,-190,-189,
// -187,-185,-183,-181,-180,-178,-176,-174,-173,-171,-169,-167,-165,-164,-162,-160,-158,-157,-155,-153,-151,-149,-148,
// -146,-144,-142,-141,-139,-137,-135,-133,-132,-130,-128,-126,-125,-123,-121,-119,-117,-116,-114,-112,-110,-109,-107,
// -105,-103,-101,-100,-98,-96,-94,-93,-91,-89,-87,-85,-84,-82,-80,-78,-76,-75,-73,-71,-69,-68,-66,-64,-62,-60,-59,-57,
// -55,-53,-52,-50,-48,-46,-44,-43,-41,-39,-37,-36,-34,-32,-30,-28,-27,-25,-23,-21,-20,-18,-16,-14,-12,-11,-9,-7,-5,-4,
// -2,0,2,4,5,7,9,11,12,14,16,18,20,21,23,25,27,28,30,32,34,36,37,39,41,43,44,46,48,50,52,53,55,57,59,60,62,64,66,68,69,
// 71,73,75,76,78,80,82,84,85,87,89,91,93,94,96,98,100,101,103,105,107,109,110,112,114,116,117,119,121,123,125,126,128,
// 130,132,133,135,137,139,141,142,144,146,148,149,151,153,155,157,158,160,162,164,165,167,169,171,173,174,176,178,180,
// 181,183,185,187,189,190,192,194,196,197,199,201,203,205,206,208,210,212,213,215,217,219,221,222,224,226};
 
 

// void yuyv2y2(const void *p, int size)
// {
//     int i, j;
//     unsigned char y1, y2;
//     char *pointer;
//     pointer = (char *)p;
//     unsigned char *frame_buffer;
//     unsigned char buffer4y2[frame_width * frame_height * 2];
//     frame_buffer = (unsigned char *)buffer4y2;

//     for (i = 0; i < frame_height; i++)
//         for (j = 0; j < frame_width / 2; j++)
//         {
//             y1 = *(pointer + (i * frame_width / 2 + j) * 4);
//             y2 = *(pointer + (i * frame_width / 2 + j) * 4 + 2);
//             *(frame_buffer + ((i)*frame_width / 2 + j) * 2) = (unsigned char)y1;
//             *(frame_buffer + ((i)*frame_width / 2 + j) * 2 + 1) = (unsigned char)y2;
//         }
//     printf("Converted yuyv to rgb.");
// }






// //table index: convert YUV422 to RGB32
// void yuyv2rgb(unsigned int *rgb_buf, unsigned char *v_buf) wait to modify
// {

//     // if(vinfo.bits_per_pixel == 32)
//     //yuv422 两个像素占4个字节（Y0，V，Y1，U），两个像素共用一个UV对，总字节数为W*H*2，
//     //而RGB32则正好两个像素是一个LONG型对齐，以LONG为单位一次可计算两个像素点，之后framebuf自增4个字节地址指向一两个像素点
//     for(y = 0; y < fmt.fmt.pix.height;  y++)
//     {
//         for(x = 0; x < fmt.fmt.pix.width/2; x++)
//         {
//             //YUV422转换为RGB32
//             //process(fbp32 + y*vinfo.xres + x,
//             //(unsigned char *)framebuf[buf.index].start + (y*fmt.fmt.pix.width+x)*2);
//             //由于是以long为基数自增，所以一行long数为：width(long)=width(int)/2

//             processinbellow((picdata + y*fmt.fmt.pix.width/2 + x),
//                     (unsigned char *)framebuf[buf.index].start + y*fmt.fmt.pix.width*2+x*4);

//             int r,g,b;
//             int u,v;
//             int y[2];
//             int rv,guv,bu,i;
//             unsigned int *fb_buf = (unsigned int *)rgb_buf;
//             y[0]=(int)*v_buf;
//             v=(int)*(v_buf+1);
//             y[1]=(int)*(v_buf+2);
//             u=(int)*(v_buf+3); 
//             rv=rvarrxyp[v];
//             guv=guarrxyp[u]+gvarrxyp[v];
//             bu=buarrxyp[u];
        
//             for(i=0;i<2;i++){
//                 r=y[i]+rv;
//                 g=y[i]-guv;
//                 b=y[i]+bu;
//                 if (r>255) r=255;
//                 if (r<0) r=0;
//                 if (g>255) g=255;
//                 if (g<0) g=0;
//                 if (b>255) b=255;
//                 if (b<0) b=0;
//                 *(fb_buf+i)=(b<<16)+(g<<8)+r;
//             }
//         }
//     }
// }



#endif /* BMP_H_ */

 