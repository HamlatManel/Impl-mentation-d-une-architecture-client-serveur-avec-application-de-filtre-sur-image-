//  worker.h : partie interface du module de traitement d'images multi-threadé
//    pour la réalisation de filtrages BMP en isolation de ressources.

//  Fonctionnement général :
//  - le module assure le traitement d'une image BMP chargée en mémoire partagée
//      privée (SHM) pour garantir l'isolation des données entre processus ;
//  - le parallélisme est mis en œuvre par un découpage de l'image en bandes
//      horizontales, chaque bande étant traitée par un thread POSIX distinct ;
//  - le nombre de threads utilisés est défini par la macro NUM_THREADS ;
//  - une fois le traitement achevé, les données (en-têtes et pixels) sont
//      transmises de manière séquentielle vers le tube nommé (FIFO) du client ;
//  - les fonctions du module vérifient systématiquement les retours des appels
//      système (write, open, shm, etc.) et signalent les erreurs sur stderr.

#ifndef WORKER__H
#define WORKER__H

#include "common.h"

//- PARAMÈTRES DE CONFIGURATION --v---v---v---v---v---v---v---v---v---v---v---v

//  NUM_THREADS : définit le nombre de threads POSIX créés pour le traitement
//    parallèle de l'image. Par défaut fixé à 8.
#define NUM_THREADS 8

//  load_bmp_image : tente de charger l'image située au chemin path dans un
//    segment de mémoire partagée. Affecte l'adresse du contrôleur à *img_ptr,
//    l'adresse des pixels à *pixel_data_ptr et la taille totale à
//    total_shm_size. Renvoie 0 en cas de succès, une valeur non nulle sinon.
extern int load_bmp_image(const char *path, struct image_data **img_ptr,
    char **pixel_data_ptr, int *total_shm_size);

//  apply_grayscale_filter : applique la transformation en niveaux de gris sur
//    la plage de lignes comprise entre start_row et end_row (exclue) pour
//    l'image pointée par pixels de dimensions width x height.
extern void apply_grayscale_filter(Pixel *pixels, int width, int height,
    int start_row, int end_row);

//  worker_process : point d'entrée principal du processus ouvrier. Récupère
//    les ressources SHM, orchestre la création et la synchronisation des
//    threads de filtrage définis par req.filtre, puis transmet l'image
//    résultante via la FIFO associée au PID du client demandeur.
extern void worker_process(struct filter_request req);

//  thread_filter_task : routine de démarrage pour les threads POSIX. Interprète
//    le paramètre arg comme un pointeur vers un thread_workspace pour
//    appliquer le filtre requis (Gris, Négatif ou Luminosité) sur la zone
//    mémoire assignée. Termine le thread par un appel à pthread_exit.
extern void *thread_filter_task(void *arg);

#endif
