#include "fonctions.h"

int write_in_queue(RT_QUEUE *msgQueue, void * data, int size);

/*Le thread envoyer permet d'envoyer les messages du shuttle vers le moniteur*/
void envoyer(void * arg) {
    DMessage *msg;
    int err;

    while (1) {
        printf("tenvoyer : Attente d'un message\n");
        if ((err = rt_queue_read(&queueMsgGUI, &msg, sizeof (DMessage), TM_INFINITE)) >= 0) {
            printf("tenvoyer : envoi d'un message au moniteur\n");
            serveur->send(serveur, msg);
            msg->free(msg);
        } else {
            printf("Error msg queue write: %s\n", strerror(-err));
        }
    }
}


/*Le thread connecter permet d'établir la connexion avec le robot. Il est lancé lorsque l'utilisateur 
 demande la connexion robot depuis le moniteur*/
void connecter(void * arg) {
    int status;
    int err;
    DMessage *message;
    printf("tconnect : Debut de l'exécution de tconnect\n");

    while (1) {
        printf("tconnect : Attente du sémarphore semConnecterRobot\n");
        rt_sem_p(&semConnecterRobot, TM_INFINITE);
        printf("tconnect : Ouverture de la communication avec le robot\n");
        status = robot->open_device(robot);

        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        etatCommRobot = status;
        rt_mutex_release(&mutexEtat);

        if (status == STATUS_OK) {
            status = robot->start(robot);
            if (status == STATUS_OK) {
                printf("tconnect : Robot démarrer\n");
            }
        }

        message = d_new_message();
        message->put_state(message, status);

        printf("tconnecter : Envoi message\n");
        message->print(message, 100);

        if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
            message->free(message);
            printf("tconnecter : dans le if\n");
        }
        printf("tconnecter : sortie du if\n");
        err = rt_event_signal(&evCoPerdue, 1);
        printf("tconnecter : erreur %d\n", err);
        printf("tconnecter : apres le signal\n");
        rt_mutex_acquire(&mutexInitConnexion, TM_INFINITE);
        initConnexion = 1;
        rt_mutex_release(&mutexInitConnexion);
    }
}



/*Le thread communiquer permet l'intéraction entre le moniteur et le shuttle
 il va analyser les messages reçus du moniteur et agir en conséquence*/
void communiquer(void *arg) {
    DMessage *msg = d_new_message();
    int num_msg = 0;
    while (1) {
        printf("tserver : Début de l'exécution de serveur\n");
        serveur->open(serveur, "8000");
        printf("tserver : Connexion\n");

        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        etatCommMoniteur = STATUS_OK;
        rt_mutex_release(&mutexEtat);
        int var1 = 1;
        while (var1 > 0) {
            printf("tserver : Attente d'un message\n");
            var1 = serveur->receive(serveur, msg);
            num_msg++;
            if (var1 > 0) {
                switch (msg->get_type(msg)) {
                    case MESSAGE_TYPE_ACTION:
                        printf("tserver : Le message %d reçu est une action\n",
                                num_msg);
                        DAction *action = d_new_action();
                        action->from_message(action, msg);
                        switch (action->get_order(action)) {
                            case ACTION_CONNECT_ROBOT:
                                printf("tserver : Action connecter robot\n");
                                rt_sem_v(&semConnecterRobot);
                                break;
                            case ACTION_FIND_ARENA:
                                printf("tserver : Find Arena\n");
                                rt_sem_v(&semArene);
                                break;
                            case ACTION_ARENA_IS_FOUND:
                                printf("tserver : arene trouvée\n");
                                break;
                            case ACTION_ARENA_FAILED:
                                printf("tserver : On deconnecte l'arene !\n");
                                rt_mutex_acquire(&mutexAreneCam, TM_INFINITE);
                                arene = NULL;
                                rt_mutex_release(&mutexAreneCam);
                                break;
                            case ACTION_COMPUTE_CONTINUOUSLY_POSITION:
                                rt_mutex_acquire(&mutexPosition, TM_INFINITE);
                                continuCalcul = 1;
                                rt_mutex_release(&mutexPosition);
                                break;
                            case ACTION_STOP_COMPUTE_POSITION:
                                rt_mutex_acquire(&mutexPosition, TM_INFINITE);
                                continuCalcul = 0;
                                rt_mutex_release(&mutexPosition);
                                break;
                        }
                        break;
                    case MESSAGE_TYPE_MOVEMENT:
                        printf("tserver : Le message reçu %d est un mouvement\n",
                                num_msg);
                        rt_mutex_acquire(&mutexMove, TM_INFINITE);
                        move->from_message(move, msg);
                        move->print(move);
                        rt_mutex_release(&mutexMove);
                        break;
                }
            }
        }
    }
}
/*Ce thread envoie les instructions de deplacement reçu du moniteur par le shuttle directement vers le robot*/
void deplacer(void *arg) {
    int status = 1;
    int gauche;
    int droite;
    long useless;
    DMessage *message;

    printf("tmove : Debut de l'éxecution de periodique à 200ms\n");
    rt_task_set_periodic(NULL, TM_NOW, 200000000);

    while (1) {
        /* Attente de l'activation périodique */
        rt_task_wait_period(NULL);
        printf("tmove : Activation périodique\n");

        rt_event_wait(&evCoPerdue, 1, &useless, EV_ALL, TM_INFINITE);
        //printf("REGARDER ICI : erreur déplacer\n");
        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        status = etatCommRobot;
        rt_mutex_release(&mutexEtat);

        if (status == STATUS_OK) {
            rt_mutex_acquire(&mutexMove, TM_INFINITE);
            switch (move->get_direction(move)) {
                case DIRECTION_FORWARD:
                    gauche = MOTEUR_ARRIERE_LENT;
                    droite = MOTEUR_ARRIERE_LENT;
                    break;
                case DIRECTION_LEFT:
                    gauche = MOTEUR_ARRIERE_LENT;
                    droite = MOTEUR_AVANT_LENT;
                    break;
                case DIRECTION_RIGHT:
                    gauche = MOTEUR_AVANT_LENT;
                    droite = MOTEUR_ARRIERE_LENT;
                    break;
                case DIRECTION_STOP:
                    gauche = MOTEUR_STOP;
                    droite = MOTEUR_STOP;
                    break;
                case DIRECTION_STRAIGHT:
                    gauche = MOTEUR_AVANT_LENT;
                    droite = MOTEUR_AVANT_LENT;
                    break;
            }
            rt_mutex_release(&mutexMove);
            status = robot->set_motors(robot, gauche, droite); 
            rt_mutex_acquire(&mutexCompteurRecep, TM_INFINITE);
            if (status != STATUS_OK) {
                compteur_erreur_recep++;
            } else {
                compteur_erreur_recep = 0;
                printf("REGARDER ICIBIS : nb erreur est %d\n", compteur_erreur_recep);
            }

            rt_mutex_release(&mutexCompteurRecep);
        }
    }
}

/*Le thread connexion_perdue permet de determiner l'état de la connexion. Si elle est perdue 
 le thread va changer le masque de l'event et ainsi stopper les threads du robot le temps que la reconnexion soit relancée*/
void connexion_perdue(void *arg) {
    DMessage *message;

    //printf("tmove : Debut de l'éxecution de periodique à 1s\n");
    rt_task_set_periodic(NULL, TM_NOW, 200000000);

    while (1) {
        /* Attente de l'activation périodique */
        rt_task_wait_period(NULL);
        rt_mutex_acquire(&mutexCompteurRecep, TM_INFINITE);
        printf("REGARDER ICI : nb erreur est %d\n", compteur_erreur_recep);
        if (compteur_erreur_recep > 6) { //valeur dépendant du robot, peut etre plus  

            rt_mutex_acquire(&mutexInitConnexion, TM_INFINITE);
            initConnexion = 0;
            rt_mutex_release(&mutexInitConnexion);
            rt_event_clear(&evCoPerdue, 1, NULL);
            message = d_new_message();
            rt_mutex_acquire(&mutexEtat, TM_INFINITE);
            etatCommRobot = 1;
            message->put_state(message, etatCommRobot);
            rt_mutex_release(&mutexEtat);
            compteur_erreur_recep = 0;

            if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                message->free(message);
            }
        }
        rt_mutex_release(&mutexCompteurRecep);
    }
}

int write_in_queue(RT_QUEUE *msgQueue, void * data, int size) {
    void *msg;
    int err;

    msg = rt_queue_alloc(msgQueue, size);
    memcpy(msg, &data, size);

    if ((err = rt_queue_send(msgQueue, msg, sizeof (DMessage), Q_NORMAL)) < 0) {
        printf("Error msg queue send: %s\n", strerror(-err));
    }
    rt_queue_free(&queueMsgGUI, msg);

    return err;
}


/*Le thread Battery permet de renvoyer le statut de la batterie du robot. Sa période est de 250ms*/
void battery(void * arg) {
    int status;
    long useless;
    int niveau_batterie;
    DMessage *message;
    DBattery *battery;
    battery = d_new_battery();
    printf("tbattery : Debut de l'éxecution de periodique à 250ms\n");
    rt_task_set_periodic(NULL, TM_NOW, 250000000);
    while (1) {
        /* Attente de l'activation périodique */
        rt_task_wait_period(NULL);
        printf("tbattery : Activation périodique\n");
        /*Event pour savoir si on est bien co*/
        rt_event_wait(&evCoPerdue, 1, &useless, EV_ALL, TM_INFINITE);
        /*On vérifie le status de la Co Robot*/
        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        status = etatCommRobot;
        rt_mutex_release(&mutexEtat);
        rt_mutex_acquire(&mutexCompteurRecep, TM_INFINITE);
        if (status == STATUS_OK) {
            status = robot->get_vbat(robot, &niveau_batterie);
            d_battery_set_level(battery, niveau_batterie);
            if (status != STATUS_OK) {
                compteur_erreur_recep++;
            } else {
                compteur_erreur_recep = 0;
                message = d_new_message();
                message->put_battery_level(message, battery);
                printf("tbattery : Envoi message\n");
                if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                    message->free(message);
                }

            }
        } else {
            compteur_erreur_recep++;
        }
        rt_mutex_release(&mutexCompteurRecep);
    }
}


void watchdog(void *arg) {
    int comRobot;
    int comMoniteur;
    int status;
    long useless;
    while (1) {
        rt_event_wait(&evCoPerdue, 1, &useless, EV_ALL, TM_INFINITE);
        //On vérifie si la connexion vient d'être faite pour ne pas redefinir tout le temps la périodicité
        rt_mutex_acquire(&mutexInitConnexion, TM_INFINITE);
        if (initConnexion == 1) {
            printf("twatchdog : Debut de l'éxecution de periodique à 1s\n");
            rt_task_set_periodic(NULL, TM_NOW, 1000000000);
            initConnexion = 0;
        }
        rt_mutex_release(&mutexInitConnexion);
        /* Attente de l'activation périodique */
        rt_task_wait_period(NULL);
        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        comRobot = etatCommRobot;
        comMoniteur = etatCommMoniteur;
        rt_mutex_release(&mutexEtat);
        if ((comRobot + comMoniteur) == 0) {
            status = robot->reload_wdt(robot); 
            rt_mutex_acquire(&mutexCompteurRecep, TM_INFINITE);
            if (status != STATUS_OK) {
                compteur_erreur_recep++;
            } else {
                compteur_erreur_recep = 0;
            }
            rt_mutex_release(&mutexCompteurRecep);
        }
    }
}

/*Le thread arene permet de définir la position de l'arene et de la dessiner*/
void th_arene(void *arg) {
    DMessage *message;
    DJpegimage *jpegImage;
    int comMoniteur;
    long useless;
    DImage *image = d_new_image();

    printf("tarene : avant le while 1= NULL\n");
    rt_sem_p(&semArene, TM_INFINITE);
    while (1) {
        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        comMoniteur = etatCommMoniteur;
        rt_mutex_release(&mutexEtat);
        if (comMoniteur == STATUS_OK) {
            rt_mutex_acquire(&mutexAreneCam, TM_INFINITE);
            d_camera_get_frame(camera, image);
            arene = image->compute_arena_position(image);
            printf("tarene : avant le if ARENE = NULL\n");
            if (arene != NULL) {
                printf("tarene : arene est pas null\n");
                d_imageshop_draw_arena(image, arene);
            }
            message = d_new_message();
            message->put_position(message, arene);
            rt_printf("tphoto : Envoi Arene\n");
            if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                message->free(message);
            }
            
            jpegImage = d_new_jpegimage();
            jpegImage->compress(jpegImage, image);
            message = d_new_message();
            message->put_jpeg_image(message, jpegImage);
            rt_printf("tphoto : Envoi jpeg Arene\n");
            if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                message->free(message);
            }
            rt_mutex_release(&mutexAreneCam);
        }

    }
}

/*On calcule la position du robot toutes les 600ms a partir de l'image récupérer par la camera on l'affiche ensuite sur l'image du moniteur 
 * et les coordonnées sont mises a jours en bas du moniteur
 */
void photo(void *arg) {
    DImage* image;
    DJpegimage *jpegImage;
    camera->open(camera);
    DMessage *message;
    int comMoniteur;
    printf("tphoto : Debut de l'éxecution de periodique à 600ms\n");
    rt_task_set_periodic(NULL, TM_NOW, 600000000);

    while (1) {
        rt_task_wait_period(NULL);
        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        comMoniteur = etatCommMoniteur;
        rt_mutex_release(&mutexEtat);

        if (comMoniteur == STATUS_OK) {
            rt_mutex_acquire(&mutexAreneCam, TM_INFINITE);
            image = d_new_image();
            camera->get_frame(camera, image);
            if (continuCalcul == 1) {
                rt_mutex_acquire(&mutexPosition, TM_INFINITE);
                position = image->compute_robot_position(image, NULL);
                if (position != NULL) {
                    d_imageshop_draw_position(image, position);
                }
                message = d_new_message();
                message->put_position(message, position);
                rt_mutex_release(&mutexPosition);
                rt_printf("tphoto : Envoi Position\n");
                if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                    message->free(message);
                }
            }
            jpegImage = d_new_jpegimage();
            jpegImage->compress(jpegImage, image);
            message = d_new_message();
            message->put_jpeg_image(message, jpegImage);
            rt_printf("tphoto : Envoi jpeg\n");
            if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                message->free(message);
            }
            rt_mutex_release(&mutexAreneCam);
        }
    }


}