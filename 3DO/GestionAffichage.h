// GestionAffichage

#ifndef H_AFFICHAGE
#define H_AFFICHAGE

#define	Ecrans 2
//#define	DossierFondsPAL1 "PAL/Graphics/" // 320x288 pixels - 50 images/sec
//#define	DossierFondsPAL2 "PAL/Graphics/" // 384x288 pixels - 50 images/sec.
//#define	DossierFondsNTSC "NTSC/Graphics/" // 320x240 pixels - 60 images/sec.
#define	DossierFondsPAL1 "" // 320x288 pixels - 50 images/sec
#define	DossierFondsPAL2 "" // 384x288 pixels - 50 images/sec.
#define	DossierFondsNTSC "" // 320x240 pixels - 60 images/sec.

/* Inclusions des bibliotheques C */
// Necessaire pour la manipulation des chaines de caracteres
#include <string.h>
// Necessaire pour les communications en entrees et sorties
#include <stdio.h>
/* ------------------------------ */


// Declare les types de variables reconnues par la 3DO
#include <types.h>
// Necessaire pour la gestion de l'affichage
#include <graphics.h>
// Necessaire pour controler le format de restitution d'image (NTSC, PAL1 ou PAL2)
#include <getvideoinfo.h>
// Necessaire pour la gestion des écrans, des fonds et des effets de fondu
#include <displayutils.h>
// Necessaire pour la gestion de memoire
#include <mem.h>
#include <umemory.h>
// Necessaire pour la gestion des listes
#include <list.h>
// Necessaire pour la capture des evenements et les ecouteurs
#include <event.h>
/* ------------------------------ */

void drawNESscreenCEL(void);

/* Initialise l'affichage */
bool affichageInitialisation(void);
/* Met a jour l'affichage */
void affichageMiseAJour(void);


#endif
