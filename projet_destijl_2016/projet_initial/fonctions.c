#include "fonctions.h"

int write_in_queue(RT_QUEUE *msgQueue, void * data, int size);

void envoyer(void * arg) {
    DMessage *msg;
    int err;

    while (1) {
        rt_printf("tenvoyer : Attente d'un message\n");
        if ((err = rt_queue_read(&queueMsgGUI, &msg, sizeof (DMessage), TM_INFINITE)) >= 0) {
            rt_printf("tenvoyer : envoi d'un message au moniteur\n");
            serveur->send(serveur, msg);
            msg->free(msg);
        } else {
            rt_printf("Error msg queue write: %s\n", strerror(-err));
        }
    }
}
//Le connecter ne réussit pas à tous les coups on a de la chance avec le robot 07

void connecter(void * arg) {
    int status;
    DMessage *message;
    rt_printf("tconnect : Debut de l'exécution de tconnect\n");

    while (1) {
        rt_printf("tconnect : Attente du sémarphore semConnecterRobot\n");
        rt_sem_p(&semConnecterRobot, TM_INFINITE);
        rt_printf("tconnect : Ouverture de la communication avec le robot\n");
        status = robot->open_device(robot);

        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        etatCommRobot = status;
        rt_mutex_release(&mutexEtat);

        if (status == STATUS_OK) {
            status = robot->start_insecurely(robot);
            if (status == STATUS_OK) {
                rt_printf("tconnect : Robot démarrer\n");
            }
        }

        message = d_new_message();
        message->put_state(message, status);

        rt_printf("tconnecter : Envoi message\n");
        message->print(message, 100);

        if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
            message->free(message);
        }
        rt_event_signal(&evCoPerdue,1);
        rt_mutex_acquire(&mutexInitConnexion,TM_INFINITE);
        initConnexion = 1;
        rt_mutex_release(&mutexInitConnexion);
    }
}


//reconnexion au serveur si perte de connexion ( var <1)

void communiquer(void *arg) {
    DMessage *msg = d_new_message();
    int num_msg = 0;
    while (1) {
        rt_printf("tserver : Début de l'exécution de serveur\n");
        serveur->open(serveur, "8000");
        rt_printf("tserver : Connexion\n");

        rt_mutex_acquire(&mutexEtat, TM_INFINITE);
        etatCommMoniteur = 0;
        rt_mutex_release(&mutexEtat);
        int var1 = 1;
        while (var1 > 0) {
            rt_printf("tserver : Attente d'un message\n");
            var1 = serveur->receive(serveur, msg);
            num_msg++;
            if (var1 > 0) {
                switch (msg->get_type(msg)) {
                    case MESSAGE_TYPE_ACTION:
                        rt_printf("tserver : Le message %d reçu est une action\n",
                                num_msg);
                        DAction *action = d_new_action();
                        action->from_message(action, msg);
                        switch (action->get_order(action)) {
                            case ACTION_CONNECT_ROBOT:
                                rt_printf("tserver : Action connecter robot\n");
                                rt_sem_v(&semConnecterRobot);
                                break;
                        }
                        break;
                    case MESSAGE_TYPE_MOVEMENT:
                        rt_printf("tserver : Le message reçu %d est un mouvement\n",
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

void deplacer(void *arg) {
    int status = 1;
    int gauche;
    int droite;
    DMessage *message;

    rt_printf("tmove : Debut de l'éxecution de periodique à 1s\n");
    rt_task_set_periodic(NULL, TM_NOW, 1000000000);

    while (1) {
        /* Attente de l'activation périodique */
        rt_task_wait_period(NULL);
        rt_printf("tmove : Activation périodique\n");

        rt_event_wait(&evCoPerdue, 1, NULL, EV_ALL, TM_INFINITE);
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

            // peut etre faire else compteur++ ;
            rt_mutex_acquire(&mutexCompteurRecep, TM_INFINITE);

            if (status != STATUS_OK) {
                //On test la reception
                compteur_erreur_recep++;
            } else {
                compteur_erreur_recep = 0;
            }

            rt_mutex_release(&mutexCompteurRecep);
        }
    }
}

void connexion_perdue(void *arg) {
    DMessage *message;

    //rt_printf("tmove : Debut de l'éxecution de periodique à 1s\n");
    rt_task_set_periodic(NULL, TM_NOW, 1000000);

    while (1) {
        /* Attente de l'activation périodique */
        rt_task_wait_period(NULL);
        //rt_printf("tmove : Activation périodique\n");

        rt_mutex_acquire(&mutexCompteurRecep, TM_INFINITE);

        if (compteur_erreur_recep > 4) { //valeur dépendant du robot, peut etre plus  

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
        rt_printf("Error msg queue send: %s\n", strerror(-err));
    }
    rt_queue_free(&queueMsgGUI, msg);

    return err;
}


void battery(void * arg){
    int status;
    DMessage *message;
    DBattery *battery;
    battery = d_new_battery();
    rt_printf("tbattery : Debut de l'éxecution de periodique à 250ms\n");
    rt_task_set_periodic(NULL, TM_NOW, 250000000);
    while(1){
    /* Attente de l'activation périodique */
    rt_task_wait_period(NULL);
    rt_printf("tbattery : Activation périodique\n");
    /*Event pour savoir si on est bien co*/
    rt_event_wait(&evCoPerdue,1,NULL,EV_ALL,TM_INFINITE);
     
    /*On vérifie le status de la Co Robot*/
    rt_mutex_acquire(&mutexEtat, TM_INFINITE);
    status = etatCommRobot;
    rt_mutex_release(&mutexEtat);
    rt_mutex_acquire(&mutexCompteurRecep, TM_INFINITE);
    if (status == STATUS_OK){
        status = d_battery_get_level(battery);
        if(status != STATUS_OK){         
            compteur_erreur_recep++;         
        }else{
            message = d_new_message();
            message->put_state(message, status);
            rt_printf("tmove : Envoi message\n");
            if (write_in_queue(&queueMsgGUI, message, sizeof (DMessage)) < 0) {
                 message->free(message);
            }
            compteur_erreur_recep = 0;
        }
    }else{        
            compteur_erreur_recep++;
    }
     rt_mutex_release(&mutexCompteurRecep);
    }
}


void watchdog(void *arg){
    int comRobot;
    int comMoniteur;
    int status;
    rt_mutex_acquire(&mutexInitConnexion,TM_INFINITE);
    if(initConnexion==1){
        rt_printf("twatchdog : Debut de l'éxecution de periodique à 1s\n");
        rt_task_set_periodic(NULL,TM_NOW,1000000000);
    }
    rt_event_wait(&evCoPerdue,1,NULL,EV_ALL,TM_INFINITE);
    /* Attente de l'activation périodique */
    rt_task_wait_period(NULL);
    rt_mutex_acquire(&mutexEtat, TM_INFINITE);
    comRobot = etatCommRobot;
    comMoniteur = etatCommMoniteur;
    rt_mutex_release(&mutexEtat);
    if((comRobot+comMoniteur)==0){
        status = robot->reload_wdt(robot);
        rt_mutex_acquire(&mutexCompteurRecep, TM_INFINITE);
        if (status != STATUS_OK) {
            compteur_erreur_recep++;
        }else{
            compteur_erreur_recep =0;
        }
        rt_mutex_release(&mutexCompteurRecep);
    }
}