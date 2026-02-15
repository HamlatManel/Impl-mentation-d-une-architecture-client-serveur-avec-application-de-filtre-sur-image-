#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include <poll.h>

#include "client.h"

int shm_id = -1;
struct shm_request_segment *shm_ptr = nullptr;
char fifo_path[256] = { 0 };

//- GESTION DES RESSOURCES --v---v---v---v---v---v---v---v---v---v---v---v---v--

void cleanup_client(void) {
  if (shm_ptr != nullptr && shm_ptr != (void *) -1) {
    shmdt(shm_ptr);
  }
  if (fifo_path[0] != 0) {
    unlink(fifo_path);
  }
}

//- LOGIQUE PRINCIPALE --v---v---v---v---v---v---v---v---v---v---v---v---v---v--

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <chemin_image> <filtre_id> [param...]\n",
        argv[0]);
    return EXIT_FAILURE;
  }
  atexit(cleanup_client);
  snprintf(fifo_path, sizeof(fifo_path), "%s%d", FIFO_REP_PATH, getpid());
  if (mkfifo(fifo_path, 0666) < 0 && errno != EEXIST) {
    perror("mkfifo");
    return EXIT_FAILURE;
  }
  key_t key = ftok(".", SHM_REQUEST_KEY);
  shm_id = shmget(key, 0, 0);
  if (shm_id < 0) {
    fprintf(stderr, "Erreur: Serveur non détecté (SHM inaccessible).\n");
    return EXIT_FAILURE;
  }
  shm_ptr = (struct shm_request_segment *) shmat(shm_id, nullptr, 0);
  sem_t *sem_req = sem_open(SEM_NAME, 0);
  sem_t *sem_mutex = sem_open(SEM_MUTEX_NAME, 0);
  if (sem_req == SEM_FAILED || sem_mutex == SEM_FAILED) {
    perror("sem_open");
    return EXIT_FAILURE;
  }
  struct filter_request req;
  req.pid = getpid();
  strncpy(req.chemin, argv[1], sizeof(req.chemin) - 1);
  req.chemin[sizeof(req.chemin) - 1] = '\0';
  req.filtre = atoi(argv[2]);
  sem_wait(sem_mutex);
  int idx = shm_ptr->write_index;
  shm_ptr->requests[idx] = req;
  shm_ptr->write_index = (idx + 1) % (int) MAX_REQUESTS;
  sem_post(sem_mutex);
  printf("Client[%d]: Envoi requête (filtre %d) et notification serveur.\n",
      getpid(), req.filtre);
  sem_post(sem_req);
  sem_close(sem_req);
  sem_close(sem_mutex);
  int fd_fifo = open(fifo_path, O_RDONLY | O_NONBLOCK);
  if (fd_fifo == -1) {
    perror("open FIFO");
    return EXIT_FAILURE;
  }
  struct pollfd pfd = { .fd = fd_fifo, .events = POLLIN };
  printf("Client[%d]: Attente des données (timeout 5s)...\n", getpid());
  int ret = poll(&pfd, 1, 5000);
  if (ret <= 0) {
    if (ret == 0) {
      fprintf(stderr,
          "Erreur : Timeout ! Le worker est trop lent ou a crashé.\n");
    } else {
      perror("poll");
    }
    close(fd_fifo);
    return EXIT_FAILURE;
  }
  int flags = fcntl(fd_fifo, F_GETFL, 0);
  fcntl(fd_fifo, F_SETFL, flags & ~O_NONBLOCK);
  FILE *output_file = fopen("result.bmp", "wb");
  if (!output_file) {
    perror("fopen result.bmp");
    close(fd_fifo);
    return EXIT_FAILURE;
  }
  char buffer[4096];
  ssize_t bytes_read;
  printf("Client[%d]: Réception des données...\n", getpid());
  while ((bytes_read = read(fd_fifo, buffer, sizeof(buffer))) > 0) {
    if (fwrite(buffer, 1, (size_t) bytes_read,
          output_file) < (size_t) bytes_read) {
      perror("fwrite");
      break;
    }
  }
  fflush(output_file);
  fsync(fileno(output_file));
  fclose(output_file);
  close(fd_fifo);
  printf("Client[%d]: Succès. Image sauvegardée dans 'result.bmp'.\n",
      getpid());
  return EXIT_SUCCESS;
}
