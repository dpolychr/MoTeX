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

#ifndef _H_libfasta

#define _H_libfasta

#ifndef _H_Gtypes
#include "Gtypes.h"
#endif

/* ==================================================== */
/* Constantes						*/
/* ==================================================== */

#define FASTA_NAMLEN  64  	/* max length of seq. name	 */
#define FASTA_COMLEN  512 	/* max length of seq. comment	 */

#define FASTA_CHAR_PER_LINE 50	/* # of chars per line in output */

/* ==================================================== */
/* Macros standards					*/
/* ==================================================== */

#ifndef NEW
#define NEW(object)		(object*)malloc(sizeof(object)) 
#define NEWN(object, dim) 	(object*)malloc((unsigned)(dim) * sizeof(object))
#define REALLOC(typ, ptr, dim)	(typ*)realloc((void *) (ptr), (unsigned long)(dim) * sizeof(typ))
#define FREE(object)		free(object)
#endif

/* ==================================================== */
/* Structures de donnees				*/
/* ==================================================== */

typedef struct {			/* -- Sequence ---------------- */
	Bool	ok;			/* error flag			*/
	Int32	length,			/* longueur			*/
		offset,			/* offset			*/
		bufsize;		/* size of current seq buffer	*/
	char    name[FASTA_NAMLEN],	/* nom 				*/
		comment[FASTA_COMLEN],	/* commentaire			*/
		*seq;			/* sequence			*/
} FastaSequence, *FastaSequencePtr;

/* ==================================================== */
/*  Prototypes (generated by mkproto)			*/
/* ==================================================== */

					/* libfasta.c 	*/

Int32 		 CountAlpha 	    P(( char *buf ));
char  		 *StrcpyAlpha 	    P(( char *s1 , char *s2 ));
char  		 *NextSpace 	    P(( char *buffer ));
char  		 *GetFastaName 	    P(( char *buffer ));
char 		 *GetFastaComment   P(( char *buffer ));

FastaSequencePtr FreeFastaSequence  P(( FastaSequencePtr seq ));
FastaSequencePtr NewFastaSequence   P(( void ));

Bool		 ReadFastaSequence  P(( FILE *streamin, FastaSequencePtr seq ));
Bool		 GetFastaSequence   P(( FILE *streamin, FastaSequencePtr seq ));
void 		 WriteFastaSequence P(( FILE *streamou, FastaSequencePtr seq , Int32 char_per_line ));

#endif