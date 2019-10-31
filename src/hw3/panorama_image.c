#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "matrix.h"

// Comparator for matches
// const void *a, *b: pointers to the matches to compare.
// returns: result of comparison, 0 if same, 1 if a > b, -1 if a < b.
int match_compare(const void *a, const void *b) {
  match *ra = (match *)a;
  match *rb = (match *)b;
  if (ra->distance < rb->distance)
    return -1;
  else if (ra->distance > rb->distance)
    return 1;
  else
    return 0;
}

// Helper function to create 2d points.
// float x, y: coordinates of point.
// returns: the point.
point make_point(float x, float y) {
  point p;
  p.x = x;
  p.y = y;
  return p;
}

// Place two images side by side on canvas, for drawing matching pixels.
// image a, b: images to place.
// returns: image with both a and b side-by-side.
image both_images(image a, image b) {
  image both =
      make_image(a.w + b.w, a.h > b.h ? a.h : b.h, a.c > b.c ? a.c : b.c);
  int i, j, k;
  for (k = 0; k < a.c; ++k) {
    for (j = 0; j < a.h; ++j) {
      for (i = 0; i < a.w; ++i) {
        set_pixel(both, i, j, k, get_pixel(a, i, j, k));
      }
    }
  }
  for (k = 0; k < b.c; ++k) {
    for (j = 0; j < b.h; ++j) {
      for (i = 0; i < b.w; ++i) {
        set_pixel(both, i + a.w, j, k, get_pixel(b, i, j, k));
      }
    }
  }
  return both;
}

// Draws lines between matching pixels in two images.
// image a, b: two images that have matches.
// match *matches: array of matches between a and b.
// int n: number of matches.
// int inliers: number of inliers at beginning of matches, drawn in green.
// returns: image with matches drawn between a and b on same canvas.
image draw_matches(image a, image b, match *matches, int n, int inliers) {
  image both = both_images(a, b);
  int i, j;
  for (i = 0; i < n; ++i) {
    int bx = matches[i].p.x;
    int ex = matches[i].q.x;
    int by = matches[i].p.y;
    int ey = matches[i].q.y;
    for (j = bx; j < ex + a.w; ++j) {
      int r = (float)(j - bx) / (ex + a.w - bx) * (ey - by) + by;
      set_pixel(both, j, r, 0, i < inliers ? 0 : 1);
      set_pixel(both, j, r, 1, i < inliers ? 1 : 0);
      set_pixel(both, j, r, 2, 0);
    }
  }
  return both;
}

// Draw the matches with inliers in green between two images.
// image a, b: two images to match.
// matches *
image draw_inliers(image a, image b, matrix H, match *m, int n, float thresh) {
  int inliers = model_inliers(H, m, n, thresh);
  image lines = draw_matches(a, b, m, n, inliers);
  return lines;
}

// Find corners, match them, and draw them between two images.
// image a, b: images to match.
// float sigma: gaussian for harris corner detector. Typical: 2
// float thresh: threshold for corner/no corner. Typical: 1-5
// int nms: window to perform nms on. Typical: 3
image find_and_draw_matches(image a, image b, float sigma, float thresh,
                            int nms) {
  int an = 0;
  int bn = 0;
  int mn = 0;
  descriptor *ad = harris_corner_detector(a, sigma, thresh, nms, &an);
  descriptor *bd = harris_corner_detector(b, sigma, thresh, nms, &bn);
  match *m = match_descriptors(ad, an, bd, bn, &mn);

  mark_corners(a, ad, an);
  mark_corners(b, bd, bn);
  image lines = draw_matches(a, b, m, mn, 0);

  free_descriptors(ad, an);
  free_descriptors(bd, bn);
  free(m);
  return lines;
}

// Calculates L1 distance between to floating point arrays.
// float *a, *b: arrays to compare.
// int n: number of values in each array.
// returns: l1 distance between arrays (sum of absolute differences).
float l1_distance(float *a, float *b, int n) {
  float dis = 0.0;
  for (int i = 0; i < n; i++) {
    dis += fabsf(a[i] - b[i]);
  }
  return dis;
}

// Finds best matches between descriptors of two images.
// descriptor *a, *b: array of descriptors for pixels in two images.
// int an, bn: number of descriptors in arrays a and b.
// int *mn: pointer to number of matches found, to be filled in by function.
// returns: best matches found. each descriptor in a should match with at most
//          one other descriptor in b.
match *match_descriptors(descriptor *a, int an, descriptor *b, int bn,
                         int *mn) {
  int i, j;

  // We will have at most an matches.
  *mn = an;
  int data_n = an > 0 ? a[0].n : 0;
  match *m = calloc(an, sizeof(match));
  for (j = 0; j < an; ++j) {
    float min_dis = __FLT_MAX__;
    int bind = 0;  // <- find the best match
    for (i = 0; i < bn; i++) {
      float dis = l1_distance(a[j].data, b[i].data, data_n);
      if (dis < min_dis) {
        min_dis = dis;
        bind = i;
      }
    }
    m[j].ai = j;
    m[j].bi = bind;  // <- should be index in b.
    m[j].p = a[j].p;
    m[j].q = b[bind].p;
    m[j].distance = min_dis;  // <- should be the smallest L1 distance!
  }

  int count = 0;
  int8_t *seen = calloc(bn, sizeof(int8_t));
  qsort((void *)m, an, sizeof(m[0]), match_compare);
  for (i = 0; i < an; i++) {
    if (!seen[m[i].bi]) {
      seen[m[i].bi] = 1;
      m[count++] = m[i];
    }
  }
  *mn = count;
  free(seen);
  return m;
}

// Apply a projective transformation to a point.
// matrix H: homography to project point.
// point p: point to project.
// returns: point projected using the homography.
point project_point(matrix H, point p) {
  matrix c = make_matrix(3, 1);
  c.data[0][0] = p.x;
  c.data[1][0] = p.y;
  c.data[2][0] = 1;
  matrix result = matrix_mult_matrix(H, c);
  free_matrix(c);
  float w = result.data[2][0];
  // float w = 1;
  point result_p = make_point(result.data[0][0] / w, result.data[1][0] / w);
  free_matrix(result);
  return result_p;
}

// Calculate L2 distance between two points.
// point p, q: points.
// returns: L2 distance between them.
float point_distance(point p, point q) {
  float dx = p.x - q.x;
  float dy = p.y - q.y;
  return sqrtf((dx * dx) + (dy * dy));
}

// Count number of inliers in a set of matches. Should also bring inliers
// to the front of the array.
// matrix H: homography between coordinate systems.
// match *m: matches to compute inlier/outlier.
// int n: number of matches in m.
// float thresh: threshold to be an inlier.
// returns: number of inliers whose projected point falls within thresh of
//          their match in the other image. Should also rearrange matches
//          so that the inliers are first in the array. For drawing.
int model_inliers(matrix H, match *m, int n, float thresh) {
  int i, count = n;
  match tmp;
  for (i = 0; i < count; i++) {
    // printf("%d %d\n", H.rows, H.cols);
    float dis = point_distance(project_point(H, m[i].p), m[i].q);
    printf("%f-----------\n", dis);
    if (dis >= thresh) {
      count--;
      tmp = m[count];
      m[count] = m[i];
      m[i] = tmp;
      i--;
    }
  }
  printf("----------%f-------------\n", thresh);
  return count;
}

// Randomly shuffle matches for RANSAC.
// match *m: matches to shuffle in place.
// int n: number of elements in matches.
void randomize_matches(match *m, int n) {
  match tmp;
  int i, j;
  for (i = n - 1; i >= 1; i--) {
    j = rand() % (i + 1);
    tmp = m[i];
    m[i] = m[j];
    m[j] = tmp;
  }
}

void fill_M_row(double *row, double x, double y, double p, int offset) {
  row[0] = 0;
  row[1] = 0;
  row[2] = 0;
  row[3] = 0;
  row[4] = 0;
  row[5] = 0;
  row[offset] = x;
  row[offset + 1] = y;
  row[offset + 2] = 1;
  row[6] = -x * p;
  row[7] = -y * p;
}

// Computes homography between two images given matching pixels.
// match *matches: matching points between images.
// int n: number of matches to use in calculating homography.
// returns: matrix representing homography H that maps image a to image b.
matrix compute_homography(match *matches, int n) {
  matrix M = make_matrix(n * 2, 8);
  matrix b = make_matrix(n * 2, 1);

  int i;
  for (i = 0; i < n; ++i) {
    int idx = i * 2;
    int idx2 = idx + 1;
    double x = matches[i].p.x;
    double xp = matches[i].q.x;
    double y = matches[i].p.y;
    double yp = matches[i].q.y;
    // printf("%d %d-----\n", matches[i].ai, matches[i].bi);
    b.data[idx][0] = xp;
    b.data[idx2][0] = yp;
    fill_M_row(M.data[idx], x, y, xp, 0);
    fill_M_row(M.data[idx2], x, y, yp, 3);
  }
  // printf("-------\n");
  // print_matrix(M);
  // print_matrix(b);
  matrix a = solve_system(M, b);
  // print_matrix(a);
  free_matrix(M);
  free_matrix(b);

  // If a solution can't be found, return empty matrix;
  matrix none = {0};
  if (!a.data) return none;

  matrix H = make_matrix(3, 3);
  int size_a = a.cols * a.rows;
  for (i = 0; i < size_a; i++) {
    H.data[i / H.cols][i % H.cols] = a.data[i][0];
  }
  H.data[2][2] = 1;
  // print_matrix(H);

  free_matrix(a);
  return H;
}

// int count_inliners(match *m, int n, matrix H, float thresh) {
//   int i;
//   int inliners = 0;
//   matrix p = make_matrix(3, 1);
//   p.data[2][0] = 1;
//   for (i = 0; i < n; i++) {
//     point q = project_point(H, m[i].p);
//     if (point_distance(q, m[i].q) < thresh) {
//       inliners++;
//     }
//   }
//   free_matrix(p);
//   return inliners;
// }

// Perform RANdom SAmple Consensus to calculate homography for noisy
// matches. match *m: set of matches. int n: number of matches. float
// thresh: inlier/outlier distance threshold. int k: number of iterations to
// run. int cutoff: inlier cutoff to exit early. returns: matrix
// representing most common homography between matches.
matrix RANSAC(match *m, int n, float thresh, int k, int cutoff) {
  int e;
  int best = 0;
  matrix Hb = make_translation_homography(256, 0);
  // TODO: fill in RANSAC algorithm.
  // for k iterations:
  //     shuffle the matches
  //     compute a homography with a few matches (how many??)
  //     if new homography is better than old (how can you tell?):
  //         compute updated homography using all inliers
  //         remember it and how good it is
  //         if it's better than the cutoff:
  //             return it immediately
  // if we get to the end return the best homography
  int inliners, match_size = 3;  // what should be the match size???
  for (e = 0; e < k; e++) {
    randomize_matches(m, n);
    // for (int i = 0; i < n; i++) {
    //   printf("%d ", m[i].ai);
    // }
    // printf("\n----------------\n");
    matrix H = compute_homography(m, match_size);
    if (!!H.data) {
      // printf("%d %d\n", H.rows, H.cols);
      inliners = model_inliers(H, m, n, thresh);
      // free_matrix(H);
      if (inliners > best) {
        best = inliners;
        printf("----%d---%d---%d-----\n", inliners, n, cutoff);
        H = compute_homography(m, inliners);
        if (!!H.data) {
          // free_matrix(Hb);
          Hb = H;
          printf("%d %d %lld %d", Hb.rows, Hb.cols, (long long)Hb.data,
                 Hb.shallow);
          if (model_inliers(H, m, n, thresh) > cutoff) {
            return H;
          }
        }
      }
    }
  }
  return Hb;
}

// Stitches two images together using a projective transformation.
// image a, b: images to stitch.
// matrix H: homography from image a coordinates to image b coordinates.
// returns: combined image stitched together.
image combine_images(image a, image b, matrix H) {
  matrix Hinv = matrix_invert(H);

  // Project the corners of image b into image a coordinates.
  point c1 = project_point(Hinv, make_point(0, 0));
  point c2 = project_point(Hinv, make_point(b.w - 1, 0));
  point c3 = project_point(Hinv, make_point(0, b.h - 1));
  point c4 = project_point(Hinv, make_point(b.w - 1, b.h - 1));

  // Find top left and bottom right corners of image b warped into image a.
  point topleft, botright;
  botright.x = MAX(c1.x, MAX(c2.x, MAX(c3.x, c4.x)));
  botright.y = MAX(c1.y, MAX(c2.y, MAX(c3.y, c4.y)));
  topleft.x = MIN(c1.x, MIN(c2.x, MIN(c3.x, c4.x)));
  topleft.y = MIN(c1.y, MIN(c2.y, MIN(c3.y, c4.y)));

  // Find how big our new image should be and the offsets from image a.
  int dx = MIN(0, topleft.x);
  int dy = MIN(0, topleft.y);
  int w = MAX(a.w, botright.x) - dx;
  int h = MAX(a.h, botright.y) - dy;

  // Can disable this if you are making very big panoramas.
  // Usually this means there was an error in calculating H.
  if (w > 7000 || h > 7000) {
    fprintf(stderr, "output too big, stopping\n");
    return copy_image(a);
  }

  int x, y, channel;
  float value;
  image c = make_image(w, h, a.c);

  // Paste image a into the new image offset by dx and dy.
  for (channel = 0; channel < a.c; ++channel) {
    for (y = 0; y < a.h; ++y) {
      for (x = 0; x < a.w; ++x) {
        value = get_pixel(a, x, y, channel);
        set_pixel(c, x + dx, y + dy, channel, value);
      }
    }
  }

  // TODO: Paste in image b as well.
  // You should loop over some points in the new image (which? all?)
  // and see if their projection from a coordinates to b coordinates falls
  // inside of the bounds of image b. If so, use bilinear interpolation to
  // estimate the value of b at that projection, then fill in image c.
  float bx, by;
  matrix q, p = make_matrix(3, 1);
  p.data[2][0] = 1;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      p.data[0][0] = x;
      p.data[1][0] = y;
      q = matrix_mult_matrix(H, p);
      bx = q.data[0][0];
      by = q.data[1][0];
      if (0 <= bx && bx < b.w && 0 <= by && by <= b.h) {
        for (channel = 0; channel < b.c; channel++) {
          value = bilinear_interpolate(b, bx, by, channel);
          set_pixel(c, x, y, channel, value);
        }
      }
    }
  }

  return c;
}

// Create a panoramam between two images.
// image a, b: images to stitch together.
// float sigma: gaussian for harris corner detector. Typical: 2
// float thresh: threshold for corner/no corner. Typical: 1-5
// int nms: window to perform nms on. Typical: 3
// float inlier_thresh: threshold for RANSAC inliers. Typical: 2-5
// int iters: number of RANSAC iterations. Typical: 1,000-50,000
// int cutoff: RANSAC inlier cutoff. Typical: 10-100
image panorama_image(image a, image b, float sigma, float thresh, int nms,
                     float inlier_thresh, int iters, int cutoff) {
  srand(10);
  int an = 0;
  int bn = 0;
  int mn = 0;

  // Calculate corners and descriptors
  descriptor *ad = harris_corner_detector(a, sigma, thresh, nms, &an);
  descriptor *bd = harris_corner_detector(b, sigma, thresh, nms, &bn);

  // Find matches
  match *m = match_descriptors(ad, an, bd, bn, &mn);

  // Run RANSAC to find the homography
  matrix H = RANSAC(m, mn, inlier_thresh, iters, cutoff);

  if (1) {
    // Mark corners and matches between images
    mark_corners(a, ad, an);
    mark_corners(b, bd, bn);
    image inlier_matches = draw_inliers(a, b, H, m, mn, inlier_thresh);
    save_image(inlier_matches, "inliers");
  }

  free_descriptors(ad, an);
  free_descriptors(bd, bn);
  free(m);

  // Stitch the images together with the homography
  image comb = combine_images(a, b, H);
  return comb;
}

// Project an image onto a cylinder.
// image im: image to project.
// float f: focal length used to take image (in pixels).
// returns: image projected onto cylinder, then flattened.
image cylindrical_project(image im, float f) {
  // TODO: project image onto a cylinder
  image c = copy_image(im);
  return c;
}
