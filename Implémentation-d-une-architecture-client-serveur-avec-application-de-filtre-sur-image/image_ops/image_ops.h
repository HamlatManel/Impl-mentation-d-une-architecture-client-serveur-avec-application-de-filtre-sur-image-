//  image_ops.h : partie interface du module de manipulation de fichiers BMP
//    et algorithmes de filtrage d'image.
//
//  Fonctionnement général :
//  - le module assure le décodage binaire des structures BMP (File et Info) ;
//  - il gère l'allocation de segments de mémoire partagée (SHM) privés ;
//  - il implémente les calculs d'alignement (padding) pour garantir un accès
//      mémoire correct aux pixels (alignement sur 4 octets) ;
//  - les algorithmes de filtrage sont conçus pour être "thread-safe" en ne
//      travaillant que sur des plages de lignes (start_row à end_row).

#ifndef IMAGE_OPS__H
#define IMAGE_OPS__H

#include "common.h"

//- GESTION BINAIRE ET MÉMOIRE --v---v---v---v---v---v---v---v---v---v---v---v--

//  load_bmp_image : ouvre le fichier au chemin path, vérifie la signature BMP
//    et alloue un segment SHM de taille suffisante. Remplit les structures
//    img_ptr (méta-données) et pixel_data_ptr (données brutes). Calcule la
//    taille totale dans total_shm_size_out. Renvoie 0 en cas de succès.
extern int load_bmp_image(const char *path, struct image_data **img_ptr,
    char **pixel_data_ptr, int *total_shm_size_out);

//- ALGORITHMES DE FILTRAGE --v---v---v---v---v---v---v---v---v---v---v---v---v

//  apply_grayscale_filter : transforme la zone de l'image définie par start_row
//    et end_row en niveaux de gris en utilisant les coefficients de luminance
//    standard (ITU-R BT.601).
extern void apply_grayscale_filter(Pixel *pixels, int width, int height,
    int start_row, int end_row);

//  apply_negative_filter : inverse les composantes colorimétriques (255 -
//    valeur) sur la plage de lignes spécifiée.
extern void apply_negative_filter(Pixel *pixels, int width,
    int start_row, int end_row);

#endif
