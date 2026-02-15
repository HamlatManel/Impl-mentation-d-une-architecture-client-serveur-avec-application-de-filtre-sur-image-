//  common.h : Interface de communication et structures de données partagées
//    pour le système de traitement d'images multi-processus et multi-threads.
//
//  Fonctionnement général :
//  - La communication entre le client et le serveur repose sur un segment
//      de mémoire partagée (SHM) et un sémaphore nommé pour la synchronisation.
//  - Le transfert des images traitées s'effectue via des tubes nommés (FIFOs).
//  - Les structures BMP sont alignées rigoureusement sur 1 octet pour garantir
//      la compatibilité avec le format de fichier binaire.

#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

//- CONSTANTES DE COMMUNICATION --v---v---v---v---v---v---v---v---v---v---v---v

//  SEM_NAME : Identifiant du sémaphore POSIX utilisé pour notifier le serveur
//    du dépôt d'une nouvelle requête dans la file d'attente.
#define SEM_NAME "/sem_image_server"

// SEM_MUTEX_NAME : Verrou pour protéger l'accès au tampon circulaire par
//    plusieurs clients
#define SEM_MUTEX_NAME "/sem_image_mutex"

//  SHM_REQUEST_KEY : Clé System V pour le segment de mémoire partagée contenant
//    le tampon circulaire de requêtes.
#define SHM_REQUEST_KEY 1234

//  SHM_REQUEST_SIZE : Taille allouée pour le segment de mémoire des requêtes.
#define SHM_REQUEST_SIZE 4096

//  FIFO_REP_PATH : Préfixe du chemin pour les tubes nommés de réponse.
//    Le PID du client est concaténé à ce préfixe pour l'unicité du canal.
#define FIFO_REP_PATH "/tmp/fifo_rep_"

//- PARAMÈTRES ET LIMITES --v---v---v---v---v---v---v---v---v---v---v---v---v--

#define FILTER_GRAYSCALE 1
#define FILTER_NEGATIVE 2
#define FILTER_BRIGHTNESS 3

//  MAX_IMAGE_SIZE : Limite de sécurité (100 Mo) pour éviter les débordements
//    mémoire lors du chargement de fichiers BMP malformés.
#define MAX_IMAGE_SIZE (100 * 1024 * 1024)

// NUM_THREADS : Nombre de threads par défaut pour le worker
#define NUM_THREADS 8

//- STRUCTURES DE REQUÊTES --v---v---v---v---v---v---v---v---v---v---v---v---v-

//  struct filter_request : Définit les paramètres d'un travail de filtrage.
struct filter_request {
  pid_t pid;
  char chemin[256];
  int filtre;
  int parametres[5];
};

//  MAX_REQUESTS : Nombre maximal de requêtes simultanées calculé selon
//    la taille du segment SHM.
#define MAX_REQUESTS (SHM_REQUEST_SIZE / sizeof(struct filter_request))

//  struct shm_request_segment : Structure du segment de mémoire partagée.
//    Gère une file d'attente circulaire entre Producteurs (Clients) et
//    Consommateurs (Serveur).
struct shm_request_segment {
  int write_index;
  int read_index;
  struct filter_request requests[MAX_REQUESTS];
};

//- FORMATS BINAIRES BMP (Alignement strict) --v---v---v---v---v---v---v---v---

#pragma pack(push, 1) // Désactive le rembourrage d'octets (Padding)

//  BMPFileHeader : Entête du fichier (14 octets).
typedef struct {
  uint16_t bfType;
  uint32_t bfSize;
  uint16_t bfReserved1;
  uint16_t bfReserved2;
  uint32_t bfOffBits;
} BMPFileHeader;

//  BMPInfoHeader : Entête d'information technique de l'image (DIB header).
typedef struct {
  uint32_t biSize;
  int32_t biWidth;
  int32_t biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  int32_t biXPelsPerMeter;
  int32_t biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
} BMPInfoHeader;

//  Pixel : Représentation d'un point coloré en format BGR (Blue-Green-Red).
typedef struct {
  unsigned char blue;
  unsigned char green;
  unsigned char red;
} Pixel;

#pragma pack(pop) //  Restaure l'alignement par défaut

//- STRUCTURES DE TRAVAIL (INTERNE) --v---v---v---v---v---v---v---v---v---v---v

//  struct image_data : Regroupe les métadonnées d'une image chargée en mémoire.
struct image_data {
  BMPFileHeader file_header;
  BMPInfoHeader info_header;
  int data_size;
};

//  struct thread_workspace : Contexte de travail envoyé à chaque thread POSIX.
//    Permet la division du travail par bandes de lignes (parallélisme de
//    données).
struct thread_workspace {
  int thread_id;
  struct image_data *shm_img;
  Pixel *pixel_data_ptr;
  int ligne_debut;
  int ligne_fin;
  int filtre;
};

#endif
