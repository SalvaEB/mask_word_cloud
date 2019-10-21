/* 

   Mask word cloud generator

   Copyright Salva Espana Boquera sespana@dsic.upv.es

   mostly made for fun (it could be greatly improved)
   
   in debian/ubuntu install first libcairomm-1.0-dev

   LICENSE: LGPL v3
*/

#include "mask_word_cloud.h"
#include <queue>
#include <utility>
#include <cstring>
#include <cassert>

MaskWordCloud::MaskWordCloud(const char *maskFilename,
			     const char *colorFilename,
			     const char *svgFilename,
			     const char *pdfFilename,
			     const char *fontName,
			     double r, double g, double b,
			     int    R, int    G, int    B,
			     int vertical_preference,
			     int words_margin,
			     int font_step, int mini_font_sz, bool colorMode) :
  rd(std::random_device()()),
  vertical_preference(vertical_preference),
  words_margin(words_margin),
  font_step(font_step), mini_font_sz(mini_font_sz) 
{
  overlapSurface = Cairo::ImageSurface::create_from_png(maskFilename);
  overlapCxt = Cairo::Context::create(overlapSurface);
  overlapCxt->set_source_rgb(0, 0, 1); // paint in BLUE
  width = overlapSurface->get_width();
  height = overlapSurface->get_height();
  pixelmat = (unsigned char (*)[4])overlapSurface->get_data();
  countmat = new int[width*height](); // initialized to zero, probably unnecessary
  scanner = new int[width](); // initialized to zero, probably unnecessary
  maxwidthrow = new int[height](); // initialized to zero
  
  useColorSurface = (colorFilename!=0 && strlen(colorFilename)>0);
  randColorMode = colorMode;
  if (useColorSurface) {
    colorSurface = Cairo::ImageSurface::create_from_png(colorFilename);
    assert(width  == colorSurface->get_width());
    assert(height == colorSurface->get_height());
    colorCxt = Cairo::Context::create(colorSurface);
    colormat = (unsigned char (*)[4])colorSurface->get_data();
  } else {
    colormat = 0;
  }

  wordCloudSurface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width, height);
  wordCloudCxt     = Cairo::Context::create(wordCloudSurface);
  svgSurface       = Cairo::SvgSurface::create(svgFilename, width, height);
  svgCxt           = Cairo::Context::create(svgSurface);
  pdfSurface       = Cairo::PdfSurface::create(pdfFilename, width, height);
  pdfCxt           = Cairo::Context::create(pdfSurface);

  // fill background
  wordCloudCxt->set_source_rgb(r,g,b);
  wordCloudCxt->paint();
  wordCloudCxt->set_source_rgb(0, 0, 0); // not needed

  svgCxt->set_source_rgb(r,g,b);
  svgCxt->paint();
  svgCxt->set_source_rgb(0, 0, 0); // not needed

  pdfCxt->set_source_rgb(r,g,b);
  pdfCxt->paint();
  pdfCxt->set_source_rgb(0, 0, 0); // not needed

  overlapCxt->select_font_face(fontName,Cairo::FONT_SLANT_NORMAL,Cairo::FONT_WEIGHT_NORMAL);
  wordCloudCxt->select_font_face(fontName,Cairo::FONT_SLANT_NORMAL,Cairo::FONT_WEIGHT_NORMAL);
  svgCxt->select_font_face(fontName,Cairo::FONT_SLANT_NORMAL,Cairo::FONT_WEIGHT_NORMAL);
  pdfCxt->select_font_face(fontName,Cairo::FONT_SLANT_NORMAL,Cairo::FONT_WEIGHT_NORMAL);

  // traverse pixelmat to process the mask and remove from the image
  for (int y=0; y<height; ++y) {
    int count=0;
    for (int x=0; x<width; ++x) {
      if (pxMat(x,y)[RED] == R && pxMat(x,y)[GREEN] == G && pxMat(x,y)[BLUE] == B) { // not masked
	count++;
	if (count > maxwidthrow[y])
	  maxwidthrow[y]=count;
      } else { // masked
	count=0;
      }
      cnMat(x,y)=count;
      // in any case paint in black
      pxMat(x,y)[BLUE] = pxMat(x,y)[GREEN] = pxMat(x,y)[RED] = 0;
      pxMat(x,y)[ALPHA] = 255;
    }
  }
}

MaskWordCloud::~MaskWordCloud() {
  delete[] countmat;
  delete[] scanner;
  delete[] maxwidthrow;
}

// returns lower left corner
bool MaskWordCloud::findFreeRectangle(int bbWidth, int bbHeight,
				      int &posx, int &posy) {
  int lasty=0,firsty=0;
  int the_x_pos=-1, the_y_pos=-1, solcount=0;
  while (lasty<height) {
    while (lasty<height && (maxwidthrow[lasty]>=bbWidth)) {
      lasty++;
    }
    if (lasty - firsty >= bbHeight) {
      for (int x=0; x<width; ++x)
	scanner[x] = 0;
      for (int y=firsty; y<lasty; ++y)
	for (int x=bbWidth; x<width; ++x)
	  if (cnMat(x,y)<bbWidth) {
	    scanner[x]=0;
	  } else {
	    scanner[x]++;
	    if (scanner[x]>=bbHeight) {
	      // annotate the index position using
	      // http://en.wikipedia.org/wiki/Reservoir_sampling
	      if (solcount==0) {
		the_x_pos = x;
		the_y_pos = y;
	      } else {
		std::uniform_int_distribution<int> d(0,solcount);
		if (d(rd)==0) {
		  the_x_pos = x;
		  the_y_pos = y;
		}
	      }
	      solcount++;
	    }
	  }
    }
    lasty++;
    firsty = lasty;
  }
  if (solcount > 0) {
    posx = the_x_pos-bbWidth;
    posy = the_y_pos;
    return true;
  }
  return false;
}

// posx and posy point to the left upper corner
void MaskWordCloud::dilateImage(int posx, int posy,
				int bbWidth, int bbHeight,
				int margin) {
  // using BFS, not cache friendly and could be improved
  if (margin>0) {
    std::queue< std::pair<int,int> >  thequeue;
    int upY = std::min(posy+bbHeight,height);
    int upX = std::min(posx+bbWidth,width);
    for (int y=posy; y<upY; ++y)
      for (int x=posx; x<upX; ++x) {
	if (pxMat(x,y)[BLUE]>0) {
	  pxMat(x,y)[BLUE]=margin+1;
	  thequeue.push(std::make_pair(x,y));
	}
      }
    while (!thequeue.empty()) {
      int x = thequeue.front().first;
      int y = thequeue.front().second;
      int v = pxMat(x,y)[BLUE]-1;
      thequeue.pop();
      if (x>0 && pxMat(x-1,y)[BLUE]==0) {
	pxMat(x-1,y)[BLUE]=v;
	if (v>0) thequeue.push(std::make_pair(x-1,y));
      }
      if (x<width-1 && pxMat(x+1,y)[BLUE]==0) {
	pxMat(x+1,y)[BLUE]=v;
	if (v>0) thequeue.push(std::make_pair(x+1,y));
      }
      if (y>0 && pxMat(x,y-1)[BLUE]==0) {
	pxMat(x,y-1)[BLUE]=v;
	if (v>0) thequeue.push(std::make_pair(x,y-1));
      }
      if (y<height-1 && pxMat(x,y+1)[BLUE]==0) {
	pxMat(x,y+1)[BLUE]=v; 
	if (v>0) thequeue.push(std::make_pair(x,y+1));
      }
    }
  }
}

void MaskWordCloud::freezeImage(int posx, int posy, int bbWidth, int bbHeight, int margin) {
  int lowerx=std::max(0,     posx-margin);
  int upperx=std::min(width, posx+bbWidth+margin);
  int lowery=std::max(0,     posy-bbHeight-margin);
  int uppery=std::min(height,posy+margin);

  for (int y=lowery; y<uppery; ++y) {
    int maxremoved=0;
    for (int x=upperx-1; x>=lowerx; --x) {
      if (pxMat(x,y)[BLUE] > 0) {
	// put pixel
	int other = cnMat(x,y);
	int count=0;
	for (int x2=x; x2<width && cnMat(x2,y)>0; ++x2) {
	  other = cnMat(x2,y);
	  cnMat(x2,y) = count++;
	}
	if (other>maxremoved)
	  maxremoved = other;
	pxMat(x,y)[BLUE] = pxMat(x,y)[GREEN] = pxMat(x,y)[RED] = 0;
	pxMat(x,y)[ALPHA] = 255;
      }
    }
    if (maxremoved >= maxwidthrow[y]) {
      maxwidthrow[y]= cnMat(0,y);
      for (int x=1; x<width; ++x)
	if (cnMat(x,y)>maxwidthrow[y])
	  maxwidthrow[y]= cnMat(x,y);
    }
  }
}

// expects upper left corner
void MaskWordCloud::getMeanColor(int posx, int posy, int bbWidth, int bbHeight,
				   double &r, double &g, double &b) {
  double mR=0,mG=0,mB=0;
  for (int y=posy; y<posy+bbHeight; ++y)
    for (int x=posx; x<posx+bbWidth; ++x) {
      mR += clrMat(x,y)[RED];
      mG += clrMat(x,y)[GREEN];
      mB += clrMat(x,y)[BLUE];
    }
  double count = 255.0*bbWidth*bbHeight;
  r = mR/count;
  g = mG/count;
  b = mB/count;
}

// this method could be greatly improved
void MaskWordCloud::pickRandomColor(double &r, double &g, double &b) {
  std::uniform_int_distribution<int> d(0,155);
  r = (d(rd)+100.0)/255;
  g = (d(rd)+100.0)/255;
  b = (d(rd)+100.0)/255;
}

bool MaskWordCloud::paintWord(const char *text, double initialFontSz) {
  overlapCxt->save();
  wordCloudCxt->save();
  svgCxt->save();
  pdfCxt->save();
  int fontSize=initialFontSz;
  bool vertical = choose_vertical();
  while (fontSize>=mini_font_sz) {
    overlapCxt->set_font_size(fontSize);
    Cairo::TextExtents extents;
    overlapCxt->get_text_extents(text,extents);
    double bbWidth = extents.width;
    double bbHeight = extents.height;
    double x_bearing = extents.x_bearing;
    double y_bearing = extents.y_bearing;
    
    int textposx,textposy, bbposx, bbposy;
    if (vertical) {
      std::swap(bbHeight,bbWidth);
      std::swap(x_bearing,y_bearing);
    }
    // returns lower left corner
    if (findFreeRectangle(bbWidth,bbHeight,bbposx,bbposy)) {
      // adjust positions to be used by show_text method
      textposx = bbposx - x_bearing;
      textposy = bbposy;
      if (vertical) {
	textposy += y_bearing;
      } else {
	textposy -= bbHeight+y_bearing;
      }

      double r,g,b;
      if (useColorSurface) {
	// expects upper left corner
	getMeanColor(bbposx,bbposy-bbHeight,bbWidth,bbHeight,r,g,b);
      } else if(randColorMode) {
	pickRandomColor(r,g,b);
      } else {
        r = g = b = 0.0;
      }
      
      overlapCxt->move_to(textposx,textposy);
      wordCloudCxt->move_to(textposx,textposy);
      svgCxt->move_to(textposx,textposy);
      pdfCxt->move_to(textposx,textposy);
      
      if (vertical) {
	overlapCxt->rotate_degrees(-90);
	wordCloudCxt->rotate_degrees(-90);
	svgCxt->rotate_degrees(-90);
	pdfCxt->rotate_degrees(-90);
      }
      
      wordCloudCxt->set_source_rgb(r,g,b);
      svgCxt->set_source_rgb(r,g,b);
      pdfCxt->set_source_rgb(r,g,b);

      wordCloudCxt->set_font_size(fontSize);
      svgCxt->set_font_size(fontSize);
      pdfCxt->set_font_size(fontSize);

      overlapCxt->show_text(text);
      wordCloudCxt->show_text(text);
      svgCxt->show_text(text);
      pdfCxt->show_text(text);

      // expects upper left corner
      dilateImage(bbposx, bbposy-(bbHeight-1), bbWidth, bbHeight, words_margin);
      
      // freeze updates the "RLSA like" matrix and erases the content
      freezeImage(bbposx, bbposy, bbWidth, bbHeight, words_margin);
      
      overlapCxt->restore();
      wordCloudCxt->restore();
      svgCxt->restore();
      pdfCxt->restore();
      return true;
    } else {
      fontSize -= font_step;
    }
  }
  overlapCxt->restore();
  wordCloudCxt->restore();
  svgCxt->restore();
  pdfCxt->restore();
  return false;
}

void MaskWordCloud::writeImage(const char *filename) {
  svgCxt->show_page();
  pdfCxt->show_page();
  wordCloudSurface->write_to_png(filename);
}

