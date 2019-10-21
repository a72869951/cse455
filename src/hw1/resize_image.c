#include <math.h>
#include "image.h"

int cap_index(int index, int size) {
  return index < size - 1 ? index >= 0 ? index : 0 : size - 1;
}

float to_original_scale(int new_index, int original_size, int new_size) {
  return ((new_index + 0.5) * (original_size) / (new_size)) - 0.5;
}

float nn_interpolate(image im, float x, float y, int c) {
  int row, col;
  col = cap_index((int)round(x), im.w);
  row = cap_index((int)round(y), im.h);
  return get_pixel(im, col, row, c);
}

image nn_resize(image im, int w, int h) {
  image result = make_image(w, h, im.c);
  int channel, row, col;
  float color_value, row_scaled, col_scaled;
  for (channel = 0; channel < im.c; channel++) {
    for (row = 0; row < h; row++) {
      for (col = 0; col < w; col++) {
        col_scaled = to_original_scale(col, im.w, w);
        row_scaled = to_original_scale(row, im.h, h);
        color_value = nn_interpolate(im, col_scaled, row_scaled, channel);
        set_pixel(result, col, row, channel, color_value);
      }
    }
  }
  return result;
}

void find_bound(int *bound, float current_pos, int size) {
  bound[0] = cap_index((int)current_pos, size);
  bound[1] = cap_index((int)(current_pos + 1), size);
}

void find_distances(float *dis, float current_pos, int *bound) {
  if (bound[0] == bound[1]) {
    dis[0] = 0.5;
    dis[1] = 0.5;
  } else {
    dis[0] = current_pos - bound[0];
    dis[1] = bound[1] - current_pos;
  }
}

float bilinear_interpolate(image im, float x, float y, int c) {
  int rows[2], cols[2], row, col;
  float d_rows[2], d_cols[2], color_value = 0;

  find_bound(rows, y, im.h);
  find_bound(cols, x, im.w);

  find_distances(d_rows, y, rows);
  find_distances(d_cols, x, cols);

  for (row = 0; row < 2; row++) {
    for (col = 0; col < 2; col++) {
      color_value += get_pixel(im, cols[col], rows[row], c) * d_rows[1 - row] *
                     d_cols[1 - col];
    }
  }
  return color_value;
}

image bilinear_resize(image im, int w, int h) {
  image result = make_image(w, h, im.c);
  int channel, row, col;
  float color_value, row_scaled, col_scaled;
  for (channel = 0; channel < im.c; channel++) {
    for (row = 0; row < h; row++) {
      for (col = 0; col < w; col++) {
        col_scaled = to_original_scale(col, im.w, w);
        row_scaled = to_original_scale(row, im.h, h);
        color_value = bilinear_interpolate(im, col_scaled, row_scaled, channel);
        set_pixel(result, col, row, channel, color_value);
      }
    }
  }
  return result;
}
