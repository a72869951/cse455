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

image bilinear_resize(image im, int w, int h) {
  image result = make_image(w, h, im.c);
  int channel, row, col;
  int rows[2], cols[2];
  float d_rows[2], d_cols[2];
  // int lower_r, lower_c, upper_r, upper_c;
  float current_row, current_col;
  // float d_lower_r, d_upper_r, d_lower_c, d_upper_c;
  float color_value;
  for (channel = 0; channel < im.c; channel++) {
    for (row = 0; row < h; row++) {
      for (col = 0; col < w; col++) {
        current_row = scale_to_original(row, im.h, h);
        current_col = scale_to_original(col, im.w, w);

        // lower_r = (int)current_row;
        // lower_c = (int)current_col;

        // upper_r = cap_index(lower_r + 1, h);
        // upper_c = cap_index(lower_c + 1, w);

        // d_lower_r = current_row - lower_r;
        // d_lower_c = current_col - lower_c;
        // d_upper_r = abs(upper_r - current_row);
        // d_upper_c = abs(upper_c - current_col);

        // color_value =
        //     get_pixel(im, lower_c, lower_r, channel) * d_upper_r * d_upper_c
        //     + get_pixel(im, lower_c, upper_r, channel) * d_lower_r *
        //     d_upper_c + get_pixel(im, upper_c, lower_r, channel) * d_upper_r
        //     * d_lower_c + get_pixel(im, upper_c, upper_r, channel) *
        //     d_lower_r * d_lower_c;

        rows[0] = (int)current_row;
        cols[0] = (int)current_col;

        rows[1] = cap_index(rows[0] + 1, h);
        cols[1] = cap_index(cols[0] + 1, w);

        d_rows[0] = current_row - rows[0];
        d_cols[0] = current_col - cols[0];
        d_rows[1] = abs(rows[1] - current_row);
        d_cols[1] = abs(cols[1] - current_col);

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
