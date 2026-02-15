//  client.h : partie interface du module utilisateur (Client) pour la
//    transmission de requêtes de filtrage et la réception de flux BMP.

//  Fonctionnement général :
//  - le module agit comme un producteur dans un tampon circulaire situé en
//      mémoire partagée (SHM) ;
//  - il utilise un sémaphore nommé pour notifier le serveur de la présence
//      d'une nouvelle requête ;
//  - la réception du résultat s'effectue via un tube nommé (FIFO) dont le
//      nom est basé sur le PID du processus client pour garantir l'unicité ;
//  - la fonction cleanup_client assure la libération systématique des
//      ressources IPC et la suppression de la FIFO en fin d'exécution ;
//  - le module stocke l'image résultante localement sous le nom « result.bmp ».

#ifndef CLIENT__H
#define CLIENT__H

#include "common.h"

//- PROCÉDURES DU MODULE --v---v---v---v---v---v---v---v---v---v---v---v---v---v

//  cleanup_client : libère les ressources de mémoire partagée (shmdt) et
//    supprime la FIFO du système de fichiers (unlink). Cette procédure est
//    conçue pour être enregistrée via atexit() afin de garantir le nettoyage
//    systématique à la fermeture du programme.
extern void cleanup_client(void);

//  main : point d'entrée principal du programme client. Orchestre l'ouverture
//    des IPC, le dépôt de la requête struct filter_request dans le segment SHM,
//    la notification du serveur via le sémaphore SEM_NAME et la reconstruction
//    du fichier BMP final reçu par la FIFO.
int main(int argc, char *argv[]);

#endif
