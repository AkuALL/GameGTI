#include <assert.h> 
#include <fstream> 
#include "../../include/imageloader.h" 
using namespace std; 
Image::Image(char* ps, int w, int h) : pixels(ps), width(w), 
height(h) {} 
Image::~Image() { 
delete[] pixels; 
} 
namespace { 
//Konversi 4 buah karakter ke integer,  
//menggunakan bentuk little-endian 
  int toInt(const char* bytes) { 
    return (int)(((unsigned int)(unsigned char)bytes[3] << 24) | 
   ((unsigned int)(unsigned char)bytes[2] << 16) | 
   ((unsigned int)(unsigned char)bytes[1] << 8) | 
   (unsigned char)bytes[0]); 
 } 
  //Konversi 2 buah karakter ke integer,  
  //menggunakan bentuk little-endian 
 short toShort(const char* bytes) { 
   return (short)(((unsigned char)bytes[1] << 8) | 
    (unsigned char)bytes[0]); 
 } 
  //Membaca 4 byte selanjutnya sebagai integer,  
  //menggunakan bentuk little-endian 
  int readInt(ifstream &input) { 
 char buffer[4]; 
 input.read(buffer, 4); 
 return toInt(buffer); 
  } 
  short readShort(ifstream &input) { 
 char buffer[2]; 
 input.read(buffer, 2); 
 return toShort(buffer); 
  } 
  struct ColorMask {
   unsigned int mask;
   int shift;
   int bits;
  };
  ColorMask makeMask(unsigned int mask) {
   ColorMask colorMask = {mask, 0, 0};
   if (mask == 0) return colorMask;
   while (((mask >> colorMask.shift) & 1) == 0) colorMask.shift++;
   unsigned int shifted = mask >> colorMask.shift;
   while ((shifted & 1) == 1) {
    colorMask.bits++;
    shifted >>= 1;
   }
   return colorMask;
  }
  unsigned char getMaskedChannel(unsigned int pixel, const ColorMask &colorMask) {
   if (colorMask.mask == 0 || colorMask.bits == 0) return 0;
   unsigned int value = (pixel & colorMask.mask) >> colorMask.shift;
   if (colorMask.bits >= 8) return (unsigned char)(value >> (colorMask.bits - 8));
   return (unsigned char)((value * 255) / ((1u << colorMask.bits) - 1));
  }
  template<class T> 
  class auto_array { 
  private: 
 T* array; 
 mutable bool isReleased; 
  public: 
 explicit auto_array(T* array_ = NULL) : 
 array(array_), isReleased(false) {} 
 auto_array(const auto_array<T> &aarray) { 
  array = aarray.array; 
  isReleased = aarray.isReleased; 
  aarray.isReleased = true; 
 } 
 ~auto_array() { 
 if (!isReleased && array != NULL) { 
  delete[] array; 
 } 
  } 
 T* get() const { 
  return array; 
 } 
 T &operator*() const { 
  return *array; 
 } 
 void operator=(const auto_array<T> &aarray) { 
  if (!isReleased && array != NULL) { 
   delete[] array; 
  } 
  array = aarray.array; 
  isReleased = aarray.isReleased; 
  aarray.isReleased = true; 
 } 
 T* operator->() const { 
  return array; 
 } 
 T* release() { 
  isReleased = true; 
  return array; 
 } 
 void reset(T* array_ = NULL) { 
  if (!isReleased && array != NULL) { 
   delete[] array; 
  } 
  array = array_; 
 } 
   T* operator+(int i) { 
    return array + i; 
   } 
    
   T &operator[](int i) { 
    return array[i]; 
   } 
 }; 
} 
 
Image* loadBMP(const char* filename) { 
 ifstream input; 
 input.open(filename, ifstream::binary); 
 assert(!input.fail() || !"File tidak ditemukan!!!"); 
 char buffer[2]; 
 input.read(buffer, 2); 
 assert(buffer[0] == 'B' && buffer[1] == 'M' || !"Bukan file bitmap!!!"); 
 input.ignore(8); 
 int dataOffset = readInt(input); 
  
 int headerSize = readInt(input); 
 int width; 
 int height;
 int bitsPerPixel = 24;
 int compression = 0;
 unsigned int redMask = 0x00FF0000;
 unsigned int greenMask = 0x0000FF00;
 unsigned int blueMask = 0x000000FF;
 switch(headerSize) { 
  case 40: 
   width = readInt(input); 
   height = readInt(input); 
   assert(readShort(input) == 1 || !"Bitmap planes tidak valid!");
   bitsPerPixel = readShort(input);
   compression = readInt(input);
   break; 
  case 12: 
   width = readShort(input); 
   height = readShort(input); 
   assert(readShort(input) == 1 || !"Bitmap planes tidak valid!");
   bitsPerPixel = readShort(input);
   compression = 0;
   break; 
  case 64: 
   assert(!"Tidak dapat mengambil OS/2 V2 bitmaps"); 
   break; 
  case 108: 
  case 124: 
   width = readInt(input); 
   height = readInt(input); 
   assert(readShort(input) == 1 || !"Bitmap planes tidak valid!");
   bitsPerPixel = readShort(input);
   compression = readInt(input);
   break; 
  default: 
   assert(!"Format bitmap ini tidak diketahui!"); 
 } 

 assert((bitsPerPixel == 24 || bitsPerPixel == 32) || !"Gambar harus 24 atau 32 bits per pixel!");
 if (bitsPerPixel == 24) {
  assert(compression == 0 || !"Bitmap 24-bit harus tanpa kompresi!");
 } else {
  assert((compression == 0 || compression == 3) || !"Bitmap 32-bit harus RGB atau bitfields!");
  if (compression == 3) {
   input.seekg(14 + 40, ios_base::beg);
   redMask = (unsigned int)readInt(input);
   greenMask = (unsigned int)readInt(input);
   blueMask = (unsigned int)readInt(input);
  }
 }
  
 //Membaca data 
 int bytesPerPixel = bitsPerPixel / 8;
 int absHeight = height < 0 ? -height : height;
 bool topDown = height < 0;
 int bytesPerRow = ((width * bytesPerPixel + 3) / 4) * 4; 
 int size = bytesPerRow * absHeight; 
 auto_array<char> pixels(new char[size]); 
 input.seekg(dataOffset, ios_base::beg); 
 input.read(pixels.get(), size); 
  
 //Mengambil data yang mempunyai format benar 
 auto_array<char> pixels2(new char[width * absHeight * 3]);
 ColorMask rMask = makeMask(redMask);
 ColorMask gMask = makeMask(greenMask);
 ColorMask bMask = makeMask(blueMask);
 for(int y = 0; y < absHeight; y++) {
  int destY = topDown ? absHeight - 1 - y : y;
  for(int x = 0; x < width; x++) { 
   int dest = 3 * (width * destY + x);
   int src = bytesPerRow * y + bytesPerPixel * x;
   if (bitsPerPixel == 24) {
    for(int c = 0; c < 3; c++) { 
     pixels2[dest + c] = pixels[src + (2 - c)]; 
    }
   } else {
    unsigned int pixel =
     ((unsigned char)pixels[src]) |
     ((unsigned int)(unsigned char)pixels[src + 1] << 8) |
     ((unsigned int)(unsigned char)pixels[src + 2] << 16) |
     ((unsigned int)(unsigned char)pixels[src + 3] << 24);
    pixels2[dest] = (char)getMaskedChannel(pixel, rMask);
    pixels2[dest + 1] = (char)getMaskedChannel(pixel, gMask);
    pixels2[dest + 2] = (char)getMaskedChannel(pixel, bMask);
   } 
  } 
 } 
  
 input.close(); 
 return new Image(pixels2.release(), width, absHeight); 
}
