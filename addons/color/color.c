/* Addon which allows converting between different color
 * representations. Included are:
 * - HSV (like A4)
 * - names (mostly X11 color names, except where CSS redefines them)
 * - HSL (the "better" HSV)
 * - CMYK (a bit like the opposite of RGB)
 * - YUV (the Y channel is quite useful for creating grayscale pictures)
 */
#include "allegro5/allegro.h"
#include "allegro5/allegro_color.h"
#include <math.h>
#include <stdio.h>

typedef struct {
   char const *name;
   int r, g, b;
} ColorName;

/* Taken from http://www.w3.org/TR/2010/PR-css3-color-20101028/#svg-color
 * This must be sorted correctly for binary search.
 */
static ColorName _al_color_names[] = {
   {"aliceblue", 0xf0, 0xf8, 0xff},
   {"antiquewhite", 0xfa, 0xeb, 0xd7},
   {"aqua", 0x00, 0xff, 0xff},
   {"aquamarine", 0x7f, 0xff, 0xd4},
   {"azure", 0xf0, 0xff, 0xff},
   {"beige", 0xf5, 0xf5, 0xdc},
   {"bisque", 0xff, 0xe4, 0xc4},
   {"black", 0x00, 0x00, 0x00},
   {"blanchedalmond", 0xff, 0xeb, 0xcd},
   {"blue", 0x00, 0x00, 0xff},
   {"blueviolet", 0x8a, 0x2b, 0xe2},
   {"brown", 0xa5, 0x2a, 0x2a},
   {"burlywood", 0xde, 0xb8, 0x87},
   {"cadetblue", 0x5f, 0x9e, 0xa0},
   {"chartreuse", 0x7f, 0xff, 0x00},
   {"chocolate", 0xd2, 0x69, 0x1e},
   {"coral", 0xff, 0x7f, 0x50},
   {"cornflowerblue", 0x64, 0x95, 0xed},
   {"cornsilk", 0xff, 0xf8, 0xdc},
   {"crimson", 0xdc, 0x14, 0x3c},
   {"cyan", 0x00, 0xff, 0xff},
   {"darkblue", 0x00, 0x00, 0x8b},
   {"darkcyan", 0x00, 0x8b, 0x8b},
   {"darkgoldenrod", 0xb8, 0x86, 0x0b},
   {"darkgray", 0xa9, 0xa9, 0xa9},
   {"darkgreen", 0x00, 0x64, 0x00},
   {"darkgrey", 0xa9, 0xa9, 0xa9},
   {"darkkhaki", 0xbd, 0xb7, 0x6b},
   {"darkmagenta", 0x8b, 0x00, 0x8b},
   {"darkolivegreen", 0x55, 0x6b, 0x2f},
   {"darkorange", 0xff, 0x8c, 0x00},
   {"darkorchid", 0x99, 0x32, 0xcc},
   {"darkred", 0x8b, 0x00, 0x00},
   {"darksalmon", 0xe9, 0x96, 0x7a},
   {"darkseagreen", 0x8f, 0xbc, 0x8f},
   {"darkslateblue", 0x48, 0x3d, 0x8b},
   {"darkslategray", 0x2f, 0x4f, 0x4f},
   {"darkslategrey", 0x2f, 0x4f, 0x4f},
   {"darkturquoise", 0x00, 0xce, 0xd1},
   {"darkviolet", 0x94, 0x00, 0xd3},
   {"deeppink", 0xff, 0x14, 0x93},
   {"deepskyblue", 0x00, 0xbf, 0xff},
   {"dimgray", 0x69, 0x69, 0x69},
   {"dimgrey", 0x69, 0x69, 0x69},
   {"dodgerblue", 0x1e, 0x90, 0xff},
   {"firebrick", 0xb2, 0x22, 0x22},
   {"floralwhite", 0xff, 0xfa, 0xf0},
   {"forestgreen", 0x22, 0x8b, 0x22},
   {"fuchsia", 0xff, 0x00, 0xff},
   {"gainsboro", 0xdc, 0xdc, 0xdc},
   {"ghostwhite", 0xf8, 0xf8, 0xff},
   {"gold", 0xff, 0xd7, 0x00},
   {"goldenrod", 0xda, 0xa5, 0x20},
   {"gray", 0x80, 0x80, 0x80},
   {"green", 0x00, 0x80, 0x00},
   {"greenyellow", 0xad, 0xff, 0x2f},
   {"grey", 0x80, 0x80, 0x80},
   {"honeydew", 0xf0, 0xff, 0xf0},
   {"hotpink", 0xff, 0x69, 0xb4},
   {"indianred", 0xcd, 0x5c, 0x5c},
   {"indigo", 0x4b, 0x00, 0x82},
   {"ivory", 0xff, 0xff, 0xf0},
   {"khaki", 0xf0, 0xe6, 0x8c},
   {"lavender", 0xe6, 0xe6, 0xfa},
   {"lavenderblush", 0xff, 0xf0, 0xf5},
   {"lawngreen", 0x7c, 0xfc, 0x00},
   {"lemonchiffon", 0xff, 0xfa, 0xcd},
   {"lightblue", 0xad, 0xd8, 0xe6},
   {"lightcoral", 0xf0, 0x80, 0x80},
   {"lightcyan", 0xe0, 0xff, 0xff},
   {"lightgoldenrodyellow", 0xfa, 0xfa, 0xd2},
   {"lightgray", 0xd3, 0xd3, 0xd3},
   {"lightgreen", 0x90, 0xee, 0x90},
   {"lightgrey", 0xd3, 0xd3, 0xd3},
   {"lightpink", 0xff, 0xb6, 0xc1},
   {"lightsalmon", 0xff, 0xa0, 0x7a},
   {"lightseagreen", 0x20, 0xb2, 0xaa},
   {"lightskyblue", 0x87, 0xce, 0xfa},
   {"lightslategray", 0x77, 0x88, 0x99},
   {"lightslategrey", 0x77, 0x88, 0x99},
   {"lightsteelblue", 0xb0, 0xc4, 0xde},
   {"lightyellow", 0xff, 0xff, 0xe0},
   {"lime", 0x00, 0xff, 0x00},
   {"limegreen", 0x32, 0xcd, 0x32},
   {"linen", 0xfa, 0xf0, 0xe6},
   {"magenta", 0xff, 0x00, 0xff},
   {"maroon", 0x80, 0x00, 0x00},
   {"mediumaquamarine", 0x66, 0xcd, 0xaa},
   {"mediumblue", 0x00, 0x00, 0xcd},
   {"mediumorchid", 0xba, 0x55, 0xd3},
   {"mediumpurple", 0x93, 0x70, 0xdb},
   {"mediumseagreen", 0x3c, 0xb3, 0x71},
   {"mediumslateblue", 0x7b, 0x68, 0xee},
   {"mediumspringgreen", 0x00, 0xfa, 0x9a},
   {"mediumturquoise", 0x48, 0xd1, 0xcc},
   {"mediumvioletred", 0xc7, 0x15, 0x85},
   {"midnightblue", 0x19, 0x19, 0x70},
   {"mintcream", 0xf5, 0xff, 0xfa},
   {"mistyrose", 0xff, 0xe4, 0xe1},
   {"moccasin", 0xff, 0xe4, 0xb5},
   {"navajowhite", 0xff, 0xde, 0xad},
   {"navy", 0x00, 0x00, 0x80},
   {"oldlace", 0xfd, 0xf5, 0xe6},
   {"olive", 0x80, 0x80, 0x00},
   {"olivedrab", 0x6b, 0x8e, 0x23},
   {"orange", 0xff, 0xa5, 0x00},
   {"orangered", 0xff, 0x45, 0x00},
   {"orchid", 0xda, 0x70, 0xd6},
   {"palegoldenrod", 0xee, 0xe8, 0xaa},
   {"palegreen", 0x98, 0xfb, 0x98},
   {"paleturquoise", 0xaf, 0xee, 0xee},
   {"palevioletred", 0xdb, 0x70, 0x93},
   {"papayawhip", 0xff, 0xef, 0xd5},
   {"peachpuff", 0xff, 0xda, 0xb9},
   {"peru", 0xcd, 0x85, 0x3f},
   {"pink", 0xff, 0xc0, 0xcb},
   {"plum", 0xdd, 0xa0, 0xdd},
   {"powderblue", 0xb0, 0xe0, 0xe6},
   {"purple", 0x80, 0x00, 0x80},
   {"purwablue", 0x9b, 0xe1, 0xff},
   {"red", 0xff, 0x00, 0x00},
   {"rosybrown", 0xbc, 0x8f, 0x8f},
   {"royalblue", 0x41, 0x69, 0xe1},
   {"saddlebrown", 0x8b, 0x45, 0x13},
   {"salmon", 0xfa, 0x80, 0x72},
   {"sandybrown", 0xf4, 0xa4, 0x60},
   {"seagreen", 0x2e, 0x8b, 0x57},
   {"seashell", 0xff, 0xf5, 0xee},
   {"sienna", 0xa0, 0x52, 0x2d},
   {"silver", 0xc0, 0xc0, 0xc0},
   {"skyblue", 0x87, 0xce, 0xeb},
   {"slateblue", 0x6a, 0x5a, 0xcd},
   {"slategray", 0x70, 0x80, 0x90},
   {"slategrey", 0x70, 0x80, 0x90},
   {"snow", 0xff, 0xfa, 0xfa},
   {"springgreen", 0x00, 0xff, 0x7f},
   {"steelblue", 0x46, 0x82, 0xb4},
   {"tan", 0xd2, 0xb4, 0x8c},
   {"teal", 0x00, 0x80, 0x80},
   {"thistle", 0xd8, 0xbf, 0xd8},
   {"tomato", 0xff, 0x63, 0x47},
   {"turquoise", 0x40, 0xe0, 0xd0},
   {"violet", 0xee, 0x82, 0xee},
   {"wheat", 0xf5, 0xde, 0xb3},
   {"white", 0xff, 0xff, 0xff},
   {"whitesmoke", 0xf5, 0xf5, 0xf5},
   {"yellow", 0xff, 0xff, 0x00},
   {"yellowgreen", 0x9a, 0xcd, 0x32},
};

#define NUM_COLORS (sizeof(_al_color_names) / sizeof(ColorName))

static void assert_sorted_names(void)
{
   /* In debug mode, check once that the array is sorted. */
#ifdef DEBUGMODE
   static bool done = false;
   unsigned i;

   if (!done) {
      for (i = 1; i < NUM_COLORS; i++) {
         ASSERT(strcmp(_al_color_names[i-1].name, _al_color_names[i].name) < 0);
      }
      done = true;
   }
#endif
}

static int compare(const void *va, const void *vb)
{
   char const *ca = va;
   ColorName const *cb = vb;
   return strcmp(ca, cb->name);
}

/* Function: al_color_name_to_rgb
 */
bool al_color_name_to_rgb(char const *name, float *r, float *g, float *b)
{
   void *result;
   assert_sorted_names();
   result = bsearch(name, _al_color_names, NUM_COLORS, sizeof(ColorName),
      compare);
   if (result) {
      ColorName *c = result;
      *r = c->r / 255.0;
      *g = c->g / 255.0;
      *b = c->b / 255.0;
      return true;
   }
   return false;
}

/* Function: al_color_rgb_to_name
 */
char const *al_color_rgb_to_name(float r, float g, float b)
{
   int i;
   int ir = r * 255;
   int ig = g * 255;
   int ib = b * 255;
   int n = NUM_COLORS;
   int min = n, mind = 0;
   /* Could optimize this, right now it does linear search. */
   for (i = 0; i < n; i++) {
      int dr = _al_color_names[i].r - ir;
      int dg = _al_color_names[i].g - ig;
      int db = _al_color_names[i].b - ib;
      int d = dr * dr + dg * dg + db * db;
      if (min == n || d < mind) {
         min = i;
         mind = d;
      }
   }
   return _al_color_names[min].name;
}

/* Function: al_color_name
 */
ALLEGRO_COLOR al_color_name(char const *name)
{
   float r, g, b;
   if (al_color_name_to_rgb(name, &r, &g, &b))
      return al_map_rgb_f(r, g, b);
   else
      return al_map_rgb_f(0, 0, 0);
}

/* Function: al_color_hsv_to_rgb
 */
void al_color_hsv_to_rgb(float hue, float saturation, float value,
   float *red, float *green, float *blue)
{
   int d;
   float e, a, b, c;
   hue = fmodf(hue, 360);
   if (hue < 0) hue += 360;
   d = hue / 60;
   e = hue / 60 - d;
   a = value * (1 - saturation);
   b = value * (1 - e * saturation);
   c = value * (1 - (1 - e) * saturation);
   switch (d) {
      default:
      case 0: *red = value, *green = c,     *blue = a;     return;
      case 1: *red = b,     *green = value, *blue = a;     return;
      case 2: *red = a,     *green = value, *blue = c;     return;
      case 3: *red = a,     *green = b,     *blue = value; return;
      case 4: *red = c,     *green = a,     *blue = value; return;
      case 5: *red = value, *green = a,     *blue = b;     return;
   }
}

/* Function: al_color_rgb_to_hsv
 */
void al_color_rgb_to_hsv(float red, float green, float blue,
   float *hue, float *saturation, float *value)
{
   float a, b, c, d;
   if (red > green)
      if (red > blue)
         if (green > blue)
            a = red, b = green - blue, c = blue, d = 0;
         else
            a = red, b = green - blue, c = green, d = 0;
      else
         a = blue, b = red - green, c = green, d = 4;
   else
      if (red > blue)
          a = green, b = blue - red, c = blue, d = 2;
      else
         if (green > blue)
            a = green, b = blue - red, c = red, d = 2;
         else
             a = blue, b = red - green, c = red, d = 4;

   if (a == c)
       *hue = 0;
   else
       *hue = 60 * (d + b / (a - c));
       if (*hue < 0)
         *hue += 360;
       if (*hue > 360)
         *hue -= 360;

   if (a == 0)
      *saturation = 0;
   else
      *saturation = (a - c) / a;
   *value = a;
}

/* Function: al_color_hsv
 */
ALLEGRO_COLOR al_color_hsv(float h, float s, float v)
{
   float r, g, b;
   al_color_hsv_to_rgb(h, s, v, &r, &g, &b);
   return al_map_rgb_f(r, g, b);
}

static float helper(float x, float a, float b)
{
   if (x < 0) x += 1;
   if (x > 1) x -= 1;

   if (x * 6 < 1) return b + (a - b) * 6 * x;
   if (x * 6 < 3) return a;
   if (x * 6 < 4) return b + (a - b) * (4.0 - 6.0 * x);
   return b;
}

/* Function: al_color_hsl_to_rgb
 */
void al_color_hsl_to_rgb(float hue, float saturation, float lightness,
   float *red, float *green, float *blue)
{
   float a, b, h;
   hue = fmodf(hue, 360);
   if (hue < 0) hue += 360;
   h = hue / 360;
   if (lightness < 0.5)
      a = lightness + lightness * saturation;
   else
      a = lightness + saturation - lightness * saturation;
   b = lightness * 2 - a;
   *red = helper(h + 1.0 / 3.0, a, b);
   *green = helper(h, a, b);
   *blue = helper(h - 1.0 / 3.0, a, b);
}

/* Function: al_color_rgb_to_hsl
 */
void al_color_rgb_to_hsl(float red, float green, float blue,
   float *hue, float *saturation, float *lightness)
{
   float a, b, c, d;
   if (red > green)
      if (red > blue)
         if (green > blue)
            a = red, b = green - blue, c = blue, d = 0;
         else
            a = red, b = green - blue, c = green, d = 0;
      else
         a = blue, b = red - green, c = green, d = 4;
   else
      if (red > blue)
          a = green, b = blue - red, c = blue, d = 2;
      else
         if (green > blue)
            a = green, b = blue - red, c = red, d = 2;
         else
             a = blue, b = red - green, c = red, d = 4;

   if (a == c)
       *hue = 0;
   else
       *hue = 60 * (d + b / (a - c));
       if (*hue < 0)
         *hue += 360;

   if (a == c)
      *saturation = 0;
   else if (a + c < 1)
      *saturation = (a - c) / (a + c);
   else
      *saturation = (a - c) / (2 - a - c);
   
   *lightness = (a + c) / 2;
}

/* Function: al_color_hsl
 */
ALLEGRO_COLOR al_color_hsl(float h, float s, float l)
{
   float r, g, b;
   al_color_hsl_to_rgb(h, s, l, &r, &g, &b);
   return al_map_rgb_f(r, g, b);
}

/* Function: al_color_cmyk_to_rgb
 */
void al_color_cmyk_to_rgb(float cyan, float magenta, float yellow,
    float key, float *red, float *green, float *blue)
{
   float max = 1 - key;
   *red = max - cyan * max;
   *green = max - magenta * max;
   *blue = max - yellow * max;
}

/* Function: al_color_rgb_to_cmyk
 */
void al_color_rgb_to_cmyk(float red, float green, float blue,
   float *cyan, float *magenta, float *yellow, float *key)
{
   float max = red;
   if (green > max) max = green;
   if (blue > max) max = blue;
   *key = 1 - max;
   if (max > 0) {
      *cyan = (max - red) / max;
      *magenta = (max - green) / max;
      *yellow = (max - blue) / max;
   }
   else {
      *cyan = *magenta = *yellow = 0;
   }
}

/* Function: al_color_cmyk
 */
ALLEGRO_COLOR al_color_cmyk(float c, float m, float y, float k)
{
   float r, g, b;
   al_color_cmyk_to_rgb(c, m, y, k, &r, &g, &b);
   return al_map_rgb_f(r, g, b);
}

/* Function: al_color_yuv_to_rgb
 */
void al_color_yuv_to_rgb(float y, float u, float v,
    float *red, float *green, float *blue)
{
   /* Translate range 0..1 to actual range. */
   u = 0.436 * (u * 2 - 1);
   v = 0.615 * (v * 2 - 1);
   *red = y + v * 1.13983;
   *green = y + u * -0.39465 + v * -0.58060;
   *blue = y + u * 2.03211;
}

/* Function: al_color_rgb_to_yuv
 */
void al_color_rgb_to_yuv(float red, float green, float blue,
   float *y, float *u, float *v)
{
   *y = red * 0.299 + green * 0.587 + blue * 0.114;
   *u = red * -0.14713 + green * -0.28886 + blue * 0.436;
   *v = red * 0.615 + green * -0.51499 + blue * -0.10001;
   /* Normalize chroma components into 0..1 range. */
   *u = (*u / 0.436 + 1) * 0.5;
   *v = (*v / 0.615 + 1) * 0.5;
}

/* Function: al_color_yuv
 */
ALLEGRO_COLOR al_color_yuv(float y, float u, float v)
{
   float r, g, b;
   al_color_yuv_to_rgb(y, u, v, &r, &g, &b);
   return al_map_rgb_f(r, g, b);
}

/* Function: al_color_rgb_to_html
 */
void al_color_rgb_to_html(float red, float green, float blue,
    char *string)
{
   sprintf(string, "#%02x%02x%02x", (int)(red * 255),
      (int)(green * 255), (int)blue * 255);
}

/* Function: al_color_html_to_rgb
 */
void al_color_html_to_rgb(char const *string,
   float *red, float *green, float *blue)
{
   char const *ptr = string;
   long rgb;
   if (*ptr == '#') ptr++;
   rgb = strtol(ptr, NULL, 16);
   *red = (rgb >> 16) / 255.0;
   *green = ((rgb >> 8) & 255) / 255.0;
   *blue = (rgb & 255) / 255.0;
}

/* Function: al_color_html
 */
ALLEGRO_COLOR al_color_html(char const *string)
{
   float r, g, b;
   al_color_html_to_rgb(string, &r, &g, &b);
   return al_map_rgb_f(r, g, b);
}


/* Function: al_get_allegro_color_version
 */
uint32_t al_get_allegro_color_version(void)
{
   return ALLEGRO_VERSION_INT;
}
