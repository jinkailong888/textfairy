/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -
 -  Redistribution and use in source and binary forms, with or without
 -  modification, are permitted provided that the following conditions
 -  are met:
 -  1. Redistributions of source code must retain the above copyright
 -     notice, this list of conditions and the following disclaimer.
 -  2. Redistributions in binary form must reproduce the above
 -     copyright notice, this list of conditions and the following
 -     disclaimer in the documentation and/or other materials
 -     provided with the distribution.
 -
 -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================*/

/*
 *   ptafunc2.c
 *
 *      Sorting
 *           PTA        *ptaSort()
 *           l_int32     ptaGetSortIndex()
 *           PTA        *ptaSortByIndex()
 *           PTAA       *ptaaSortByIndex()
 *
 *      Union and intersection by aset (rbtree)
 *           PTA        *ptaUnionByAset()
 *           PTA        *ptaRemoveDupsByAset()
 *           PTA        *ptaIntersectionByAset()
 *           L_ASET     *l_asetCreateFromPta()
 *
 *      Union and intersection by hash
 *           PTA        *ptaUnionByHash()
 *           l_int32     ptaRemoveDupsByHash()
 *           PTA        *ptaIntersectionByHash();
 *           l_int32     ptaFindPtByHash()
 *           L_DNAHASH  *l_dnaHashCreateFromPta()
 */

#include "allheaders.h"


/*---------------------------------------------------------------------*
 *                               Sorting                               *
 *---------------------------------------------------------------------*/
/*!
 *  ptaSort()
 *
 *      Input:  ptas
 *              sorttype (L_SORT_BY_X, L_SORT_BY_Y)
 *              sortorder  (L_SORT_INCREASING, L_SORT_DECREASING)
 *              &naindex (<optional return> index of sorted order into
 *                        original array)
 *      Return: ptad (sorted version of ptas), or null on error
 */
PTA *
ptaSort(PTA     *ptas,
        l_int32  sorttype,
        l_int32  sortorder,
        NUMA   **pnaindex)
{
PTA   *ptad;
NUMA  *naindex;

    PROCNAME("ptaSort");

    if (pnaindex) *pnaindex = NULL;
    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);
    if (sorttype != L_SORT_BY_X && sorttype != L_SORT_BY_Y)
        return (PTA *)ERROR_PTR("invalid sort type", procName, NULL);
    if (sortorder != L_SORT_INCREASING && sortorder != L_SORT_DECREASING)
        return (PTA *)ERROR_PTR("invalid sort order", procName, NULL);

    if (ptaGetSortIndex(ptas, sorttype, sortorder, &naindex) != 0)
        return (PTA *)ERROR_PTR("naindex not made", procName, NULL);

    ptad = ptaSortByIndex(ptas, naindex);
    if (pnaindex)
        *pnaindex = naindex;
    else
        numaDestroy(&naindex);
    if (!ptad)
        return (PTA *)ERROR_PTR("ptad not made", procName, NULL);
    return ptad;
}


/*!
 *  ptaGetSortIndex()
 *
 *      Input:  ptas
 *              sorttype (L_SORT_BY_X, L_SORT_BY_Y)
 *              sortorder  (L_SORT_INCREASING, L_SORT_DECREASING)
 *              &naindex (<return> index of sorted order into
 *                        original array)
 *      Return: 0 if OK, 1 on error
 */
l_int32
ptaGetSortIndex(PTA     *ptas,
                l_int32  sorttype,
                l_int32  sortorder,
                NUMA   **pnaindex)
{
l_int32    i, n;
l_float32  x, y;
NUMA      *na;

    PROCNAME("ptaGetSortIndex");

    if (!pnaindex)
        return ERROR_INT("&naindex not defined", procName, 1);
    *pnaindex = NULL;
    if (!ptas)
        return ERROR_INT("ptas not defined", procName, 1);
    if (sorttype != L_SORT_BY_X && sorttype != L_SORT_BY_Y)
        return ERROR_INT("invalid sort type", procName, 1);
    if (sortorder != L_SORT_INCREASING && sortorder != L_SORT_DECREASING)
        return ERROR_INT("invalid sort order", procName, 1);

        /* Build up numa of specific data */
    n = ptaGetCount(ptas);
    if ((na = numaCreate(n)) == NULL)
        return ERROR_INT("na not made", procName, 1);
    for (i = 0; i < n; i++) {
        ptaGetPt(ptas, i, &x, &y);
        if (sorttype == L_SORT_BY_X)
            numaAddNumber(na, x);
        else
            numaAddNumber(na, y);
    }

        /* Get the sort index for data array */
    *pnaindex = numaGetSortIndex(na, sortorder);
    numaDestroy(&na);
    if (!*pnaindex)
        return ERROR_INT("naindex not made", procName, 1);
    return 0;
}


/*!
 *  ptaSortByIndex()
 *
 *      Input:  ptas
 *              naindex (na that maps from the new pta to the input pta)
 *      Return: ptad (sorted), or null on  error
 */
PTA *
ptaSortByIndex(PTA   *ptas,
               NUMA  *naindex)
{
l_int32    i, index, n;
l_float32  x, y;
PTA       *ptad;

    PROCNAME("ptaSortByIndex");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);
    if (!naindex)
        return (PTA *)ERROR_PTR("naindex not defined", procName, NULL);

        /* Build up sorted pta using sort index */
    n = numaGetCount(naindex);
    if ((ptad = ptaCreate(n)) == NULL)
        return (PTA *)ERROR_PTR("ptad not made", procName, NULL);
    for (i = 0; i < n; i++) {
        numaGetIValue(naindex, i, &index);
        ptaGetPt(ptas, index, &x, &y);
        ptaAddPt(ptad, x, y);
    }

    return ptad;
}


/*!
 *  ptaaSortByIndex()
 *
 *      Input:  ptaas
 *              naindex (na that maps from the new ptaa to the input ptaa)
 *      Return: ptaad (sorted), or null on error
 */
PTAA *
ptaaSortByIndex(PTAA  *ptaas,
                NUMA  *naindex)
{
l_int32  i, n, index;
PTA     *pta;
PTAA    *ptaad;

    PROCNAME("ptaaSortByIndex");

    if (!ptaas)
        return (PTAA *)ERROR_PTR("ptaas not defined", procName, NULL);
    if (!naindex)
        return (PTAA *)ERROR_PTR("naindex not defined", procName, NULL);

    n = ptaaGetCount(ptaas);
    if (numaGetCount(naindex) != n)
        return (PTAA *)ERROR_PTR("numa and ptaa sizes differ", procName, NULL);
    ptaad = ptaaCreate(n);
    for (i = 0; i < n; i++) {
        numaGetIValue(naindex, i, &index);
        pta = ptaaGetPta(ptaas, index, L_COPY);
        ptaaAddPta(ptaad, pta, L_INSERT);
    }

    return ptaad;
}

/*---------------------------------------------------------------------*
 *                 Union and intersection by aset (rbtree)             *
 *---------------------------------------------------------------------*/
/*!
 *  ptaUnionByAset()
 *
 *      Input:  pta1, pta2
 *      Return: ptad (with the union of the set of points), or null on error
 *
 *  Notes:
 *      (1) See sarrayRemoveDupsByAset() for the approach.
 *      (2) The key is a 64-bit hash from the (x,y) pair.
 *      (3) This is slower than ptaUnionByHash(), mostly because of the
 *          nlogn sort to build up the rbtree.  Do not use for large
 *          numbers of points (say, > 1M).
 *      (4) The *Aset() functions use the sorted l_Aset, which is just
 *          an rbtree in disguise.
 */
PTA *
ptaUnionByAset(PTA  *pta1,
               PTA  *pta2)
{
PTA  *pta3, *ptad;

    PROCNAME("ptaUnionByAset");

    if (!pta1)
        return (PTA *)ERROR_PTR("pta1 not defined", procName, NULL);
    if (!pta2)
        return (PTA *)ERROR_PTR("pta2 not defined", procName, NULL);

        /* Join */
    pta3 = ptaCopy(pta1);
    ptaJoin(pta3, pta2, 0, -1);

        /* Eliminate duplicates */
    ptad = ptaRemoveDupsByAset(pta3);
    ptaDestroy(&pta3);
    return ptad;
}


/*!
 *  ptaRemoveDupsByAset()
 *
 *      Input:  ptas (assumed to be integer values)
 *      Return: ptad (with duplicates removed), or null on error
 *
 *  Notes:
 *      (1) This is slower than ptaRemoveDupsByHash(), mostly because
 *          of the nlogn sort to build up the rbtree.  Do not use for
 *          large numbers of points (say, > 1M).
 */
PTA *
ptaRemoveDupsByAset(PTA  *ptas)
{
l_int32   i, n, x, y;
PTA      *ptad;
l_uint64  hash;
L_ASET   *set;
RB_TYPE   key;

    PROCNAME("ptaRemoveDupsByAset");

    if (!ptas)
        return (PTA *)ERROR_PTR("ptas not defined", procName, NULL);

    set = l_asetCreate(L_UINT_TYPE);
    n = ptaGetCount(ptas);
    ptad = ptaCreate(n);
    for (i = 0; i < n; i++) {
        ptaGetIPt(ptas, i, &x, &y);
        l_hashPtToUint64(x, y, &hash);
        key.utype = hash;
        if (!l_asetFind(set, key)) {
            ptaAddPt(ptad, x, y);
            l_asetInsert(set, key);
        }
    }

    l_asetDestroy(&set);
    return ptad;
}


/*!
 *  ptaIntersectionByAset()
 *
 *      Input:  pta1, pta2
 *      Return: ptad (intersection of the point sets), or null on error
 *
 *  Notes:
 *      (1) See sarrayIntersectionByAset() for the approach.
 *      (2) The key is a 64-bit hash from the (x,y) pair.
 *      (3) This is slower than ptaIntersectionByHash(), mostly because
 *          of the nlogn sort to build up the rbtree.  Do not use for
 *          large numbers of points (say, > 1M).
 */
PTA *
ptaIntersectionByAset(PTA  *pta1,
                      PTA  *pta2)
{
l_int32   n1, n2, i, n, x, y;
l_uint64  hash;
L_ASET   *set1, *set2;
RB_TYPE   key;
PTA      *pta_small, *pta_big, *ptad;

    PROCNAME("ptaIntersectionByAset");

    if (!pta1)
        return (PTA *)ERROR_PTR("pta1 not defined", procName, NULL);
    if (!pta2)
        return (PTA *)ERROR_PTR("pta2 not defined", procName, NULL);

        /* Put the elements of the biggest array into a set */
    n1 = ptaGetCount(pta1);
    n2 = ptaGetCount(pta2);
    pta_small = (n1 < n2) ? pta1 : pta2;   /* do not destroy pta_small */
    pta_big = (n1 < n2) ? pta2 : pta1;   /* do not destroy pta_big */
    set1 = l_asetCreateFromPta(pta_big);

        /* Build up the intersection of points */
    ptad = ptaCreate(0);
    n = ptaGetCount(pta_small);
    set2 = l_asetCreate(L_UINT_TYPE);
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta_small, i, &x, &y);
        l_hashPtToUint64(x, y, &hash);
        key.utype = hash;
        if (l_asetFind(set1, key) && !l_asetFind(set2, key)) {
            ptaAddPt(ptad, x, y);
            l_asetInsert(set2, key);
        }
    }

    l_asetDestroy(&set1);
    l_asetDestroy(&set2);
    return ptad;
}


/*!
 *  l_asetCreateFromPta()
 *
 *      Input:  pta
 *      Return: set (using a 64-bit hash of (x,y) as the key)
 */
L_ASET *
l_asetCreateFromPta(PTA  *pta)
{
l_int32   i, n, x, y;
l_uint64  hash;
L_ASET   *set;
RB_TYPE   key;

    PROCNAME("l_asetCreateFromPta");

    if (!pta)
        return (L_ASET *)ERROR_PTR("pta not defined", procName, NULL);

    set = l_asetCreate(L_UINT_TYPE);
    n = ptaGetCount(pta);
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &x, &y);
        l_hashPtToUint64(x, y, &hash);
        key.utype = hash;
        l_asetInsert(set, key);
    }

    return set;
}


/*---------------------------------------------------------------------*
 *                    Union and intersection by hash                   *
 *---------------------------------------------------------------------*/
/*!
 *  ptaUnionByHash()
 *
 *      Input:  pta1, pta2
 *      Return: ptad (with the union of the set of points), or null on error
 *
 *  Notes:
 *      (1) This is faster than ptaUnionByAset(), because the
 *          bucket lookup is O(n).  It should be used if the pts are
 *          integers (e.g., representing pixel positions).
 */
PTA *
ptaUnionByHash(PTA  *pta1,
               PTA  *pta2)
{
PTA  *pta3, *ptad;

    PROCNAME("ptaUnionByHash");

    if (!pta1)
        return (PTA *)ERROR_PTR("pta1 not defined", procName, NULL);
    if (!pta2)
        return (PTA *)ERROR_PTR("pta2 not defined", procName, NULL);

        /* Join */
    pta3 = ptaCopy(pta1);
    ptaJoin(pta3, pta2, 0, -1);

        /* Eliminate duplicates */
    ptaRemoveDupsByHash(pta3, &ptad, NULL);
    ptaDestroy(&pta3);
    return ptad;
}


/*!
 *  ptaRemoveDupsByHash()
 *
 *      Input:  ptas (assumed to be integer values)
 *              &ptad (<return> unique set of pts; duplicates removed)
 *              &dahash (<optional return> dnahash used for lookup)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Generates a pta with unique values.
 *      (2) The dnahash is built up with ptad to assure uniqueness.
 *          It can be used to find if a point is in the set:
 *              ptaFindPtByHash(ptad, dahash, x, y, &index)
 *      (3) The hash of the (x,y) location is simple and fast.  It scales
 *          up with the number of buckets to insure a fairly random
 *          bucket selection for adjacent points.
 *      (4) A Dna is used rather than a Numa because we need accurate
 *          representation of 32-bit integers that are indices into ptas.
 *          Integer --> float --> integer conversion makes errors for
 *          integers larger than 10M.
 *      (5) This is faster than ptaRemoveDupsByAset(), because the
 *          bucket lookup is O(n), although there is a double-loop
 *          lookup within the dna in each bucket.
 */
l_int32
ptaRemoveDupsByHash(PTA         *ptas,
                    PTA        **pptad,
                    L_DNAHASH  **pdahash)
{
l_int32     i, n, index, items, x, y;
l_uint32    nsize;
l_uint64    key;
l_float64   val;
PTA        *ptad;
L_DNAHASH  *dahash;

    PROCNAME("ptaRemoveDupsByHash");

    if (pdahash) *pdahash = NULL;
    if (!pptad)
        return ERROR_INT("&ptad not defined", procName, 1);
    *pptad = NULL;
    if (!ptas)
        return ERROR_INT("ptas not defined", procName, 1);

    n = ptaGetCount(ptas);
    findNextLargerPrime(n / 20, &nsize);  /* buckets in hash table */
    dahash = l_dnaHashCreate(nsize, 8);
    ptad = ptaCreate(n);
    *pptad = ptad;
    for (i = 0, items = 0; i < n; i++) {
        ptaGetIPt(ptas, i, &x, &y);
        ptaFindPtByHash(ptad, dahash, x, y, &index);
        if (index < 0) {  /* not found */
            l_hashPtToUint64Fast(nsize, x, y, &key);
            l_dnaHashAdd(dahash, key, (l_float64)items);
            ptaAddPt(ptad, x, y);
            items++;
        }
    }

    if (pdahash)
        *pdahash = dahash;
    else
        l_dnaHashDestroy(&dahash);
    return 0;
}


/*!
 *  ptaIntersectionByHash()
 *
 *      Input:  pta1, pta2
 *      Return: ptad (intersection of the point sets), or null on error
 *
 *  Notes:
 *      (1) This is faster than ptaIntersectionByAset(), because the
 *          bucket lookup is O(n).  It should be used if the pts are
 *          integers (e.g., representing pixel positions).
 */
PTA *
ptaIntersectionByHash(PTA  *pta1,
                      PTA  *pta2)
{
l_int32     n1, n2, nsmall, i, x, y, index1, index2;
l_uint32    nsize2;
l_uint64    key;
L_DNAHASH  *dahash1, *dahash2;
PTA        *pta_small, *pta_big, *ptad;

    PROCNAME("ptaIntersectionByHash");

    if (!pta1)
        return (PTA *)ERROR_PTR("pta1 not defined", procName, NULL);
    if (!pta2)
        return (PTA *)ERROR_PTR("pta2 not defined", procName, NULL);

        /* Put the elements of the biggest pta into a dnahash */
    n1 = ptaGetCount(pta1);
    n2 = ptaGetCount(pta2);
    pta_small = (n1 < n2) ? pta1 : pta2;   /* do not destroy pta_small */
    pta_big = (n1 < n2) ? pta2 : pta1;   /* do not destroy pta_big */
    dahash1 = l_dnaHashCreateFromPta(pta_big);

        /* Build up the intersection of points.  Add to ptad
         * if the point is in pta_big (using dahash1) but hasn't
         * yet been seen in the traversal of pta_small (using dahash2). */
    ptad = ptaCreate(0);
    nsmall = ptaGetCount(pta_small);
    findNextLargerPrime(nsmall / 20, &nsize2);  /* buckets in hash table */
    dahash2 = l_dnaHashCreate(nsize2, 0);
    for (i = 0; i < nsmall; i++) {
        ptaGetIPt(pta_small, i, &x, &y);
        ptaFindPtByHash(pta_big, dahash1, x, y, &index1);
        if (index1 >= 0) {  /* found */
            ptaFindPtByHash(pta_small, dahash2, x, y, &index2);
            if (index2 == -1) {  /* not found */
                ptaAddPt(ptad, x, y);
                l_hashPtToUint64Fast(nsize2, x, y, &key);
                l_dnaHashAdd(dahash2, key, (l_float64)i);
            }
        }
    }

    l_dnaHashDestroy(&dahash1);
    l_dnaHashDestroy(&dahash2);
    return ptad;
}


/*!
 *  ptaFindPtByHash()
 *
 *      Input:  pta
 *              dahash (built from pta)
 *              x, y  (arbitrary points)
 *              &index (<return> index into pta if (x,y) is in pta;
 *                      -1 otherwise)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) Fast lookup in dnaHash associated with a pta, to see if a
 *          random point (x,y) is already stored in the hash table.
 */
l_int32
ptaFindPtByHash(PTA        *pta,
                L_DNAHASH  *dahash,
                l_int32     x,
                l_int32     y,
                l_int32    *pindex)
{
l_int32   i, nbuckets, nvals, index, xi, yi;
l_uint64  key;
L_DNA    *da;

    PROCNAME("ptaFindPtByHash");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = -1;
    if (!pta)
        return ERROR_INT("pta not defined", procName, 1);
    if (!dahash)
        return ERROR_INT("dahash not defined", procName, 1);

    nbuckets = l_dnaHashGetCount(dahash);
    l_hashPtToUint64Fast(nbuckets, x, y, &key);
    da = l_dnaHashGetDna(dahash, key, L_NOCOPY);
    if (!da) return 0;

        /* Run through the da, looking for this point */
    nvals = l_dnaGetCount(da);
    for (i = 0; i < nvals; i++) {
        l_dnaGetIValue(da, i, &index);
        ptaGetIPt(pta, index, &xi, &yi);
        if (x == xi && y == yi) {
            *pindex = index;
            return 0;
        }
    }

    return 0;
}


/*!
 *  l_dnaHashCreateFromPta()
 *
 *      Input:  pta
 *      Return: dahash, or null on error
 */
L_DNAHASH *
l_dnaHashCreateFromPta(PTA  *pta)
{
l_int32     i, n, x, y;
l_uint32    nsize;
l_uint64    key;
L_DNAHASH  *dahash;

    PROCNAME("l_dnaHashCreateFromPta");

        /* Build up dnaHash of indices, hashed by a key that is
         * a large linear combination of x and y values designed to
         * randomize the key.  Having about 20 pts in each bucket is
         * roughly optimal for speed for large sets. */
    n = ptaGetCount(pta);
    findNextLargerPrime(n / 20, &nsize);  /* buckets in hash table */
/*    fprintf(stderr, "Prime used: %d\n", nsize); */

        /* Add each point, using the hash as key and the index into
         * @ptas as the value.  Storing the index enables operations
         * that check for duplicates. */
    dahash = l_dnaHashCreate(nsize, 8);
    for (i = 0; i < n; i++) {
        ptaGetIPt(pta, i, &x, &y);
        l_hashPtToUint64Fast(nsize, x, y, &key);
        l_dnaHashAdd(dahash, key, (l_float64)i);
    }

    return dahash;
}

