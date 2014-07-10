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

#ifndef _BARRE_
#define _BARRE_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHARBARRE   '.'
#define MAXCOL      60      /* Nombre de colonnes de la barre */

/******************************************************************************
  barre permet d'afficher une barre d'etat lors d'un calcul progressif.
  Le premier appel se fait en donnant le nombre d'etats a franchir
  avant d'atteindre les 100%. La fonction est alors initialisee.
  Ensuite il suffit de l'appeler avec la valeur 0, le nombre de
  fois annonc�.
  Sortie sur stderr.
  MAXCOL permet de definir la taille de la barre. Mis a 0 la barre est
  desactivee pour ne laisser que le temps et le pourcentage.
*******************************************************************************/
void    barre(int);

#endif

