// GestionAffichage
#include "GestionAffichage.h"
#include "../lamenes.h"

// Format de l'ecran par defaut (NTSC)
int32 ecranFormat=0;
int32 *pointeurEcranFormat=&ecranFormat;
// Taille de l'image par defaut (NTSC)
int32 ecranLargeur=320;
int32 *pointeurEcranLargeur=&ecranLargeur;
int32 ecranHauteur=240;
int32 *pointeurEcranHauteur=&ecranHauteur;
// Images par secondes par defaut (NTSC)
int32 ecranFrequence=60;
int32 *pointeurEcranFrequence=&ecranFrequence;
// Requete d'entree/sortie sur la memoire video (VRAM)
Item requeteVRAMIO=-1;
// Ecran
ScreenContext *pointeurEcranContexte=NULL;
// Affichage
Item affichage=-1;
// Ecran courant
int32 ecranCourant=0;
// Image de fond


/* Initialise l'affichage */
bool affichageInitialisation(){
	// Retour
	int32 retourLocal=TRUE;	
	// Standard d'affichae utilise
	int32 affichageStandard=0;	
	// Demarre la gestion video de la 3DO
	retourLocal=OpenGraphicsFolio();
	// Si le demarrage a echoue
	if(retourLocal<0){
		// Retourne une erreur
		return FALSE;
	}
	// Cree une requete d'entree/sortie sur la memoire video (VRAM)
	requeteVRAMIO=CreateVRAMIOReq();
	// Si la creation de la requete a echoue
	if(requeteVRAMIO<0){
		// Retourne une erreur
		return FALSE;
	}
	// Alloue de la memoire pour l'affichage d'un ecran
	pointeurEcranContexte=(ScreenContext *)AllocMem(sizeof(ScreenContext), MEMTYPE_ANY);
	//Si l'allocation de memoire n'a pas eu lieu
	if(pointeurEcranContexte==NULL){
		// Retourne une erreur
		return FALSE;
	}
	// Recupere le format de l'ecran			
	affichageStandard=GetDisplayType();
	// Si la recuperation a echoue
	if(affichageStandard<0){
		// Retourne une erreur
		return FALSE;
	}
	// Si l'affichage est en PAL1
	if(affichageStandard==DI_TYPE_PAL1){
		// Format
		*pointeurEcranFormat=1;
		// Taille de l'image
		*pointeurEcranLargeur=320;
		*pointeurEcranHauteur=288;
		// Images par secondes
		*pointeurEcranFrequence=50;	
	// Si l'affichage est en PAL2
	}else if(affichageStandard==DI_TYPE_PAL2){
		// Format
		*pointeurEcranFormat=2;
		// Taille de l'image
		*pointeurEcranLargeur=384;
		*pointeurEcranHauteur=288;
		// Images par secondes
		*pointeurEcranFrequence=50;	
	// L'affichage est en NTSC
	}else{
		// Format
		*pointeurEcranFormat=0;
		// Taille de l'image
		*pointeurEcranLargeur=320;
		*pointeurEcranHauteur=240;
		// Images par secondes
		*pointeurEcranFrequence=60;
	}	
	// Cree l'affichage sur deux ecrans (ou deux zones tampon)
	// Ainsi, pendant que le premier ecran est affiche, le second est genere
	// A chaque fois que la fonction affichageRendu est appelee, l'affichage passe d'un ecran a l'autre
	affichage=CreateBasicDisplay(pointeurEcranContexte, affichageStandard, Ecrans);
	// Si la creation de l'affichage a echoue
	if(affichage<0){
		// Retourne une erreur
		return FALSE;
	}

	// Retourne un succes	
	return TRUE;
}

void drawNESscreenCEL()
{
	DrawCels(pointeurEcranContexte->sc_BitmapItems[ecranCourant], screenCel);
}

/* Met a jour l'affichage */
void affichageMiseAJour(void){
	// Affiche l'ecran courant
	// Le second parametre est utilise pour un affichage stereoscopique ou entrelace
	// Dans le cas contraire, le premier et le second parametre doivent etre identiques ou le second a 0
	DisplayScreen(pointeurEcranContexte->sc_Screens[ecranCourant], 0);
	// Passe a l'écran suivant
	ecranCourant++;
	// Si l'écran courant est superieur ou egal au nombre d'écrans utilises
	if(ecranCourant>=Ecrans){
		// Passe au premier ecran
		ecranCourant=0;
	}
}
