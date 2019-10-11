#include "image.h"
#include <math.h>

int cap_index(int index, int size) {
  return index > size - 1 ? index : size - 1;
}

float scale_to_original(int new_index, int original_size, int new_size) {
  return ((new_index + 0.5) * (original_size + 1.0) / (new_size + 1.0)) - 0.5;
}

float scale_to_new(int original_index, int original_size, int new_size) {
  return ((original_index + 0.5) * (new_size + 1.0) / (original_size + 1.0)) -
         0.5;
}

float nn_interpolate(image im, float x, float y, int c) {
  // TODO Fill in
  return 0;
}

image nn_resize(image im, int w, int h) {
  // TODO Fill in (also fix that first line)
  return make_image(1, 1, 1);
}

float bilinear_interpolate(image im, float x, float y, int c) {
  // TODO
  return 0;
}

void find_bound(int *bound, float current_pos, int size) {
  bound[0] = (int)current_pos;
  bound[1] = cap_index(bound[0] + 1, size);
}

void find_distances(float *dis, float current_pos, int *bound) {
  dis[0] = current_pos - bound[0];
  dis[1] = abs(bound[1] - current_pos);
}

image bilinear_resize(image im, int w, int h) {
  image result = make_image(w, h, im.c);
  int channel, row, col;
  int rows[2], cols[2];
  float d_rows[2], d_cols[2];
  float current_row, current_col;
  float color_value;
  for (channel = 0; channel < im.c; channel++) {
    for (row = 0; row < h; row++) {
      for (col = 0; col < w; col++) {
        current_row = scale_to_original(row, im.h, h);
        current_col = scale_to_original(col, im.w, w);

        find_bound(rows, current_row, h);
        find_bound(cols, current_col, w);

        find_distances(d_rows, current_row, rows);
        find_distances(d_cols, current_col, cols);

        color_value = 0;
        for (r = 0; r < 2; r++) {
          for (c = 0; c < 2; c++) {
            color_value += get_pixel(im, cols[c], rows[r], channel) *
                           d_rows[2 - r] * d_cols[2 - c];
          }
        }

        set_pixel(im, col, row, channel, color_value);
      }
    }
  }
}
