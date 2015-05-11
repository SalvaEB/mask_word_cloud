/* 

   Mask word cloud generator

   Copyright Salva Espana Boquera sespana@dsic.upv.es

   mostly made for fun (it could be greatly improved)
   
   in debian/ubuntu install first libcairomm-1.0-dev

   LICENSE: LGPL v3
*/

#include "mask_word_cloud.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <string>
#include <unistd.h> // getopt

void usage(const char *programname) {
  std::cerr <<
    "Usage: " << programname << " \\\n"
    "\t[-h] \\ (show this help)\n"
    "\t[-r red_background] \\   (RGB of background, default 0 0 0 black\n"
    "\t[-g green_background] \\  color components in scale 0-255)\n"
    "\t[-b blue_background] \\\n"
    "\t[-m mask_file] \\ (determines also the size of image)\n"
    "\t[-R red_mask] \\   (words can be painted where mask\n"
    "\t[-G green_mask] \\  has this RGB color, default 0 0 0 black)\n"
    "\t[-B blue_mask] \\\n"
    "\t[-c color_file] \\ (must have the same size of mask, words colors are picked from here)\n"
    "\t[-f font_name] \\  (e.g. Sans)\n"
    "\t[-s font_step] \\  (default value 2, 1 is somewhat slower but more accurate)\n"
    "\t[-M min_font_size] \\ (default value 4)\n"
    "\t[-o output_prefix] \\ (default \"output\" generate \"output.svg\" \"output.png\" and \"output.pdf\") \n"
    "\t[-d words_margin] \\ (default value is 2)\n"
    "\t[-v vertical_preference] \\ (value between 0 and 100, default is 50)\n"
    "\twords.txt (an ordered list of pairs word initial_size)\n";
}

int main (int argc, char **argv) {
  const char *maskfilename       = "mask.png";
  const char *colorimagefilename = "";
  const char *fontname           = "Sans";
  const char *outputfilename  = "output";
  double r = 0;
  double g = 0;
  double b = 0;
  int    R = 0;
  int    G = 0;
  int    B = 0;
  int vertical_preference=50;
  int font_step=2;
  int mini_font_size=2;
  int words_margin=2;
  int opt;
  while ((opt = getopt(argc, argv, "hr:g:b:m:R:G:B:c:f:s:M:o:d:v:")) != -1) {
    switch (opt) 
      {
      case 'h':
	usage(argv[0]);
	exit(EXIT_SUCCESS);
      case 'r':
	r = atof(optarg)/255.0;
	break;
      case 'g':
	g = atof(optarg)/255.0;
	break;
      case 'b':
	b = atof(optarg)/255.0;
	break;
      case 'm':
	maskfilename = optarg;
	break;
      case 'R':
	R = atoi(optarg);
	break;
      case 'G':
	G = atoi(optarg);
	break;
      case 'B':
	B = atoi(optarg);
	break;
      case 'c':
	colorimagefilename = optarg;
	break;
      case 'f':
	fontname = optarg;
	break;
      case 's':
	font_step = atoi(optarg);
	break;
      case 'M':
	mini_font_size = atoi(optarg);
	break;
      case 'o':
	outputfilename = optarg;
	break;
      case 'd':
	words_margin = atoi(optarg);
	break;
      case 'v':
	vertical_preference = atoi(optarg);
	break;
      default: /* '?' */
	usage(argv[0]);
	exit(EXIT_FAILURE);
      }
  }
  
  if (optind >= argc) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  const char *termsfilename = argv[optind];

  std::string output_prefix(outputfilename);
  std::string png_outputfilename = output_prefix + ".png";
  std::string svg_outputfilename = output_prefix + ".svg";
  std::string pdf_outputfilename = output_prefix + ".pdf";
  
  MaskWordCloud mwc(maskfilename,
		    colorimagefilename,
		    svg_outputfilename.c_str(),
		    pdf_outputfilename.c_str(),
		    fontname,
		    r,g,b,
		    R,G,B,
		    vertical_preference,
		    words_margin,
		    font_step,mini_font_size);
  
  std::ifstream fs;
  fs.open(termsfilename);
  std::string word;
  double freq;
  std::vector <std::pair<std::string,double> > wordsList;
  while (fs >> word >> freq) {
    wordsList.push_back(std::make_pair(word,freq));
  }
  bool goon=true;
  while (goon) {
    for (int i=0; goon && i<wordsList.size(); ++i) {
      goon = mwc.paintWord(wordsList[i].first.c_str(),wordsList[i].second);
    }
  }
  mwc.writeImage(png_outputfilename.c_str());
  return 0;
}

