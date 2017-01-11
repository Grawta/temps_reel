/* 
 * File:   global.h
 * Author: pehladik
 *
 * Created on 12 janvier 2012, 10:11
 */

#ifndef GLOBAL_H
#define	GLOBAL_H

#include "includes.h"

/* @descripteurs des tâches */
extern RT_TASK tServeur;
extern RT_TASK tconnect;
extern RT_TASK tmove;
extern RT_TASK tenvoyer;
extern RT_TASK tbattery;
extern RT_TASK tcoperdue;
extern RT_TASK twatchdog;
extern RT_TASK tarena;

/* @descripteurs des mutex */
extern RT_MUTEX mutexEtat;
extern RT_MUTEX mutexMove;
extern RT_MUTEX mutexCompteurRecep;
extern RT_MUTEX mutexInitConnexion;
extern RT_MUTEX mutexAreneCam;
extern RT_MUTEX mutexPosition;
//extern RT_MUTEX mutexComRobot;
/* @descripteurs des sempahore */
extern RT_SEM semConnecterRobot;
extern RT_SEM semArene;
extern RT_SEM semPhoto;
/*descripteur evenement*/
extern RT_EVENT evCoPerdue;
/* @descripteurs des files de messages */
extern RT_QUEUE queueMsgGUI;

/* @variables partagées */
extern int etatCommMoniteur;
extern int compteur_erreur_recep;
extern int etatCommRobot;
extern int initConnexion;
extern int continuCalcul;
extern DPosition *position;
extern DServer *serveur;
extern DRobot *robot;
extern DMovement *move;
extern DArena *arene;
extern DCamera *camera;
/* @constantes */
extern int MSG_QUEUE_SIZE;
extern int PRIORITY_TSERVEUR;
extern int PRIORITY_TCONNECT;
extern int PRIORITY_TMOVE;
extern int PRIORITY_TENVOYER;
extern int PRIORITY_TBATTERY;
extern int PRIORITY_TWATCHDOG;
extern int PRIORITY_TCOPERDUE;
extern int PRIORITY_TARENE;

#endif	/* GLOBAL_H */

