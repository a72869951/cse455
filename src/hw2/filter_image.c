#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"
#define TWOPI 6.2831853

void l1_normalize(image im) {
  int size = im.c * im.h * im.h;
  float sum = 0;
  for (int i = 0; i < size; i++) {
    sum += im.data[i];
  }
  for (int i = 0; i < size; i++) {
    im.data[i] /= sum;
  }
}

image make_box_filter(int w) {
  image filter = make_image(w, w, 1);
  float value = 1.0 / (w * w);
  int size = w * w;
  for (int i = 0; i < size; i++) {
    filter.data[i] = value;
  }
  return filter;
}
image add_padding(image im, int padding) {
  image im_tmp = make_image(im.w + (2 * padding), im.h + (2 * padding), im.c);
  for (int channel = 0; channel < im.c; channel++) {
    for (int row = 0; row < im.h; row++) {
      for (int col = 0; col < im.w; col++) {
        float value = get_pixel(im, col, row, channel);
        set_pixel(im_tmp, col + padding, row + padding, channel, value);
      }
    }
  }

  int end_w = im_tmp.w - padding - 1;
  for (int channel = 0; channel < im.c; channel++) {
    for (int row = 0; row < im_tmp.h; row++) {
      for (int col = 0; col < padding; col++) {
        float value = get_pixel(im_tmp, padding, row, channel);
        set_pixel(im_tmp, col, row, channel, value);
        value = get_pixel(im_tmp, end_w, row, channel);
        set_pixel(im_tmp, end_w + 1 + col, row, channel, value);
      }
    }
  }

  int end_h = im_tmp.h - padding - 1;
  for (int channel = 0; channel < im.c; channel++) {
    for (int col = 0; col < im_tmp.w; col++) {
      for (int row = 0; row < padding; row++) {
        float value = get_pixel(im_tmp, col, padding, channel);
        set_pixel(im_tmp, col, row, channel, value);
        value = get_pixel(im_tmp, col, end_h, channel);
        set_pixel(im_tmp, col, end_h + 1 + row, channel, value);
      }
    }
  }

  return im_tmp;
}

float convolve_pixel(image im, image filter, int col, int row, int channel, int channel_f) {
  float value = 0;
  for (int row_f = 0; row_f < filter.h; row_f++) {
    for (int col_f = 0; col_f < filter.w; col_f++) {
      value += get_pixel(im, col + col_f, row + row_f, channel) * get_pixel(filter, col_f, row_f, channel_f);
    }
  }
  return value;
}

image convolve_image(image im, image filter, int preserve) {
  assert(im.c == filter.c || filter.c == 1);
  assert(filter.w == filter.h);

  int padding = (filter.w - 1) / 2;
  image result = make_image(im.w, im.h, preserve ? im.c : 1);
  im = add_padding(im, padding);

  for (int channel = 0; channel < im.c; channel++) {
    int channel_f = filter.c == 1 ? 0 : channel;
    for (int row = 0; row < result.h; row++) {
      for (int col = 0; col < result.w; col++) {
        float value = convolve_pixel(im, filter, col, row, channel, channel_f);
        if (preserve) {
          set_pixel(result, col, row, channel, value);
        } else {
          float org_value = get_pixel(result, col, row, 0);
          set_pixel(result, col, row, 0, value + org_value);
        }
      }
    }
  }

  free_image(im);
  return result;
}

image make_filter_from_template(int *filter_template, int size) {
  image filter = make_image(3, 3, 1);
  for (int i = 0; i < size; i++) {
    filter.data[i] = filter_template[i];
  }
  return filter;
}

image make_highpass_filter() {
  int filter_template[9] = {0, -1, 0, -1, 4, -1, 0, -1, 0};
  return make_filter_from_template(filter_template, 9);
}

image make_sharpen_filter() {
  int filter_template[9] = {0, -1, 0, -1, 5, -1, 0, -1, 0};
  return make_filter_from_template(filter_template, 9);
}

image make_emboss_filter() {
  int filter_template[9] = {-2, -1, 0, -1, 1, 1, 0, 1, 2};
  return make_filter_from_template(filter_template, 9);
}

/*
Question 2.2.1: Which of these filters should we use preserve when we run our convolution and which ones should we not? Why?
Answer:
Sharpen and Emboss need preserve, but Highpass does not.
When convolving with Sharpen and Emboss, the resulting images have color. So,
if do not preserve on these two filters, the resulting images loss the color.
On the other hand, when convolving with Highpass, the resulting image does not have
color (only has color in the sharp lines). So, not using preserve will produce
a resulting image with more emphasised sharp lines.
*/

/*
Question 2.2.2: Do we have to do any post-processing for the above filters? Which ones and why?
Answer:
All of them. When convoling, some pixel value might be out of range [0, 1).
So, we need to cap every pixel value to be between [0, 1)
*/

float get_gaussian_value(int x, int y, float sigma) {
  float two_sigma2 = 2.0 * sigma * sigma;
  return exp(-((x * x) + (y * y)) / two_sigma2) / (M_PI * two_sigma2);
}

image make_gaussian_filter(float sigma) {
  int width = (int)ceilf(6 * sigma);
  width += 1 - (width % 2);

  int mid = (width - 1) / 2;
  image filter = make_image(width, width, 1);
  for (int row = 0; row < width; row++) {
    for (int col = 0; col < width; col++) {
      float value = get_gaussian_value(col - mid, row - mid, sigma);
      set_pixel(filter, col, row, 0, value);
    }
  }
  l1_normalize(filter);
  return filter;
}

image add_image(image a, image b) {
  assert(a.w == b.w && a.h == b.h && a.c == b.c);
  image result = make_image(a.w, a.h, a.c);
  int result_size = result.w * result.h * result.c;
  for (int i = 0; i < result_size; i++) {
    result.data[i] = a.data[i] + b.data[i];
  }
  return result;
}

image sub_image(image a, image b) {
  assert(a.w == b.w && a.h == b.h && a.c == b.c);
  image result = make_image(a.w, a.h, a.c);
  int result_size = result.w * result.h * result.c;
  for (int i = 0; i < result_size; i++) {
    result.data[i] = a.data[i] - b.data[i];
  }
  return result;
}

image make_gx_filter() {
  int filter_template[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
  return make_filter_from_template(filter_template, 9);
}

image make_gy_filter() {
  int filter_template[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};
  return make_filter_from_template(filter_template, 9);
}

void feature_normalize(image im) {
  float min = __FLT_MAX__, max = __FLT_MIN__;
  int size = im.w * im.h * im.c;
  for (int i = 0; i < size; i++) {
    float datum = im.data[i];
    min = datum < min ? datum : min;
    max = datum > max ? datum : max;
  }

  float range = max - min;
  if (range == 0) {
    for (int i = 0; i < size; i++) im.data[i] = 0;
  } else {
    for (int i = 0; i < size; i++) {
      im.data[i] = (im.data[i] - min) / range;
    }
  }
}

image *sobel_image(image im) {
  image gx_filter = make_gx_filter();
  image gy_filter = make_gy_filter();
  image gx = convolve_image(im, gx_filter, 0);
  image gy = convolve_image(im, gy_filter, 0);
  image *result = calloc(2, sizeof(image));
  image g = make_image(im.w, im.h, 1);
  image theta = make_image(im.w, im.h, 1);
  int size = im.w * im.h;
  for (int i = 0; i < size; i++) {
    float x_datum = gx.data[i];
    float y_datum = gy.data[i];
    g.data[i] = sqrtf((x_datum * x_datum) + (y_datum * y_datum));
    theta.data[i] = atan2f(y_datum, x_datum);
  }
  result[0] = g;
  result[1] = theta;

  free_image(gx);
  free_image(gy);
  free_image(gx_filter);
  free_image(gy_filter);
  return result;
}

image colorize_sobel(image im) {
  int num_channel = 3;
  image *sobel_result = sobel_image(im);
  image grad = sobel_result[0];
  image theta = sobel_result[1];
  image result = make_image(im.w, im.h, num_channel);

  feature_normalize(grad);
  feature_normalize(theta);

  for (int row = 0; row < im.h; row++) {
    for (int col = 0; col < im.w; col++) {
      float grad_datum = get_pixel(grad, col, row, 0);
      float theta_datum = get_pixel(theta, col, row, 0);

      set_pixel(result, col, row, 0, theta_datum);
      set_pixel(result, col, row, 1, grad_datum);
      set_pixel(result, col, row, 2, grad_datum);
    }
  }

  hsv_to_rgb(result);
  return result;
}
