#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "image_ops.h"

//- CHARGEMENT ET MÉMOIRE --v---v---v---v---v---v---v---v---v---v---v---v---v---

int load_bmp_image(const char *path, struct image_data **img_ptr,
    char **pixel_data_ptr, int *total_shm_size_out) {
  FILE *f = fopen(path, "rb");
  if (f == nullptr) {
    perror("image_ops: fopen");
    return -1;
  }
  BMPFileHeader file_header;
  BMPInfoHeader info_header;
  if (fread(&file_header, 1, sizeof(BMPFileHeader), f) != sizeof(BMPFileHeader)
      || fread(&info_header, 1, sizeof(BMPInfoHeader),
      f) != sizeof(BMPInfoHeader)) {
    fclose(f);
    return -1;
  }
  if (file_header.bfType != 0x4D42) {
    fprintf(stderr, "image_ops: Format non BMP (0x4D42 attendu).\n");
    fclose(f);
    return -1;
  }
  int row_size = (int) ((info_header.biWidth * 3 + 3) & ~3);
  unsigned int pixel_data_size = info_header.biSizeImage;
  if (pixel_data_size == 0) {
    pixel_data_size = (unsigned int) (row_size * abs(info_header.biHeight));
  }
  size_t full_size = sizeof(struct image_data) + (size_t) pixel_data_size;
  *total_shm_size_out = (int) full_size;
  int shmid = shmget(IPC_PRIVATE, full_size, IPC_CREAT | 0666);
  if (shmid < 0) {
    fclose(f);
    return -1;
  }
  struct image_data *img_shm_ptr = (struct image_data *) shmat(shmid, nullptr,
        0);
  shmctl(shmid, IPC_RMID, nullptr);
  img_shm_ptr->file_header = file_header;
  img_shm_ptr->info_header = info_header;
  img_shm_ptr->data_size = (int) pixel_data_size;
  char *shm_pixel_data_base = (char *) img_shm_ptr + sizeof(struct image_data);
  fseek(f, (long) file_header.bfOffBits, SEEK_SET);
  size_t sz_read = (size_t) pixel_data_size;
  if (fread(shm_pixel_data_base, 1, sz_read, f) != sz_read) {
    shmdt(img_shm_ptr);
    fclose(f);
    return -1;
  }
  fclose(f);
  *img_ptr = img_shm_ptr;
  *pixel_data_ptr = shm_pixel_data_base;
  return 0;
}

//- FILTRES --v---v---v---v---v---v---v---v---v---v---v---v---v---v---v---v---v

void apply_grayscale_filter(Pixel *pixels, int width, int height, int start_row,
    int end_row) {
  (void) height;
  int row_size_with_padding = (int) ((width * 3 + 3) & ~3);
  for (int y = start_row; y < end_row; y++) {
    unsigned char *row_ptr = ((unsigned char *) pixels)
        + ((long) y * row_size_with_padding);
    for (int x = 0; x < width; x++) {
      Pixel *p = (Pixel *) (row_ptr + (x * 3));
      unsigned char gray = (unsigned char) (0.299 * (double) p->red
          + 0.587 * (double) p->green
          + 0.114 * (double) p->blue);
      p->red = p->green = p->blue = gray;
    }
  }
}

void apply_negative_filter(Pixel *pixels, int width, int start_row,
    int end_row) {
  int row_size_with_padding = ((width * 3 + 3) & ~3);
  for (int y = start_row; y < end_row; y++) {
    unsigned char *row_ptr = ((unsigned char *) pixels)
        + ((long) y * row_size_with_padding);
    for (int x = 0; x < width; x++) {
      Pixel *p = (Pixel *) (row_ptr + (x * 3));
      p->red = (unsigned char) (255 - p->red);
      p->green = (unsigned char) (255 - p->green);
      p->blue = (unsigned char) (255 - p->blue);
    }
  }
}
