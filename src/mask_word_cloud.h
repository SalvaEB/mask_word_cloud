/* 

   Mask word cloud generator

   Copyright Salva Espana Boquera sespana@dsic.upv.es

   mostly made for fun (it could be greatly improved)
   
   in debian/ubuntu install first libcairomm-1.0-dev

   LICENSE: LGPL v3
*/

#ifndef MASK_WORD_CLOUD
#define MASK_WORD_CLOUD

#include <cairomm/cairomm.h>
#include <random>

class MaskWordCloud {
  int width, height;
  int vertical_preference;
  int words_margin;
  int font_step, mini_font_sz;
  bool useColorSurface;
  bool randColorMode;

  Cairo::RefPtr<Cairo::ImageSurface> overlapSurface;
  Cairo::RefPtr<Cairo::ImageSurface> wordCloudSurface;
  Cairo::RefPtr<Cairo::ImageSurface> colorSurface;
  Cairo::RefPtr<Cairo::Context> overlapCxt;
  Cairo::RefPtr<Cairo::Context> wordCloudCxt;
  Cairo::RefPtr<Cairo::Context> colorCxt;
  Cairo::RefPtr<Cairo::SvgSurface> svgSurface;
  Cairo::RefPtr<Cairo::Context> svgCxt;
  Cairo::RefPtr<Cairo::PdfSurface> pdfSurface;
  Cairo::RefPtr<Cairo::Context> pdfCxt;
  
  // each element is [BLUE GREEN RED ALPHA], at least when using
  // little-endian (x86/amd64 in my case)
  static const int BLUE =0;
  static const int GREEN=1;
  static const int RED  =2;
  static const int ALPHA=3;
  unsigned char (*pixelmat)[4];
  unsigned char (*colormat)[4];

  std::mt19937 rd;  
  bool choose_vertical() {
    std::uniform_int_distribution<int> d(0,100);
    return d(rd)<vertical_preference;
  }
  
  int *countmat, *scanner, *maxwidthrow;
  int xyTOindex(int x, int y) { return y*width+x; }
  unsigned char* pxMat(int x, int y)  { return pixelmat[xyTOindex(x,y)]; }
  unsigned char* clrMat(int x, int y) { return colormat[xyTOindex(x,y)]; }
  int&           cnMat(int x, int y)  { return countmat[xyTOindex(x,y)]; }
  
  bool findFreeRectangle(int bbWidth, int bbHeight, int &posx, int &posy);
  void dilateImage(int posx, int posy, int bbWidth, int bbHeight, int margin);
  void freezeImage(int posx, int posy, int bbWidth, int bbHeight, int margin);
  void getMeanColor(int posx, int posy,
		    int bbWidth, int bbHeight,
		    double &r, double &g, double &b);
  void pickRandomColor(double &r, double &g, double &b);
public:
  MaskWordCloud(const char *maskFilename,
		const char *colorFilename,
		const char *svgFilename,
		const char *pdfFilename,
		const char *fontName,
		// colors in scale [0,1]
		double r, double g, double b,
		int    R, int    G, int    B,
		int vertical_preference,
		int words_margin,
		int font_step, int mini_font_sz, bool colorMode);
  ~MaskWordCloud();
  bool paintWord(const char *text, double initialFontSz);
  void writeImage(const char *filename);
};

#endif // MASK_WORD_CLOUD
