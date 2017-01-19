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
RT_TASK tcoperdue;
RT_TASK twatchdog;
RT_TASK tarena;
RT_TASK tphoto;

RT_MUTEX mutexEtat;
RT_MUTEX mutexCompteurRecep;
RT_MUTEX mutexMove;
RT_MUTEX mutexInitConnexion;
RT_MUTEX mutexAreneCam;
RT_MUTEX mutexPosition;
//RT_MUTEX mutexComRobot;

RT_EVENT evCoPerdue;

RT_SEM semConnecterRobot;
RT_SEM semArene;
RT_SEM semPhoto;

RT_QUEUE queueMsgGUI;
int compteur_erreur_recep = 0;
int etatCommMoniteur = 1;
int etatCommRobot = 1;
int initConnexion = 0;
int continuCalcul;
DPosition *position;
DRobot *robot;
DMovement *move;
DServer *serveur;
DArena *arene;
DCamera *camera;

int MSG_QUEUE_SIZE = 10;

int PRIORITY_TSERVEUR = 30;
int PRIORITY_TCONNECT = 20;
int PRIORITY_TMOVE = 10;
int PRIORITY_TENVOYER = 25;
int PRIORITY_TBATTERY = 5;
int PRIORITY_TWATCHDOG = 27;
int PRIORITY_TCOPERDUE = 23;
int PRIORITY_TARENE= 15;
int PRIORITY_TPHOTO = 14;

