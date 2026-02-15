#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>

#include "worker.h"
#include "image_ops.h"

//- LOGIQUE DES THREADS --v---v---v---v---v---v---v---v---v---v---v---v---v---v

void *thread_filter_task(void *arg) {
  struct thread_workspace *ws = (struct thread_workspace *) arg;
  int width = ws->shm_img->info_header.biWidth;
  int height = ws->shm_img->info_header.biHeight;
  switch (ws->filtre) {
    case FILTER_GRAYSCALE:
      apply_grayscale_filter(ws->pixel_data_ptr, width, height,
          ws->ligne_debut, ws->ligne_fin);
      break;
    case FILTER_NEGATIVE:
      apply_negative_filter(ws->pixel_data_ptr, width,
          ws->ligne_debut, ws->ligne_fin);
      break;
    case FILTER_BRIGHTNESS:
      {
        int adj = 50;
        for (int y = ws->ligne_debut; y < ws->ligne_fin; y++) {
          for (int x = 0; x < width; x++) {
            Pixel *p = &ws->pixel_data_ptr[y * width + x];
            int r = (int) p->red + adj;
            int g = (int) p->green + adj;
            int b = (int) p->blue + adj;
            p->red = (unsigned char) (r > 255 ? 255 : (r < 0 ? 0 : r));
            p->green = (unsigned char) (g > 255 ? 255 : (g < 0 ? 0 : g));
            p->blue = (unsigned char) (b > 255 ? 255 : (b < 0 ? 0 : b));
          }
        }
        break;
      }
    default:
      fprintf(stderr, "Thread %d: Filtre %d inconnu.\n", ws->thread_id,
          ws->filtre);
  }
  pthread_exit(nullptr);
}

//- LOGIQUE DU PROCESSUS --v---v---v---v---v---v---v---v---v---v---v---v---v---v

void worker_process(struct filter_request req) {
  struct image_data *img_shm_ptr = nullptr;
  char *pixel_data_base_ptr = nullptr;
  int total_shm_size = 0;
  if (load_bmp_image(req.chemin, &img_shm_ptr, &pixel_data_base_ptr,
        &total_shm_size) != 0) {
    fprintf(stderr, "Worker[%d]: Erreur chargement %s\n", getpid(), req.chemin);
    return;
  }
  int height = abs(img_shm_ptr->info_header.biHeight);
  int rows_per_thread = height / NUM_THREADS;
  pthread_t threads[NUM_THREADS];
  struct thread_workspace workspaces[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; i++) {
    workspaces[i].thread_id = i;
    workspaces[i].shm_img = img_shm_ptr;
    workspaces[i].pixel_data_ptr = (Pixel *) pixel_data_base_ptr;
    workspaces[i].filtre = req.filtre;
    workspaces[i].ligne_debut = i * rows_per_thread;
    workspaces[i].ligne_fin = (i
        == NUM_THREADS - 1) ? height : (i + 1) * rows_per_thread;
    pthread_create(&threads[i], nullptr, thread_filter_task, &workspaces[i]);
  }
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], nullptr);
  }
  char fifo_path[256];
  snprintf(fifo_path, sizeof(fifo_path), "%s%d", FIFO_REP_PATH, req.pid);
  int fd_fifo = open(fifo_path, O_WRONLY);
  if (fd_fifo != -1) {
    ssize_t ret;
    ret = write(fd_fifo, &(img_shm_ptr->file_header), sizeof(BMPFileHeader));
    if (ret < 0) {
      perror("Erreur write file_header");
    }
    ret = write(fd_fifo, &(img_shm_ptr->info_header), sizeof(BMPInfoHeader));
    if (ret < 0) {
      perror("Erreur write info_header");
    }
    ret = write(fd_fifo, pixel_data_base_ptr, (size_t) img_shm_ptr->data_size);
    if (ret < 0) {
      perror("Erreur write pixels");
    }
    close(fd_fifo);
  } else {
    perror("Worker: Erreur ouverture FIFO");
  }
  shmdt(img_shm_ptr);
}
