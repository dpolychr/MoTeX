/*
 *  Copyright (c) Atelier de BioInformatique
 *  
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *  
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA 02111-1307, USA
 *  
 *  For questions, suggestions, bug-reports, enhancement-requests etc.
 *  I may be contacted at: Alain.Viari@inrialpes.fr
 */

#ifndef _H_Gtypes
#include "Gtypes.h"
#endif

#define _H_libsysk

/* ==================================================== */
/* Constantes						*/
/* ==================================================== */

#define TICKS_PER_SEC	60

#define TIME_RESET	Vrai
#define TIME_NO_RESET	Faux

/* ==================================================== */
/*  Prototypes (generated by mproto)			*/
/* ==================================================== */

					/* libsysk.c 	*/
					
float	UserCpuTime	P((	Bool reset	));
float	SysCpuTime	P((	Bool reset	));
char	*StrCpuTime	P((	Bool reset	));

				
void	SetUpKmrNotify   P(( Bool notif 			 ));
Bool	GetKmrNotify     P(( void	 			 ));
void	NotifyKmrStep	 P(( Int32 wlen, Int32 wnb 		 ));
void	NotifyKmrEnd	 P(( Int32 wlen, Int32 maxlen, float cpu ));
void 	NotifyKmrError   P(( char *msg, Int32 wlen 		 ));