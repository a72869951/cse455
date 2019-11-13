#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "image.h"

float get_pixel(image im, int x, int y, int c) {
  if (x < 0)
    x = 0;
  else if (x >= im.w)
    x = im.w - 1;

  if (y < 0)
    y = 0;
  else if (y >= im.h)
    y = im.h - 1;

  return im.data[x + (im.w * y) + (im.w * im.h * c)];
}

void set_pixel(image im, int x, int y, int c, float v) {
  if (0 <= x && x < im.w && 0 <= y && y < im.h) {
    im.data[x + (im.w * y) + (im.w * im.h * c)] = v;
  }
}

image copy_image(image im) {
  image copy = make_image(im.w, im.h, im.c);
  int channel, row, col;
  float pixel_value;
  for (channel = 0; channel < im.c; channel++) {
    for (row = 0; row < im.h; row++) {
      for (col = 0; col < im.w; col++) {
        pixel_value = get_pixel(im, col, row, channel);
        set_pixel(copy, col, row, channel, pixel_value);
      }
    }
  }
  return copy;
}

float rgb_pixel_to_grayscale(image im, int x, int y) {
  return get_pixel(im, x, y, 0) * 0.299 + get_pixel(im, x, y, 1) * 0.587 +
         get_pixel(im, x, y, 2) * 0.114;
}

image rgb_to_grayscale(image im) {
  assert(im.c == 3);
  image gray = make_image(im.w, im.h, 1);
  int row, col;
  float pixel_value;
  for (row = 0; row < im.h; row++) {
    for (col = 0; col < im.w; col++) {
      pixel_value = rgb_pixel_to_grayscale(im, col, row);
      set_pixel(gray, col, row, 0, pixel_value);
    }
  }
  return gray;
}

void shift_image(image im, int c, float v) {
  assert(im.c > c);
  int row, col;
  float pixel_value;
  for (row = 0; row < im.h; row++) {
    for (col = 0; col < im.w; col++) {
      pixel_value = v + get_pixel(im, col, row, c);
      set_pixel(im, col, row, c, pixel_value);
    }
  }
}

void clamp_image(image im) {
  int channel, row, col;
  float pixel_value;
  for (channel = 0; channel < im.c; channel++) {
    for (row = 0; row < im.h; row++) {
      for (col = 0; col < im.w; col++) {
        pixel_value = get_pixel(im, col, row, channel);
        if (pixel_value < 0.0)
          set_pixel(im, col, row, channel, 0);
        else if (pixel_value > 1.0)
          set_pixel(im, col, row, channel, 1);
      }
    }
  }
}

// These might be handy
float three_way_max(float a, float b, float c) {
  return (a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c);
}

float three_way_min(float a, float b, float c) {
  return (a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c);
}

void rgb_to_hsv(image im) {
  assert(im.c == 3);
  int row, col;
  float r, g, b;
  float h_prime, c;
  float v, s, h;
  for (row = 0; row < im.h; row++) {
    for (col = 0; col < im.w; col++) {
      r = get_pixel(im, col, row, 0);
      g = get_pixel(im, col, row, 1);
      b = get_pixel(im, col, row, 2);
      v = three_way_max(r, g, b);

      c = v - three_way_min(r, g, b);
      s = v == 0.0 ? 0.0 : c / v;

      if (c == 0)
        h_prime = 0;
      else if (v == r)
        h_prime = (g - b) / c;
      else if (v == g)
        h_prime = ((b - r) / c) + 2.0;
      else if (v == b)
        h_prime = ((r - g) / c) + 4.0;

      h = (h_prime < 0.0 ? h_prime + 6.0 : h_prime) / 6.0;
      set_pixel(im, col, row, 0, h);
      set_pixel(im, col, row, 1, s);
      set_pixel(im, col, row, 2, v);
    }
  }
}

void hsv_to_rgb(image im) {
  assert(im.c == 3);
  int row, col;
  float v, s, h;
  float c, x, m;
  float r, g, b;
  for (row = 0; row < im.h; row++) {
    for (col = 0; col < im.w; col++) {
      h = get_pixel(im, col, row, 0);
      s = get_pixel(im, col, row, 1);
      v = get_pixel(im, col, row, 2);
      c = v * s;

      x = h * 6.0;
      x = (x >= 4.0 ? x - 4.0 : x >= 2.0 ? x - 2.0 : x) - 1.0;
      x = c * (1 - (x >= 0.0 ? x : -x));

      m = v - c;

      if (h * 6.0 < 1.0) {
        r = c;
        g = x;
        b = 0.0;
      } else if (h * 6.0 < 2.0) {
        r = x;
        g = c;
        b = 0.0;
      } else if (h < 0.5) {
        r = 0.0;
        g = c;
        b = x;
      } else if (h * 6.0 < 4.0) {
        r = 0.0;
        g = x;
        b = c;
      } else if (h * 6.0 < 5.0) {
        r = x;
        g = 0.0;
        b = c;
      } else {
        r = c;
        g = 0.0;
        b = x;
      }

      set_pixel(im, col, row, 0, r + m);
      set_pixel(im, col, row, 1, g + m);
      set_pixel(im, col, row, 2, b + m);
    }
  }
}

void scale_image(image im, int c, float v) {
  assert(im.c > c);
  int row, col;
  float pixel_value;
  for (row = 0; row < im.h; row++) {
    for (col = 0; col < im.w; col++) {
      pixel_value = v * get_pixel(im, col, row, c);
      set_pixel(im, col, row, c, pixel_value);
    }
  }
}