#include "float_vec.h"
#include <stdio.h>
#include <stdlib.h>

floats* make_floats(long nn) {
  floats* res = malloc(sizeof(floats));
  res->size = 0;
  res->cap = nn;
  float* data = malloc(nn * sizeof(float));
  res->data = data;
  return res;
}

void floats_push(floats* xs, float xx) {
  if(xs->size + 1 >= xs->cap) {
    xs->cap *= 2;
    float* data = malloc(xs->cap * sizeof(float));
    for(long i=0;i<xs->size;++i) {
      // printf("copying: %f\n", xs->data[i]);
      data[i] = xs->data[i];
    }
    free(xs->data);
    xs->data = data;
  }
  xs->data[xs->size++] = xx;
}

void free_floats(floats* xs) {
  free(xs->data);
  free(xs);
}
void floats_print(floats* xs) {
  for(long i=0;i<xs->size;i++) {
    printf("%f\n", xs->data[i]);
  }
}