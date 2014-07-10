/******************************************************************************/
/* SMILE v1.47 - Extraction of structured motifs common to several sequences  */
/* Copyright (C) 2004 L.Marsan (lama -AT- prism.uvsq.fr)                      */
/*                                                                            */
/* This program is free software; you can redistribute it and/or              */
/* modify it under the terms of the GNU General Public License                */
/* as published by the Free Software Foundation; either version 2             */
/* of the License, or (at your option) any later version.                     */
/*                                                                            */
/* This program is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/* GNU General Public License for more details.                               */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program; if not, write to the Free Software                */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */
/******************************************************************************/

#include <spell.h>


/******************************************************************************/
/*                          PROTOTYPES PRIVES                                 */
/******************************************************************************/
/* Gestion des modeles acceptes                                               */
void    keepModel(P_mod , P_PileOcc, NbSeq, NbSeq, LongSeq, 
            LongSeq *longbloc, P_Criteres cr);

#if DEBUG_TREE
/* compte des feuilles suivant qu'on est sur un noeud ou non */
int     compteFeuilles(P_occ);
#endif

/* essaie d'avancer d'une lettre dans un arc, et renvoie le noeud image */
Flag avanceBranche(P_occ, P_occ, int, int, Flag, P_Criteres, LongSeq, Flag);

/* Lancement du saut */
NbSeq gestionSaut(P_mod model, P_PileOcc pocc, P_PileOcc poccnew, P_Criteres,
        LongSeq curbloc);

/* explore les modeles */
Flag  spellModels ( P_PileOcc   pocc,
                    P_PileOcc   poccnew,        P_PileOcc   poccsaut,
                    LongSeq     longmod,        LongSeq     longcurbloc,
                    LongSeq     curbloc,
                    Flag        multiblocs,
                    P_mod       model,          P_occ       next,
                    Bit_Tab     **colors_model, NbSeq       nbseq,
                    NbSeq       tmp_quorum,     P_Criteres  cr,
                    LongSeq     *longbloc,      LongSeq     *posdebbloc);

/* Calcule le BT union de tous les BT occurrences                             */
NbSeq   sommeBTOcc(P_PileOcc, Bit_Tab **);

/* Compute CPU time                                                           */
static  float PrintCpuTime(char);



/******************************************************************************/
/* VARIABLES GLOBALES                                                         */
/******************************************************************************/
int     nbmod = 0;
LongSeq maxlongmod=0, *maxlongbloc=NULL;
signed char    ** text=NULL;
FILE    * f=NULL;

/* EXTERNES from alphabet.c                                                   */
extern  int     nbSymbMod;
extern  int     nbSymbSeq;
extern  char    *nummod2str[127];
extern  int     carseq2num[127];
extern  int     comp[127];
extern  Flag    TabSymb[127][127];
extern  int     numJOKER;
extern  int     numSAUT;


/******************************************************************************/
/******************************************************************************/
/************************ FONCTIONS DE BASE ***********************************/
/******************************************************************************/
/******************************************************************************/

#if DEBUG_TREE
/******************************************************************************/
/* compteFeuilles                                                             */
/******************************************************************************/
/* Compte des feuilles suivant qu'on est sur un noeud ou une branche          */
/* Comme le champ nb_feuille contient en fait nb feuille+position, s'il est   */
/* negatif, c'est une feuille (nb_feuille=1) et sa valeur absolue est la pos. */
/* Sinon, ce n'est pas une feuille, en on a pas la pos (inutile)              */
/******************************************************************************/
int compteFeuilles(P_occ p)
{
int val;

if (p->lon == 0)
    val = p->x->nb_feuille;
else
    val = (p->x->Trans)[p->num]->nb_feuille;

if (val <= 0)
    return(1);
return(val);
}
#endif



/******************************************************************************/
/******************************************************************************/
/*********************** GESTION DES LISTES D'OCCURRENCES *********************/
/*********************************ET DES MODELES*******************************/
/******************************************************************************/

/******************************************************************************/
/* KeepModel                                                                  */
/******************************************************************************/
/* Affiche (ou stocke si necessaire) les modeles trouves                      */
/******************************************************************************/
void keepModel(P_mod model, P_PileOcc pocc, NbSeq nbseq, NbSeq quorum,
        LongSeq l, LongSeq *longbloc, P_Criteres cr)
{
int i,j;
LongSeq *lb, *mb;

nbmod++;
#if DEBUG_BASE
        printf("MODELE %s valide!\n",model->name);
#endif

j   = model->lon;
for(i=0; i!=j; i++)
    fprintf(f,"%s", nummod2str[model->name[i]]);
fprintf(f," ");
for(i=0; i!=j; i++)
    {
    if(model->name[i]==numJOKER)
        fprintf(f,"%c",JOKERinterne);
    else if(model->name[i]==numSAUT)
        fprintf(f,"%c",SAUTinterne);
    else
        fprintf(f,"%c", model->name[i]+SHIFTALPHA);
    }
fprintf(f," %d", quorum);

if(l > maxlongmod)
    maxlongmod  = l;

if(cr->bloc != 1)
    for(i=0, lb=longbloc, mb=maxlongbloc; i!=cr->bloc; i++, lb++, mb++)
        if(*lb > *mb)
            *mb  = *lb;

#if OCC
    #if AFF_OCC
        fprintf(f,"\n");
    #else
        fprintf(f,"\t");
    #endif
    afficheLastOcc(f, pocc, l, cr);
#else
    fprintf(f,"\n");
#endif
}


/******************************************************************************/
/******************************************************************************/
/************************* RECHERCHE DES MODELES ******************************/
/******************************************************************************/
/******************************************************************************/


/******************************************************************************/
/* avanceBranche                                                              */
/******************************************************************************/
/* Essaie d'avancer d'une lettre dans un arc.                                 */
/* Renvoie 1 si reussi, 0 sinon.                                              */
/* La variable 'flag' indique si on est sur un noeud(1) ou une branche(0)     */
/******************************************************************************/
Flag avanceBranche( P_occ   next,       P_occ       tmp,        int symbol,
                    int     trans,      Flag        flag_noeud,
                    P_Criteres  cr,     LongSeq     curbloc,
                    Flag    multiblocs)
{
/* Dans cette fonction, le code est duplique dans un souci de rapidite:       */
/* j'essaie de faire un max de tests eliminatoires avant affectations         */

/* Si la branche courante n'est pas epuisee... */
if (flag_noeud == FAUX)
    {
    if ( equiv(symbol, trans) )
        {
        next->xerr    = tmp->xerr;
        next->blocerr = tmp->blocerr;
        }
    else
        {
        next->xerr = tmp->xerr+1;

        if (next->xerr == cr->maxerr+1) /* si maxerr global atteint */
            return 0;

        if(multiblocs == VRAI)
            {
            next->blocerr = tmp->blocerr+1; /* si maxerr local atteint */
            if (next->blocerr == cr->maxerrblocs[curbloc]+1)
                return 0;
            }
        }

    next->x   = tmp->x;
    next->num = tmp->num;
    next->lon = tmp->lon+1;
    }
else
/* Si la branche courante est epuisee, on est sur une nouvelle branche */
    {
    next->x = tmp->x->fils[tmp->num];

    if (next->x->fils[trans] == NULL)
        return(0);

    if ( equiv(symbol, trans) )
        {
        next->xerr    = tmp->xerr;
        next->blocerr = tmp->blocerr;
        }
    else
        {
        next->xerr = tmp->xerr+1;

        if (next->xerr == cr->maxerr+1) /* si maxerr global atteint */
            return 0;
    
        if(multiblocs == VRAI)
            {
            next->blocerr = tmp->blocerr+1; /* si maxerr local atteint */
            if (next->blocerr == cr->maxerrblocs[curbloc]+1)
                return 0;
            }
        }

    next->num = trans;
    next->lon = 1;
    }
  

if(multiblocs == VRAI)
    {
    next->saut= tmp->saut;
    next->codesaut= tmp->codesaut;
    }
  
return(1);
}


/******************************************************************************/
/* sommeBTOcc                                                                 */
/******************************************************************************/
/* Fait l'union des sequences d'une liste d'occurrence et renvoie le nombre   */
/* de ces sequences.                                                          */
/******************************************************************************/
NbSeq sommeBTOcc(P_PileOcc p, Bit_Tab ** bt)
{
  LongSeq pos, precdummy;
  P_occ   po;

  ReinitBitTab(bt);
  pos = p->pos-1;
  if (pos < 0)
    fatalError("spell.c: sommeBTOcc: wrong position in stack!\n");

  precdummy=getPrecDummy(p);
  po  = p->occ+pos;

  while ((pos != precdummy) && (po->x != NULL))
    {
#if DEBUG_BT
    printf("Fusion avec :    ");
#endif
      
    if (po->x->fils[po->num]->debut & LEAF_BIT)
        {
        fusionneBitTab(bt,((Feuille *)po->x->fils[po->num])->sequences);
#if DEBUG_BT
        printBitTab(((Feuille *)po->x->fils[po->num])->sequences);
#endif
        }
    else
        {
        fusionneBitTab(bt,po->x->fils[po->num]->sequences);
#if DEBUG_BT
        printBitTab(po->x->fils[po->num]->sequences);
#endif
        }
    po--;
    pos--;
    }

#if DEBUG_BT
printf("Somme BT : \n") ;
printBitTab(*bt);
printf(" -> %d values\n", nbSequenceInBitTab(*bt));
#endif

return nbSequenceInBitTab(*bt);
}


/******************************************************************************/
/* sauteSymbole                                                               */
/******************************************************************************/
int sauteSymbole(Occ curocc, P_mod model, P_PileOcc pocc, P_Criteres cr,
	LongSeq curbloc, LongSeq longsaut)
{
LongSeq	lmaxbr;
Noeud	*tmpnoeud;
Occ		tmpocc;
int		res = 0,
        trans;
char    carseq;


tmpnoeud = curocc.x->fils[curocc.num];

if (tmpnoeud->debut & LEAF_BIT)
   lmaxbr  = getValue(Liste_positions_fin,((Feuille *)tmpnoeud)->fin_deb)
       - (((Feuille *)tmpnoeud)->debut & LEAF_BIT_INV); 
else
   lmaxbr = tmpnoeud->fin - tmpnoeud->debut;

#if DEBUG_SAUT
printf("SauteSymbole: j'ai gere le saut pour %d, noeud %d, etat: %d/%d branche %d\n",longsaut,curocc.x,curocc.lon,lmaxbr,curocc.num);
#endif

ajouteOcc2Pile(pocc, curocc.x, curocc.num, curocc.lon, curocc.xerr, 0, 
        curocc.saut+longsaut, addSaut2Code(curocc.codesaut, longsaut, curbloc,
        cr));
res++;
longsaut++;

if (curocc.lon != lmaxbr)  /* on est au milieu d'une branche */
	{
	curocc.lon++;

	carseq = text[tmpnoeud->sequence_number]
            [(tmpnoeud->debut & LEAF_BIT_INV)+curocc.lon-1];

	if(carseq==FINAL)	/* si on rencontre un $ c'est fini */
		return res;

	if(longsaut<=cr->saut[curbloc].max)
		res += sauteSymbole(curocc, model, pocc, cr, curbloc, longsaut);

	return res;
	}
else	/* sinon on est a un noeud, plusieurs trans sont possibles */
	{
	if(longsaut<=cr->saut[curbloc].max)
		{
		tmpocc.x    = tmpnoeud;
		tmpocc.lon  = 1;
		tmpocc.xerr = curocc.xerr;
        tmpocc.codesaut = curocc.codesaut;
        tmpocc.saut = curocc.saut;

		if ((tmpnoeud->debut & LEAF_BIT) == 0)
		  for (trans = 0; trans != nbSymbSeq; trans++)
			{
			if (tmpnoeud->fils[trans] != NULL)
				{
				tmpocc.num  = trans;

				res += sauteSymbole(tmpocc, model, pocc, cr, curbloc, longsaut);
				}
			}
		}
	return res;
	}
}

/******************************************************************************/
/* sauteBranche                                                               */
/******************************************************************************/
int sauteBranche(Occ curocc, P_mod model, P_PileOcc pocc, P_Criteres cr,
	LongSeq curbloc, LongSeq longsaut)
{
LongSeq	lmaxbr;
Noeud *	tmpnoeud, *newtmpnoeud;
Occ		tmpocc;
int		res = 0, newlongsaut,
        trans;
char    carseq;



tmpnoeud = curocc.x->fils[curocc.num];

if (tmpnoeud->debut & LEAF_BIT)
   lmaxbr  = getValue(Liste_positions_fin,((Feuille *)tmpnoeud)->fin_deb) - (((Feuille *)tmpnoeud)->debut & LEAF_BIT_INV); 
else
   lmaxbr = tmpnoeud->fin - tmpnoeud->debut;

#if DEBUG_SAUT
printf("SauteBranche: j'ai gere le saut pour %d, noeud %d, etat: %d/%d branche %d\n",longsaut,curocc.x,curocc.lon,lmaxbr,curocc.num);
#endif

if (curocc.lon != lmaxbr)  /* on est au milieu d'une branche */
	{
	if ( lmaxbr-curocc.lon <= cr->saut[curbloc].min-longsaut )
		{	
		longsaut+=lmaxbr-curocc.lon;
		curocc.lon=lmaxbr;

#if DEBUG_SAUT
		printf("SauteBranche: milieuBr, fast, je vais au bout %d/%d br %d et lgsaut %d\n",curocc.lon,lmaxbr,curocc.num,longsaut);
#endif

		carseq = text[tmpnoeud->sequence_number]
                [(tmpnoeud->debut & LEAF_BIT_INV)+lmaxbr-1];

		if(carseq != FINAL)	/* si on rencontre un $ c'est fini */
			{
#if DEBUG_SAUT
			printf("SauteBranche: finBr=$, c'est fini\n");
#endif
			res += sauteBranche(curocc, model, pocc, cr, curbloc, longsaut);
			}
		}
	else
		{
		curocc.lon+=cr->saut[curbloc].min-longsaut;
		longsaut=cr->saut[curbloc].min;

#if DEBUG_SAUT
		printf("SauteBranche: milieuBr, minsaut ds Br, je m'arrete a %d/%d num %d et lgsaut %d\n",curocc.lon,lmaxbr,curocc.num,longsaut);
#endif

		res += sauteSymbole(curocc, model, pocc, cr, curbloc, longsaut);
		}
	}
else	/* sinon on est a un noeud, plusieurs trans sont possibles */
	{
	tmpocc.x    = tmpnoeud;
	tmpocc.xerr = curocc.xerr;
    tmpocc.codesaut = curocc.codesaut;
    tmpocc.saut = curocc.saut;
    
	if ((tmpnoeud->debut & LEAF_BIT) == 0)
	  for (trans = 0; trans != nbSymbSeq; trans++)
		{
		tmpocc.num  = trans;
		newlongsaut = longsaut;

		if (tmpnoeud->fils[trans] != NULL)
			{
			newtmpnoeud	= tmpnoeud->fils[trans];

			if (newtmpnoeud->debut & LEAF_BIT)
    		    lmaxbr  = getValue(Liste_positions_fin,
                        ((Feuille *)newtmpnoeud)->fin_deb)
                         - (newtmpnoeud->debut & LEAF_BIT_INV); 
			else
			    lmaxbr = newtmpnoeud->fin - newtmpnoeud->debut;
			
			if ( lmaxbr <= cr->saut[curbloc].min-longsaut )
				{
				newlongsaut+=lmaxbr;
				tmpocc.lon=lmaxbr;

#if DEBUG_SAUT
				printf("SauteBranche: noeud, fast, %d/%d, br %d, lgsaut %d\n",tmpocc.lon,lmaxbr,tmpocc.num,newlongsaut);
#endif

				carseq = text[newtmpnoeud->sequence_number]
                        [(newtmpnoeud->debut & LEAF_BIT_INV)+lmaxbr-1];

				if(carseq!=FINAL)	/* si on rencontre un $ c'est fini */
					{
#if DEBUG_SAUT
					printf("SauteBranche: finBr=$, c'est fini\n");
#endif
					res += sauteBranche(tmpocc, model, pocc, cr, curbloc,
                            newlongsaut);
					}
				}
			else
				{
				tmpocc.lon=cr->saut[curbloc].min-newlongsaut;
				newlongsaut=cr->saut[curbloc].min;

#if DEBUG_SAUT
				printf("SauteBranche2: noeud %d, minsaut ds Br, %d/%d, br %d, lgsaut %d\n",tmpocc.x, tmpocc.lon,lmaxbr,tmpocc.num,newlongsaut);
#endif

				res += sauteSymbole(tmpocc, model, pocc, cr, curbloc,
                        newlongsaut);
				}
			}
		}
	}
return res;
}

/******************************************************************************/
/* gestionSaut                                                                */
/******************************************************************************/
NbSeq gestionSaut(P_mod model, P_PileOcc pocc, P_PileOcc poccnew,
	P_Criteres cr, NbSeq curbloc)
{
LongSeq	pos, precdummy;
P_occ	tmpocc;
int		res = 0;

pos		= poccnew->pos-1;
tmpocc	= poccnew->occ+pos;
precdummy=getPrecDummy(poccnew);

ajouteDummy(pocc);

while ((pos != precdummy) && (tmpocc->x != NULL) )
	{
	if (cr->saut[curbloc].min == 0)
		{
		res+=sauteSymbole(*tmpocc, model, pocc, cr, curbloc, 0);
		}
	else
		{
		res += sauteBranche(*tmpocc, model, pocc, cr, curbloc, 0);
		}
	pos--;
    tmpocc	= poccnew->occ+pos;
	}

if(res==0)
	depileRec(pocc);

return (res);
}

/******************************************************************************/
/* spellModels                                                                */
/******************************************************************************/
/* Explore les modeles recursivement.                                         */
/******************************************************************************/
Flag  spellModels ( P_PileOcc   pocc,
                    P_PileOcc   poccnew,        P_PileOcc   poccsaut,
                    LongSeq     longmod,        LongSeq     longcurbloc,
                    LongSeq     curbloc,        Flag        multiblocs,
                    P_mod       model,          P_occ       next,
                    Bit_Tab     **colors_model, NbSeq       nbseq,
                    NbSeq       tmp_quorum,     P_Criteres  cr,
                    LongSeq     *longbloc,      LongSeq     *posdebbloc)
{
Flag        zarb_back = 0,
            zarb_ext  = 0;
char        carseq;
LongSeq     lmaxbr,
            pos,
            precdummy,
            palbloc;
long int    maxseq;
P_occ       tmpocc;
int         tmpint,
            nbnewmod = 0,
            nbocc,
            symbol,
            trans;
NbSeq       tmp_quorum2;

if(longmod==3)
    barre(0);

/* CONDITION D'EXTENSION                                                      */
if ( ( (cr->longueur.max == 0) || (longmod < cr->longueur.max) )
    &&
     ( (multiblocs == FAUX)
       || ( (cr->longbloc[curbloc].max == 0)
       || (longcurbloc < cr->longbloc[curbloc].max) )
     ) 
    &&
     ( (cr->flag_palindrom == FAUX)
       || cr->palindrom[curbloc] == -1
       || longcurbloc != longbloc[(int)(cr->palindrom[curbloc])]
     )
   )
    {

#if DEBUG_BASE
    printf("J'etends...\n");
#endif

/* Boucle sur les symboles de l'alphabet pourl'extension du modele ************/
    for (symbol = 0; symbol != nbSymbMod; symbol++)
        {
#if DEBUG_BASE
       AfficheModel(model);
       printf("Vers %d\n", symbol);
#endif
/* Pas de JOKER en premiere position                                          */
        if(longmod == 0 && symbol == numJOKER)
            continue;

/* Gestion de la composition des modeles **************************************/
        if (cr->flag_compo == VRAI || cr->flag_compobloc[curbloc] == VRAI)
            {
            if ( (cr->compobloc[curbloc][symbol] == 0)
                || (cr->compo[symbol] == 0) )
                continue;
            }

/* Gestion des palindromes                                                    */
        if (cr->flag_palindrom)
            {
            if (longcurbloc == 0)
                posdebbloc[curbloc] = model->lon;

            if (cr->palindrom[curbloc] != -1)
                {
                palbloc = cr->palindrom[curbloc];
    
                if (symbol!=
                   comp[model->name[posdebbloc[palbloc]+longbloc[palbloc]-1-longcurbloc]])
                    continue;
                }
            }

/* Init variables de pile d'occs                                              */
        pos     = pocc->pos-1;
        tmpocc  = pocc->occ+pos;
        precdummy=getPrecDummy(pocc);
        videPile(poccnew);
        maxseq  = 0;
        nbocc   = 0;
#if DEBUG_BASE
        printf("J'ENTRE (l=%d  symbol=%d model=%s quorum=%d)\n",longmod,symbol,
                model->name,tmp_quorum); 
#endif
/*         fprintf(stderr,"pos %d pd %d\n",pos, precdummy); */
        while ((pos != precdummy) && (tmpocc->x != NULL))
            {
            lmaxbr = ((tmpocc->x->fils)[tmpocc->num]->debut & LEAF_BIT)?
                    getValue(Liste_positions_fin,
                    ((Feuille *)tmpocc->x->fils[tmpocc->num])->fin_deb) 
                    - (tmpocc->x->fils[tmpocc->num]->debut & LEAF_BIT_INV) :
                    tmpocc->x->fils[tmpocc->num]->fin
                    - tmpocc->x->fils[tmpocc->num]->debut;

#if DEBUG_BASE
              if(longmod!=0)
                  {
                  printf("Je traite l'occ:%p  num %d  lon %d  saut %d  codesaut %d (longmod=%d)\n",
                          tmpocc->x,tmpocc->num,tmpocc->lon,tmpocc->saut,
                          tmpocc->codesaut, longmod);
                  afficheOcc(stdout, tmpocc, longmod,0);
                  printf("...et je trouve:\n");
                  }
#endif

/* on est au milieu d'une branche - une transition possible */
            if (tmpocc->lon != lmaxbr)
                {
                carseq = text[tmpocc->x->fils[tmpocc->num]->sequence_number]
                    [ (tmpocc->x->fils[tmpocc->num]->debut & LEAF_BIT_INV)
                    + tmpocc->lon];

                if ( (carseq != FINAL)
                    && (avanceBranche(next, tmpocc, symbol,
                            carseq2num[(int) carseq], 0, cr, curbloc,
                            multiblocs) ) )
                    {
                    ajouteOcc2Pile(poccnew, next->x, next->num, next->lon,
                        next->xerr,next->blocerr, next->saut, next->codesaut);
#if DEBUG_BASE
                    printf("occ:%p  num %d  lon %d  saut %d  codesaut %d (longmod=%d)\n",
                            next->x,next->num,next->lon,next->saut,
                            next->codesaut, longmod);
                    afficheOcc(stdout, next, longmod+1,0);
#endif

                    nbocc++;

                    if (next->x->fils[next->num]->debut & LEAF_BIT)
                        {
                        maxseq += nbSequenceInBitTab(
                            ((Feuille *)next->x->fils[next->num])->sequences);
#if DEBUG_BT
                        printf("nb seq in bt (br): %d \n",
                            nbSequenceInBitTab(((Feuille *)
                            next->x->fils[next->num])->sequences));
#endif
                        }
                    else
                        {
                        maxseq += next->x->fils[next->num]->nb_element_bt;
#if DEBUG_BT
                        printf("nb seq in bt (br): %d \n",
                                nbSequenceInBitTab(
                                next->x->fils[next->num]->sequences));
#endif
                        }
                    }
                }
/* sinon on est a un noeud, plusieurs trans sont eventuellement possibles */
            else
                {
                for (trans = 0; trans != nbSymbSeq; trans++)
                    {
                    tmpocc=pocc->occ+pos;
                    if (avanceBranche(next, tmpocc, symbol, trans, 1, cr,
                                curbloc, multiblocs))
                        {
                        ajouteOcc2Pile(poccnew, next->x, next->num, next->lon,
                            next->xerr, next->blocerr, next->saut,
                            next->codesaut);

#if DEBUG_BASE
                        printf("occ:%p  num %d  lon %d  saut %d  codesaut %d (longmod=%d)\n",
                                next->x,next->num,next->lon,next->saut,
                                next->codesaut, longmod);
                        afficheOcc(stdout, next, longmod+1, 0);
#endif
                        nbocc++;
                        if (next->x->fils[next->num]->debut & LEAF_BIT)
                            {
                            maxseq += nbSequenceInBitTab(((Feuille *)
                                next->x->fils[next->num])->sequences);
#if DEBUG_BT
                            printf("nb seq in bt (nd): %d \n",
                                    nbSequenceInBitTab(((Feuille *)
                                    next->x->fils[next->num])->sequences));
#endif
                            }
                        else
                            {
                            maxseq += next->x->fils[next->num]->nb_element_bt;
#if DEBUG_BT
                            printf("nb seq in bt (nd): %d \n",
                                    nbSequenceInBitTab(
                                    next->x->fils[next->num]->sequences));
#endif
                            }
                        }
                    }
                }         

/* Si on n'a plus d'occurrences dans la pile                                  */
            if(pos == 0)
                {
#if DEBUG_BASE
                printf("break avec %d occ\n",nbocc);
#endif
                break;
                }

            pos--;
            tmpocc=pocc->occ+pos;

#if DEBUG_PILE
            printf("pos pile %d (adresse %p), len mod %d, nbocc %d\n",pos,
                    tmpocc,longmod,nbocc);
            printf("x %p\n",tmpocc->x);
#endif
/*             if(pos==precdummy) */
/*                 fprintf(stderr,"pos %d et fin\n",pos); */
/*             else */
/*                 fprintf(stderr,"pos %d x %x\n",pos,(pocc->occ+pos)->x); */
            }

#if DEBUG_BASE
printf("J'ai trouve %d occ\n",nbocc);
afficheOldOcc(poccnew, longmod+1);
#endif


        if (nbocc == 0)
            continue;

/***************/
/* CAS DU SAUT */
/***************/
        tmp_quorum2 = -1;

        if (    multiblocs
            &&  (curbloc != cr->bloc-1)
            &&  (longcurbloc+1 >= cr->longbloc[curbloc].min )
            &&  (maxseq >= cr->quorum)
            &&  ( (tmp_quorum2 = sommeBTOcc(poccnew, colors_model) )
                     >= cr->quorum) 
            &&  ( gestionSaut(model, pocc, poccnew, cr, curbloc)) != 0 ) 
            {
            changeModel(model, symbol);
            changeModel(model, numSAUT);
                
            ajouteDummy(poccsaut);

            tmpint  = copieLastOcc(poccsaut,poccnew);
#if DEBUG_SAUT
            printf("J'ai copie %d occ de Pnew->Psaut\n",tmpint);
            afficheOldOcc(poccnew,longmod+1);
#endif
            videPile(poccnew);
            zarb_ext = 1;
            
            if ( cr->flag_compo == VRAI || cr->flag_compobloc[curbloc] == VRAI )
                {
                cr->compo[symbol]--;
                cr->compobloc[curbloc][symbol]--;
                }

            if(multiblocs)
                longbloc[curbloc]   = longcurbloc+1;
            
            zarb_back += spellModels(pocc, poccnew,poccsaut,
                longmod+1, 0, curbloc+1, multiblocs, model,
                next, colors_model, nbseq, tmp_quorum2, cr, longbloc,
                posdebbloc);


            if ( cr->flag_compo == VRAI ||  cr->flag_compobloc[curbloc] == VRAI )
                {
                cr->compo[symbol]++;
                cr->compobloc[curbloc][symbol]++;
                }
                
            decrModel(model); /* vire la premiere lettre du nouveau bloc */
            decrModel(model); /* vire le symbole de saut */

            videPile(poccnew);
            tmpint  = copieLastOcc(poccnew,poccsaut);
#if DEBUG_SAUT
            printf("J'ai copie %d occ de Psaut->Pnew\n",tmpint);
            afficheOldOcc(poccnew,longmod+1);
#endif
            depileRec(poccsaut);
            depileRec(pocc);
            }
        
#if DEBUG_BASE
        printf("nbocc = %d\n",nbocc);
        if (nbocc<=0)
            printf("Sortie nbocc\n");
        else if (maxseq < cr->quorum)
            printf("Sortie maxseq %ld\n",maxseq);
        else 
            printf("Calcul de quorum (maxseq = %ld) tmp_quorum2=%d\n",
                    maxseq,tmp_quorum2);
#endif

    

        if ( (maxseq >= cr->quorum)
            &&  ( tmp_quorum2!=-1 ? tmp_quorum2 >= cr->quorum:
            (tmp_quorum2 = sommeBTOcc(poccnew, colors_model) ) >= cr->quorum))
            {
#if DEBUG_BASE
            printf("Accepte (res quorum=%d)\n", tmp_quorum2);
#endif
            if(symbol == numJOKER)
                zarb_ext = 1;
            else
                nbnewmod++;

            changeModel(model,symbol);

            ajouteDummy(pocc);

            transferePile2Pile(pocc, poccnew);

            if ( cr->flag_compo == VRAI || cr->flag_compobloc[curbloc] == VRAI )
                {
                cr->compo[symbol]--;
                cr->compobloc[curbloc][symbol]--;
                }

            zarb_back += spellModels(pocc, poccnew,poccsaut, longmod+1,
                longcurbloc+1, curbloc, multiblocs, model, 
                next, colors_model, nbseq, tmp_quorum2, cr, longbloc,
                posdebbloc);

            if ( cr->flag_compo == VRAI || cr->flag_compobloc[curbloc] == VRAI )
                {
                cr->compo[symbol]++;
                cr->compobloc[curbloc][symbol]++;
                }

            depileRec(pocc);

            /* on decremente la longueur du modele de 1 */
            decrModel(model);
            } 
#if DEBUG_BASE
        else
                printf("Refuse curquorum=%d quorum=%d\n",tmp_quorum2,cr->quorum);
#endif
        }

/* Si: il n'y pas eu d'extension REGULIERE, la longueur courante est valide, */
/* ET [il n'y a pas eu d'extension bizarre (joker, saut) OU ces extensions   */
/* ont pose un probleme (modele se terminant par jokers)] */
/* printf("Avant test: %s\n",model->name); */
/* printf("nbnewmod: %d longmod: %d longcurbloc: %d zarb %d %d\n",nbnewmod,longmod,longcurbloc,zarb_back,zarb_ext); */
/* printf("%d %d %d %d %d %d %d\n",curbloc == cr->bloc-1,nbnewmod == 0,longmod >= cr->longueur.min,multiblocs == FAUX,longcurbloc >= cr->longbloc[curbloc].min,zarb_back!=0,zarb_ext==0); */
    if ( (curbloc == cr->bloc-1)
        /*&& (nbnewmod == 0)*/ && (longmod >= cr->longueur.min)
        && ( (multiblocs == FAUX) || (longcurbloc >= cr->longbloc[curbloc].min ) )
        && ( cr->flag_palindrom == FAUX || cr->palindrom[curbloc] == -1
            || longcurbloc == longbloc[(int)(cr->palindrom[curbloc])] )
        && ( (zarb_back!=0) || (zarb_ext==0) ) )
        {
        /* A VIRER? ce test est il inutile? */
        if ( (model->name[model->lon-1] != numJOKER)
            && (model->name[model->lon-1] != numSAUT) )
            {
            if(multiblocs)
                longbloc[curbloc]   = longcurbloc;
            
            keepModel(model, pocc, nbseq, tmp_quorum, longmod, longbloc, cr);
            return(0);
            }
        else
            return(1);
        }
    }
else if ( (curbloc == cr->bloc-1))
/*         && ( (cr->longueur.max != 0) && (longmod <= cr->longueur.max) ) */
/*         && ((multiblocs == FAUX) || ((cr->longbloc[curbloc].max != 0)  */
/*             && (longcurbloc <= cr->longbloc[curbloc].max) ) ) ) */
    {
    /* A VIRER? ce test est il inutile? */
    if ( (model->name[model->lon-1] != numJOKER)
        && (model->name[model->lon-1] != numSAUT))
        {
        if(multiblocs)
            longbloc[curbloc]   = longcurbloc;

        keepModel(model, pocc, nbseq, tmp_quorum, longmod, longbloc, cr);
        return(0);
        }
    else
        return(1);
    }
return(0);
}


/******************************************************************************/
/* doSpell                                                                   */
/******************************************************************************/
/* Lance la recursion sur les modeles.                                        */
/******************************************************************************/
void doSpell(P_Criteres cr, NbSeq nbseq, Noeud  *root)
{
/* bloc est un indicateur du bloc en cours de construction */
P_mod       model;
P_PileOcc   pocc, poccnew, poccsaut=NULL;
P_occ       next;
Bit_Tab     *colors_model;
Flag        multiblocs;
Noeud       *root_pere;
LongSeq     *longbloc=NULL;
LongSeq     *posdebbloc=NULL;


/* Creation du faux pere de la racine (pour faciliter la recursion)           */
root_pere  = Alloc_Noeud();
root_pere->fils[Translation_Table[FINAL]] = root;
root_pere->sequence_number              = 0;
root->debut                             = 0;
root->fin                               = 1;
root->sequence_number                   = 0;

if(cr->bloc == 1)
    multiblocs = FAUX;
else
    multiblocs = VRAI;
 
/* Allocation du modele                                                       */
model      = allocModel();

/* Allocation de l'occurrence courante                                        */
next = (P_occ) calloc (1,sizeof(Occ));
if (next == NULL)
    fatalError("doSpell: cannot allocate 'next'\n");
initOcc(next);

/* Allocation du tableau de bits courant                                      */
colors_model = AllocBitTab();
ReinitBitTab(&colors_model);
    

/* Allocation des piles d'occurrences                                         */
if( multiblocs == VRAI )
    {
    poccsaut    = creePileOcc();
    longbloc    = (LongSeq *) malloc(cr->bloc *  sizeof(LongSeq));
    if (longbloc == NULL)
        fatalError("doSpell: cannot allocate 'longbloc'\n");

    if ( cr->flag_palindrom )
        {
        posdebbloc = (LongSeq *) malloc(cr->bloc *  sizeof(LongSeq));
        if (posdebbloc == NULL)
            fatalError("doSpell: cannot allocate 'posdebbloc'\n");
        }
    }

poccnew  = creePileOcc();
pocc     = creePileOcc();

/* Ajout de l'occurrence nulle dans la pile d'occurrence                      */
ajouteInitOcc2Pile(pocc, root_pere);

fprintf(stderr,"** Models extraction **\n");
barre((int)pow((double)nbSymbMod, 3.0));

/* ...et lancement de la recursion                                            */
spellModels(pocc, poccnew, poccsaut, 0, 0, 0, multiblocs,
     model, next, &colors_model, nbseq, 0, cr, longbloc, posdebbloc);


/* Liberation des structures                                                  */
free(next);
free(model->name);
free(model);
free(colors_model);
liberePileOcc(pocc);
liberePileOcc(poccnew);
if(multiblocs == VRAI)
    liberePileOcc(poccsaut);
}  



/******************************************************************************/
/******************************************************************************/
/********************************** MAIN **************************************/
/******************************************************************************/
/******************************************************************************/
int main(int argc, char **argv)
{
FastaSequence   **seq;
Flag            readok;
char            infini = 0, buf[100];
NbSeq           nbtxt;
float           quorum = 0.0, user_time;
int             i, j, posarg, taille, siztxt;
long int        nbsymb;
LongSeq         maxlongsaut = 0;
Noeud           *arbre_suffixe; 
Criteres        criteres;
Symbole         *alphaseq;

posarg  = 4;

/* QUORUM                                                                     */
quorum = atof(argv[posarg++]);

/* BLOCS                                                                      */
criteres.bloc = atoi(argv[posarg++]);
allocBloc(&criteres, criteres.bloc);

/* LONGUEUR MIN                                                               */
criteres.longueur.min = (LongSeq)atoi(argv[posarg++]);

/* LONGUEUR MAX                                                               */
criteres.longueur.max = (LongSeq)atoi(argv[posarg++]);
 if ( criteres.longueur.max == 0 )
   infini = 1;

/* ERREURS GLOBALES                                                           */
criteres.maxerr = (LongSeq)atoi(argv[posarg++]);

/* PARAMETRES BLOCS ***********************************************************/
if(criteres.bloc > 1)
    {
    for(i = 0; i != criteres.bloc; i++ )
        {

/* LONGUEUR MIN BLOC                                                          */
        criteres.longbloc[i].min = (LongSeq)atoi(argv[posarg++]);

/* LONGUEUR MAX BLOC                                                          */
        criteres.longbloc[i].max = (LongSeq)atoi(argv[posarg++]);
        if ( criteres.longbloc[i].max == 0 )
          infini = 1;
        maxlongmod += criteres.longbloc[i].max;

/* ERREURS BLOC                                                               */
        criteres.maxerrblocs[i] = atoi(argv[posarg++]);

        if(i != criteres.bloc-1 )
            {
/* SAUT MIN BLOC                                                              */
            criteres.saut[i].min = (LongSeq)atoi(argv[posarg++]);

/* SAUT MAX BLOC                                                              */
            criteres.saut[i].max = (LongSeq)atoi(argv[posarg++]);
            
            maxlongsaut += criteres.saut[i].max;
            }
        }
    }

 if ( infini == 0 )
   {
     if (maxlongmod < criteres.longueur.max)
       maxlongmod = criteres.longueur.max;
     
     maxlongmod += maxlongsaut; 
   }
 else
  maxlongmod = INT_MAX;
 



/******************************************************************************/
/* TRAITEMENT DES SEQUENCES                                                   */
/******************************************************************************/

/* Allocations                                                                */
seq     = (FastaSequence **) malloc(GRAINSEQ * sizeof(FastaSequence *));
text    = (signed char **) malloc(GRAINSEQ * sizeof(signed char *));
if(!seq || !text)
    fatalError("main: seq/text: cannot allocate\n");
siztxt  = GRAINSEQ;


/* Ouverture du fichier contenant les sequences                               */
f      = fopen (argv[2],"r"); 
if(f==NULL)
    {
    fprintf(stderr,"Error: main: cannot open FASTA file '%s'\n",argv[2]);
    exit(1);
    }

readok = 1;
nbtxt  = 0;
nbsymb  = 0;

/* Stockage des sequences en memoire                                          */
do
    {
    if(nbtxt == siztxt)
        {
        siztxt  *= 2;
        seq  = (FastaSequence **) realloc(seq,siztxt * sizeof(FastaSequence *));
        text = (signed char **)  realloc(text, siztxt * sizeof(signed char *));
        if(!seq || !text)
            fatalError("main: seq/text: cannot reallocate\n");
        }

    seq[nbtxt] = NewFastaSequence();
    readok     = ReadFastaSequence(f, seq[nbtxt]);
    if (readok)
        {
        nbsymb        += seq[nbtxt]->length;
        taille        = seq[nbtxt]->length+1;
        text[nbtxt]   = (signed char *) malloc ((taille+2) * sizeof(signed char)); 
        if (text[nbtxt] == NULL)
            fatalError("main: cannot allocate 'text'\n");

/*         printf("Seq %d\n", nbtxt); */
/*         printf("%d symboles lus\n",taille-1); */
/*         for(i=0; i!=taille; i++) */
/*             printf("%c.",seq[nbtxt]->seq[i]); */
/*         printf("\n"); */
        strcpy((char *) text[nbtxt],seq[nbtxt]->seq);
        text[nbtxt][taille-1] = FINAL;
        text[nbtxt][taille]   = '\0';
        nbtxt++;
        }
    }
while (readok);

fclose(f); 



if (nbtxt == 0)
    fatalError("No sequence in FASTA file!\n");

if (nbtxt == 1)
    fatalError("One sequence only in FASTA file!\n");

criteres.nbsymb = nbsymb;

if (quorum == 0.0)
    criteres.quorum = (NbSeq) ceil( (double) (70*nbtxt)/100.0);
else
    criteres.quorum = (NbSeq) ceil( (double) (quorum*nbtxt)/100.0);

if(criteres.quorum==1)
    warning("quorum value is 1 sequence!");
else if(criteres.quorum<1)
    fatalError("main: quorum value is lower than 1 sequence!");

/******************************************************************************/
/* Chargement alphabet sequences et modeles                                   */
if(!(f=fopen(argv[1],"r")))
    {
    fprintf(stderr,"Error: main: cannot open alphabet file '%s'\n",argv[1]);
    exit(1);
    }

initAlphabet();

if(!(alphaseq    = chargeAlphabet(f, (Symbole **) text, nbtxt)))
    fatalError("main: incorrect alphabet file format\n");

fclose(f);


/******************************************************************************/
/* COMPOSITION (traitee apres car besoin alphabet modeles)                    */
/* S'il reste des arguments, c'est la composition et/ou les palindromes       */
setCompoPal(&criteres, argv+posarg, argc-posarg);


/* Transformation de l'alphabet (ex: AG => R)                                 */
/* + cr�ation des compl�mentarit�s pour palindromes                           */
transAlphMod(criteres.flag_palindrom);

/******************************************************************************/
/* Construction de l'arbre compact generalise                                 */
fprintf(stderr, "** Suffix tree construction **\n");
barre(nbtxt);
Init_All(alphaseq, 0, nbtxt);
arbre_suffixe = Construction_Arbre((unsigned char *)text[0], maxlongmod);
barre(0);

for (i = 1; i != nbtxt; i++)
    {
    arbre_suffixe=AjouteSequence(arbre_suffixe,(unsigned char *)text[i],
        maxlongmod);
    barre(0);
    }
fprintf(stderr,"\n");

/******************************************************************************/
/* Liberation de la structure Fasta                                           */
for(i=0;i != nbtxt;i++)
    FreeFastaSequence(seq[i]);
free(seq);

#if DEBUG_TREE
fprintf(stderr,"\nNB Feuilles: %d\n\n",calculFeuilles(arbre_suffixe));
#endif

UpdateBit_TabForAllTree(arbre_suffixe);

/* if (flag_tree == VRAI) */
/* Print_Tree(arbre_suffixe,1,0); */


strcpy(buf,argv[3]);
/* if(!strcmp(buf+strlen(buf)-4,".out")) */
/*     buf[strlen(buf)-4]='\0'; */
/************************ enumeration des resultats ***************************/
printf("Extraction is going to be made with the following parameters:\n");
printf("FASTA file:                    %s\n",argv[2]);
printf("Alphabet file:                 %s\n",argv[1]);
printf("Output file:                   %s\n",argv[3]);
printf("Total min length:              %d\n",criteres.longueur.min);
if (criteres.longueur.max == 0)
    printf("Total max length:              MAX\n");
else
    printf("Total max length:              %d\n",criteres.longueur.max);
printf("Boxes:                         %d\n",criteres.bloc);
printf("Total number of subst.:        %d\n",criteres.maxerr);
printf("Quorum:                        %f%% (%d sequences in %d)\n\n",
        quorum,criteres.quorum,nbtxt);

if (criteres.flag_compo)
    {
    for (i = 0; i != nbSymbMod; i++)
        {
        if (criteres.compo[i] != -1)
printf("Total max composition in %s:    %d\n",nummod2str[i],
                    criteres.compo[i]);
        }
    }

if (criteres.bloc > 1)
    {
    for (i = 0; i != criteres.bloc; i++)
        {
        printf("\nBOX %d\n",i+1);
        printf("Min length:                    %d\n",criteres.longbloc[i].min);
        if (criteres.longbloc[i].max == 0)
            printf("Max length:                    MAX\n");
        else
            printf("Max length:                    %d\n",
                    criteres.longbloc[i].max);
        printf("Max number of subst.:          %d\n",
                criteres.maxerrblocs[i]);
        if (i != criteres.bloc-1)
            {
            printf("Min spacer length:             %d\n",criteres.saut[i].min);
            printf("Max spacer length:             %d\n",criteres.saut[i].max);
/*             sprintf(buf2,"[%d-%d]",criteres.saut[i].min,criteres.saut[i].max); */
/*             strcat(buf,buf2); */
            }

        if (criteres.flag_compobloc[i])
            {
            for (j = 0; j != nbSymbMod; j++)
                    if (criteres.compobloc[i][j] != -1)
                        printf("Max composition in %s:          %d\n",
                                nummod2str[j],criteres.compobloc[i][j]);
            }

        if (criteres.palindrom[i]!=-1)
            printf("Palindrom of box:              %d\n", criteres.palindrom[i]+1);
        }
    }


fprintf(stderr,"\n                   ------ CHECK THESE PARAMETERS! ------\n");

if(criteres.bloc > 1)
        initTabSauts(&criteres);

if (!strcmp(argv[2],"stdout"))
    f = stdout;
else
    {
/*     strcat(buf,".out"); */
    if(!(f = fopen(buf,"w")))
        {
        fprintf(stderr,"Error: main: cannot open output file '%s'\n",buf);
        exit(1);
        }
    }

/* Remise a zero de maxlongmod  pour recalcul de vraie longueur max           */
/* et allocation du tableau des longueurs max des blocs                       */
maxlongmod  = 0;
if(!(maxlongbloc = (LongSeq *) calloc(criteres.bloc, sizeof(LongSeq))))
    fatalError("main: cannot allocate 'maxlongbloc'\n");

/* Insertion de l'espace necessaire en tete de fichier pour y mettre la       */
/* ligne d'informations apres extraction                                      */
/* J'ecris en tout 80 * 3 = 240 espaces et 80 '=' => 320 caracteres           */
for(i=0; i!=3; i++)
    {
    fprintf(f,"                                        ");
    fprintf(f,"                                       \n");
    }
fprintf(f,"========================================");
fprintf(f,"=======================================\n");

/******************************************************************************/
/******************************************************************************/
/* Fonction principale                                                        */
PrintCpuTime(1);
doSpell(&criteres,nbtxt,arbre_suffixe);
user_time=PrintCpuTime(0);
/******************************************************************************/
/******************************************************************************/

for(i=0; i!=nbtxt; i++)
    free(text[i]);
free(text);


/* Affichage et insertion du nb de modeles trouves en fin de fichier          */
printf("\nNb models: %d\nUser time : %.2f sec.\n", nbmod, user_time);
fprintf(f,"Nb models: %d\nUser time : %.2f sec.\n", nbmod, user_time);

/******************************************************************************/
/* Insertion de la ligne de parametres en tete du fichier                     */
rewind(f);
fprintf(f,"%%%%%% %d %d/%d %ld %d %d %d",criteres.bloc,criteres.quorum,nbtxt,
        criteres.nbsymb, criteres.longueur.min, maxlongmod, criteres.maxerr);

/* Ecriture des dimensions des blocs trouves                                  */
if(criteres.bloc > 1)
    {
    for(j=0; j!=criteres.bloc; j++)
        {
        fprintf(f," %d %d %d",criteres.longbloc[j].min,
                maxlongbloc[j], criteres.maxerrblocs[j]);
        if(j!=criteres.bloc-1)
            fprintf(f," %d %d", criteres.saut[j].min, criteres.saut[j].max);
        }
    }

/* Ecriture du nom du fichier alphabet utilise et de l'alphabet des sequences */
fprintf(f," %s %s", argv[1], alphaseq);
/******************************************************************************/

  
/* Liberation de l'arbre                                                      */
Free_Arbre(arbre_suffixe);

return(0); 
}

/******************************************************************************/
/* PrintCpuTime                                                               */
/******************************************************************************/
static float PrintCpuTime(char flag)
{
float         ust;
struct tms    tms;
static float  dust;

times(&tms);

ust =  (float) tms.tms_utime;

if (flag)
    {
    dust = ust;
    return 0.0;
    }
else
    {
    ust -= dust;
    return ust / sysconf(_SC_CLK_TCK);
    }
}
