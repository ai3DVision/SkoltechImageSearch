/*
Copyright © INRIA 2010-2011. 
Authors: Matthijs Douze & Herve Jegou 
Contact: matthijs.douze@inria.fr  herve.jegou@inria.fr

This software is a computer program whose purpose is to provide 
efficient tools for basic yet computationally demanding tasks, 
such as find k-nearest neighbors using exhaustive search 
and kmeans clustering. 

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software.  You can  use, 
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info". 

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability. 

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or 
data to be ensured and,  more generally, to use and operate it in the 
same conditions as regards security. 

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
*/


/* This code was written by Herve Jegou. Contact: herve.jegou@inria.fr  */
/* Last change: June 1st, 2010                                          */
/* This software is governed by the CeCILL license under French law and */
/* abiding by the rules of distribution of free software.               */
/* See http://www.cecill.info/licences.en.html                          */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "hamming.h"

/* the slice size is set to avoid testing the buffer size too often */
#define HAMMATCH_SLICESIZE 16

/* geometric re-allocation: add a constant size plus a relative 50% of additional memory */
#define HAMMATCH_REALLOC_NEWSIZE(oldsize) (HAMMATCH_SLICESIZE+((oldsize * 5) / 4))



static uint16 uint8_nbones[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};



/* Elementary Hamming distance computation */

uint16 hamming_generic (const uint8 *bs1, const uint8 * bs2, int ncodes)
{
  int i;
  uint16 ham = 0;

  for (i = 0; i < ncodes ; i++) {
    ham += uint8_nbones[*bs1 ^ *bs2];
    bs1++;
    bs2++;
  }

  return ham;
}


#ifndef __SSE4_2__
#warning "SSE4.2 NOT USED FOR HAMMING DISTANCE COMPUTATION. Consider adding -msse4 in makefile.inc!"

uint16 hamming_32 (const uint32 * bs1, const uint32 * bs2)
{
  uint16 ham = 0;
  uint32 diff = ((*bs1) ^ (*bs2));

  ham = uint8_nbones[diff & 255];
  diff >>= 8;
  ham += uint8_nbones[diff & 255];
  diff >>= 8;
  ham += uint8_nbones[diff & 255];
  diff >>= 8;
  ham += uint8_nbones[diff];
  return ham;
}


uint16 hamming_64 (const uint64 * bs1, const uint64 * bs2)
{
  uint16 ham = 0;
  uint64 diff = ((*bs1) ^ (*bs2));

  ham = uint8_nbones[diff & 255];
  diff >>= 8;
  ham += uint8_nbones[diff & 255];
  diff >>= 8;
  ham += uint8_nbones[diff & 255];
  diff >>= 8;
  ham += uint8_nbones[diff & 255];
  diff >>= 8;
  ham += uint8_nbones[diff & 255];
  diff >>= 8;
  ham += uint8_nbones[diff & 255];
  diff >>= 8;
  ham += uint8_nbones[diff & 255];
  diff >>= 8;
  ham += uint8_nbones[diff];

  return ham;
}

#endif



void compute_hamming_32 (uint16 * dis, const uint8 * a, const uint8 * b, int na, int nb)
{
  int i, j;
  const uint8 * pb = b;
  for (j = 0 ; j < nb ; j++) {
    const uint8 * pa = a;
    for (i = 0 ; i < na ; i++) {
      *dis = hamming_32 (pa, pb);
      pa += 4;
      dis++;
    }
    pb += 4;
  }
}


void compute_hamming_64 (uint16 * dis, const uint8 * a, const uint8 * b, int na, int nb)
{
  int i, j;
  const uint8 * pb = b;
  for (j = 0 ; j < nb ; j++) {
    const uint8 * pa = a;
    for (i = 0 ; i < na ; i++) {
      *dis = hamming_64 (pa, pb);
      pa += 8;
      dis++;
    }
    pb += 8;
  }
}


void compute_hamming_128 (uint16 * dis, const uint8 * a, const uint8 * b, int na, int nb)
{
  int i, j;
  const uint8 * pb = b;
  for (j = 0 ; j < nb ; j++) {
    const uint8 * pa = a;
    for (i = 0 ; i < na ; i++) {
      *dis = hamming_128 (pa, pb);
      pa += 16;
      dis++;
    }
    pb += 16;
  }
}

void compute_hamming (uint16 * dis, const uint8 * a, const uint8 * b, int na, int nb)
{
  int i, j;
  const uint8 * pb = b;
  for (j = 0 ; j < nb ; j++) {
    const uint8 * pa = a;
    for (i = 0 ; i < na ; i++) {
      *dis = hamming (pa, pb);
      pa += BITVECBYTE;
      dis++;
    }
    pb += BITVECBYTE;
  }
}



void compute_hamming_generic (uint16 * dis, const uint8 * a, const uint8 * b, 
                              int na, int nb, int ncodes)
{
  switch (ncodes) {
    case 4: 
      compute_hamming_32 (dis, a, b, na, nb);
      return;
    case 8: 
      compute_hamming_64 (dis, a, b, na, nb);
      return;
    case 16: 
      compute_hamming_128 (dis, a, b, na, nb);
      return;
  }
      
  if (ncodes == BITVECBYTE) {
    compute_hamming (dis, a, b, na, nb);
    return;
  }
  
  fprintf (stderr, "# Warning: non-optimized version of the Hamming distance\n");  
     
  int i, j;
  const uint8 * pb = b;
  for (j = 0 ; j < nb ; j++) {
    const uint8 * pa = a;
    for (i = 0 ; i < na ; i++) {
      *dis = hamming_generic (pa, pb, ncodes);
      pa += ncodes;
      dis++;
    }
    pb += ncodes;
  }
}


/* Compute hamming distance and report those below a given threshold in a structure array */
hammatch_t * hammatch_new (int n)
{
  return (hammatch_t *) malloc (n * sizeof (hammatch_t));
}


hammatch_t * hammatch_realloc (hammatch_t * m, int n)
{
  return (hammatch_t *) realloc (m, n * sizeof (hammatch_t));
}



void match_hamming_thres (const uint8 * qbs, const uint8 * dbs, int nb, int ht,
                          size_t bufsize, hammatch_t ** hmptr, size_t * nptr)
{
  size_t j, posm = 0;
  uint16 h;
  *hmptr = hammatch_new (bufsize);
  hammatch_t * hm = *hmptr;
  
  for (j = 0 ; j < nb ; j++) {
    
    /* Here perform the real work of computing the distance */
    h = hamming (qbs, dbs);
            
    /* collect the match only if this satisfies the threshold */
    if (h <= ht) {
      /* Enough space to store another match ? */
      if (posm >= bufsize) {
          bufsize = HAMMATCH_REALLOC_NEWSIZE (bufsize);
          *hmptr = hammatch_realloc (*hmptr, bufsize);
          assert (*hmptr != NULL);
          hm = (*hmptr) + posm;
      }
      
      hm->bid = j;
      hm->score = h;
      hm++;
      posm++;
    }
    dbs += BITVECBYTE;  /* next signature */
  }
  
  *nptr = posm;
}


void match_hamming_thres_generic (const uint8 * qbs, const uint8 * dbs, 
                                  int nb, int ht, size_t bufsize, 
                                  hammatch_t ** hmptr, size_t * nptr, size_t ncodes)
{
  size_t j, posm = 0;
  uint16 h;
  *hmptr = hammatch_new (bufsize);
  hammatch_t * hm = *hmptr;
  
  for (j = 0 ; j < nb ; j++) {
    
    /* Here perform the real work of computing the distance */
    h = hamming_generic (qbs, dbs, ncodes);
    
    /* collect the match only if this satisfies the threshold */
    if (h <= ht) {
      /* Enough space to store another match ? */
      if (posm >= bufsize) {
        bufsize = HAMMATCH_REALLOC_NEWSIZE (bufsize);
        *hmptr = hammatch_realloc (*hmptr, bufsize);
        assert (*hmptr != NULL);
        hm = (*hmptr) + posm;
      }
      
      hm->bid = j;
      hm->score = h;
      hm++;
      posm++;
    }
    dbs += ncodes;  /* next signature */
  }
  
  *nptr = posm;
}


void crossmatch_he (const uint8 * dbs, long n, int ht,
                    long bufsize, hammatch_t ** hmptr, size_t * nptr)
{
  size_t i, j, posm = 0;
  uint16 h;
  *hmptr = hammatch_new (bufsize);
  hammatch_t * hm = *hmptr;
  const uint8 * bs1 = dbs;
  
  for (i = 0 ; i < n ; i++) {
    const uint8 * bs2 = bs1 + BITVECBYTE;
    
    for (j = i + 1 ; j < n ; j++) {
      
      /* Here perform the real work of computing the distance */
      h = hamming (bs1, bs2);
      
      /* collect the match only if this satisfies the threshold */
      if (h <= ht) {
        /* Enough space to store another match ? */
        if (posm >= bufsize) {
          bufsize = HAMMATCH_REALLOC_NEWSIZE (bufsize);
          *hmptr = hammatch_realloc (*hmptr, bufsize);
          assert (*hmptr != NULL);
          hm = (*hmptr) + posm;
        }
        
        hm->qid = i;
        hm->bid = j;
        hm->score = h;
        hm++;
        posm++;
      }
      bs2 += BITVECBYTE;
    }
    bs1  += BITVECBYTE;  /* next signature */
  }
  
  *nptr = posm;
}


/* Same as crossmatch_he, but includes 
  - twice the matches: match (i,j,h) also gives the match (j,i,h)
  - self-matches of the form (i,i,0)
*/
void crossmatch_he2 (const uint8 * dbs, long n, int ht,
                    long bufsize, hammatch_t ** hmptr, size_t * nptr)
{
  size_t i, j, posm = 0;
  uint16 h;
  *hmptr = hammatch_new (bufsize);
  hammatch_t * hm = *hmptr;
  const uint8 * bs1 = dbs;
  
  for (i = 0 ; i < n ; i++) {
    const uint8 * bs2 = dbs;
    
    for (j = 0 ; j < n ; j++) {
      
      /* Here perform the real work of computing the distance */
      h = hamming (bs1, bs2);
      
      /* collect the match only if this satisfies the threshold */
      if (h <= ht) {
        /* Enough space to store another match ? */
        if (posm >= bufsize) {
          bufsize = HAMMATCH_REALLOC_NEWSIZE (bufsize);
          *hmptr = hammatch_realloc (*hmptr, bufsize);
          assert (*hmptr != NULL);
          hm = (*hmptr) + posm;
        }
        
        hm->qid = i;
        hm->bid = j;
        hm->score = h;
        hm++;
        posm++;
      }
      bs2 += BITVECBYTE;
    }
    bs1  += BITVECBYTE;  /* next signature */
  }
  
  *nptr = posm;
}


int crossmatch_he_prealloc (const uint8 * dbs, long n, int ht,  
                            int * idx, uint16 * hams)
{
  size_t i, j, posm = 0;
  uint16 h;

  const uint8 * bs1 = dbs;
  
  for (i = 0 ; i < n ; i++) {
    const uint8 * bs2 = bs1 + BITVECBYTE;
    
    for (j = i + 1 ; j < n ; j++) {
      
      /* Here perform the real work of computing the distance */
      h = hamming (bs1, bs2);
      
      /* collect the match only if this satisfies the threshold */
      if (h <= ht) {
        
        /* Enough space to store another match ? */
        *idx = i; idx++;
        *idx = j; idx++;
        
        *hams = h;
        hams++;
        posm++;
      }
      bs2 += BITVECBYTE;
    }
    bs1 += BITVECBYTE;  /* next signature */
  }
  
  return posm;
}


int crossmatch_he_prealloc2 (const uint8 * dbs, long n, int ht,  
                             int * idx, uint16 * hams)
{
  size_t i, j, posm = 0;
  uint16 h;
  
  const uint8 * bs1 = dbs;
  
  for (i = 0 ; i < n ; i++) {
    const uint8 * bs2 = dbs;
    
    for (j = 0 ; j < n ; j++) {
      
      /* Here perform the real work of computing the distance */
      h = hamming (bs1, bs2);
      
      /* collect the match only if this satisfies the threshold */
      if (h <= ht) {
        
        /* Enough space to store another match ? */
        *idx = i; idx++;
        *idx = j; idx++;
        
        *hams = h;
        hams++;
        posm++;
      }
      bs2 += BITVECBYTE;
    }
    bs1 += BITVECBYTE;  /* next signature */
  }
  
  return posm;
}


void crossmatch_he_count (const uint8 * dbs, int n, int ht, size_t * nptr)
{
  size_t i, j, posm = 0;
  const uint8 * bs1 = dbs;
  
  for (i = 0 ; i < n ; i++) {
    const uint8 * bs2 = bs1 + BITVECBYTE;
    
    for (j = i + 1 ; j < n ; j++) {
      
      /* collect the match only if this satisfies the threshold */
      if (hamming (bs1, bs2) <= ht) 
        posm++;
      
      bs2 += BITVECBYTE;
    }
    bs1  += BITVECBYTE;  /* next signature */
  }
  
  *nptr = posm;
}


void crossmatch_he_count2 (const uint8 * dbs, int n, int ht, size_t * nptr)
{
  size_t i, j, posm = 0;
  const uint8 * bs1 = dbs;
  
  for (i = 0 ; i < n ; i++) {
    const uint8 * bs2 = dbs;
    
    for (j = 0 ; j < n ; j++) {
      
      /* collect the match only if this satisfies the threshold */
      if (hamming (bs1, bs2) <= ht) 
        posm++;
      
      bs2 += BITVECBYTE;
    }
    bs1  += BITVECBYTE;  /* next signature */
  }
  
  *nptr = posm;
}




/*-------------------------------------------*/
/* Threaded versions, if OpenMP is available */
#ifdef _OPENMP
void compute_hamming_thread (uint16 * dis, const uint8 * a, const uint8 * b, 
                             int na, int nb)
{
  long i, j;
#pragma omp parallel shared (dis, a, b, na, nb) private (i, j)
    {
#pragma omp for 
      for (j = 0 ; j < nb ; j++)
	      for (i = 0 ; i < na ; i++)
	        dis[j * na + i] = hamming (a + i * BITVECBYTE, b + j * BITVECBYTE);
    }
}

#endif /* _OPENMP */


