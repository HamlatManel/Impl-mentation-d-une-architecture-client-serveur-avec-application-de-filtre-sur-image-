#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>

#include "server.h"

int shm_id = -1;
struct shm_request_segment *shm_ptr = nullptr;
sem_t *sem_req = nullptr;
sem_t *sem_mutex = nullptr;

//- GESTION DES RESSOURCES --v---v---v---v---v---v---v---v---v---v---v---v---v--

void cleanup(void) {
  if (shm_ptr != nullptr && shm_ptr != (void *) -1) {
    shmdt(shm_ptr);
  }
  if (shm_id != -1) {
    shmctl(shm_id, IPC_RMID, nullptr);
    fprintf(stderr, "Serveur: SHM libérée.\n");
  }
  if (sem_req != nullptr) {
    sem_close(sem_req);
    sem_unlink(SEM_NAME);
    fprintf(stderr, "Serveur: Sémaphore de requête supprimé.\n");
  }
  if (sem_mutex != nullptr) {
    sem_close(sem_mutex);
    sem_unlink(SEM_MUTEX_NAME);
    fprintf(stderr, "Serveur: Sémaphore Mutex supprimé.\n");
  }
}

void init_resources(void) {
  key_t key = ftok(".", SHM_REQUEST_KEY);
  shm_id = shmget(key, sizeof(struct shm_request_segment), IPC_CREAT | 0666);
  if (shm_id < 0) {
    perror("shmget");
    exit(EXIT_FAILURE);
  }
  shm_ptr = (struct shm_request_segment *) shmat(shm_id, nullptr, 0);
  shm_ptr->write_index = 0;
  shm_ptr->read_index = 0;
  sem_req = sem_open(SEM_NAME, O_CREAT, 0666, 0);
  sem_mutex = sem_open(SEM_MUTEX_NAME, O_CREAT, 0666, 1);
  if (sem_req == SEM_FAILED || sem_mutex == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }
}

//- SYSTÈME ET SIGNAUX --v---v---v---v---v---v---v---v---v---v---v---v---v---v--

void daemonize(void) {
  pid_t pid = fork();
  if (pid < 0) {
    exit(EXIT_FAILURE);
  }
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }
  if (setsid() < 0) {
    exit(EXIT_FAILURE);
  }
  umask(0);
  //if (chdir("/") != 0) {
  //exit(EXIT_FAILURE);
  //}
  close(STDIN_FILENO);
}

void sigchld_handler(int signum) {
  (void) signum;
  while (waitpid(-1, nullptr, WNOHANG) > 0) {
  }
}

//- POINT D'ENTRÉE --v---v---v---v---v---v---v---v---v---v---v---v---v---v---v--

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sa, nullptr);
  init_resources();
  daemonize();
  atexit(cleanup);
  fprintf(stderr, "Serveur: En attente de requêtes...\n");
  while (1) {
    if (sem_wait(sem_req) == -1) {
      if (errno == EINTR) {
        continue;
      }
      perror("sem_wait");
      break;
    }
    struct filter_request req = shm_ptr->requests[shm_ptr->read_index];
    shm_ptr->read_index = (shm_ptr->read_index + 1) % (int) MAX_REQUESTS;
    pid_t pid = fork();
    if (pid == 0) {
      sem_close(sem_req);
      sem_close(sem_mutex);
      worker_process(req);
      _exit(EXIT_SUCCESS);
    }
  }
  return 0;
}
