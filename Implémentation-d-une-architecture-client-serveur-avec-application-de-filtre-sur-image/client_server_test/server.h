//  server.h : partie interface du module de coordination (Serveur) pour la
//    gestion centralisée des requêtes de filtrage d'images.
//
//  Fonctionnement général :
//  - le module initialise les ressources IPC globales (SHM et sémaphore nommé)
// ;
//  - il bascule en mode démon pour s'exécuter en arrière-plan sans terminal ;
//  - il implémente une boucle de consommation passive : le processus s'endort
//      sur un sémaphore et ne consomme aucun cycle CPU tant qu'aucune requête
//      n'est déposée par un client ;
//  - pour chaque requête, il génère un processus fils (Worker) via fork()
//      garantissant l'isolation des traitements ;
//  - il assure le nettoyage automatique des ressources système lors de sa
//      fermeture (suppression de la SHM et du sémaphore).

#ifndef SERVER__H
#define SERVER__H

#include "common.h"
#include "worker.h" // Nécessaire pour worker_process

//- PROCÉDURES DU MODULE --v---v---v---v---v---v---v---v---v---v---v---v---v---v

//  cleanup : libère le segment de mémoire partagée et ferme/supprime le
//    sémaphore nommé SEM_NAME. Affiche un compte-rendu sur stderr.
extern void cleanup(void);

//  init_resources : crée et initialise le segment de mémoire partagée ainsi
//    que le sémaphore de synchronisation. En cas d'échec, le programme
//    s'arrête avec un message d'erreur.
extern void init_resources(void);

//  daemonize : détache le serveur du terminal de contrôle, crée une nouvelle
//    session (setsid) et redirige les entrées/sorties standards pour permettre
//    une exécution permanente en arrière-plan.
extern void daemonize(void);

//  sigchld_handler : traite le signal SIGCHLD de manière asynchrone pour
//    éliminer les processus "zombies" créés après la fin de l'exécution
//    des processus Workers.
extern void sigchld_handler(int signum);

//  main : point d'entrée du serveur. Configure les signaux, initialise les
//    ressources, passe en mode démon et entre dans la boucle infinie de
//    consommation des requêtes.
int main(int argc, char *argv[]);

#endif
