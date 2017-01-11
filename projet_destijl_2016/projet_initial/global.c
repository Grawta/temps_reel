/*
 * File:   global.h
 * Author: pehladik
 *
 * Created on 21 avril 2011, 12:14
 */

#include "global.h"

RT_TASK tServeur;
RT_TASK tconnect;
RT_TASK tmove;
RT_TASK tenvoyer;
RT_TASK tbattery;

RT_MUTEX mutexEtat;
RT_MUTEX mutexCompteurRecep;
RT_MUTEX mutexMove;
RT_MUTEX mutexInitConnexion;
RT_EVENT evCoPerdue;
RT_SEM semConnecterRobot;
RT_SEM semCoPerdue;

RT_QUEUE queueMsgGUI;
int compteur_erreur_recep = 0;
int etatCommMoniteur = 1;
int etatCommRobot = 1;
int initConnexion = 0;
DRobot *robot;
DMovement *move;
DServer *serveur;


int MSG_QUEUE_SIZE = 10;

int PRIORITY_TSERVEUR = 30;
int PRIORITY_TCONNECT = 20;
int PRIORITY_TMOVE = 10;
int PRIORITY_TENVOYER = 25;
int PRIORITY_TBATTERY = 5;
/*mettre une haute prio au watchdog ex=27 prio pour connexion perdu = 23 */
