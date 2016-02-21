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
 *  dnahash.c
 *
 *      DnaHash creation, destruction
 *          L_DNAHASH   *l_dnaHashCreate()
 *          void        *l_dnaHashDestroy()
 *
 *      DnaHash: Accessors and modifiers                      *
 *          l_int32     *l_dnaHashGetCount()
 *          l_int32     *l_dnaHashGetTotalCount()
 *          L_DNA       *l_dnaHashGetDna()
 *          void        *l_dnaHashAdd()
 *
 *      DnaHash: Operations on Dna
 *          L_DNAHASH   *l_dnaHashCreateFromDna()
 *          l_int32      l_dnaRemoveDupsByHash()
 *          l_int32      l_dnaMakeHistoByHash()
 *          L_DNA       *l_dnaIntersectionByHash()
 *          l_int32      l_dnaFindValByHash()
 *
 *    (1) The DnaHash is an array of Dna.  It is useful for fast
 *        storage and lookup for sets and maps.  If the set or map
 *        is on a Dna itself, the hash is a simple function that
 *        maps a double to an int64; otherwise the function will
 *        map a string or a (x,y) point to an int64.  The result of
 *        the map is the "key", which is then used with the mod
 *        function to select which Dna array is to be used.  The
 *        number of arrays in a DnaHash should be a prime number.
 *        If there are N items, we set up the DnaHash array to have
 *        approximately N/20 Dna, so the average size of these arrays
 *        will be about 20 when fully populated.  The number 20 was
 *        found empirically to be in a broad maximum of efficiency.
 *    (2) Note that the word "hash" is overloaded.  There are actually
 *        two hashing steps: the first hashes the object to an int64,
 *        called the "key", and the second uses the mod function to
 *        "hash" the "key" to the index of a particular Dna in the
 *        DnaHash array.
 *    (3) Insertion and lookup time for DnaHash is O(1).  Hash collisions
 *        are easily handled (we expect an average of 20 for each key),
 *        so we can use simple (fast) hash functions: we deal with
 *        collisions by storing an array for each hash key.
 *        This can be contrasted with using rbtree for sets and
 *        maps, where insertion and lookup are O(logN) and hash functions
 *        are slower because they must be good enough (i.e, random
 *        enough with arbitrary input) to avoid collisions.
 *    (4) Hash functions that map points, strings and floats to int64
 *        are given in utils.c.
 *    (5) The use of the DnaHash (and RBTree) with strings and
 *        (x,y) points can be found in string2.c and ptafunc2.c, rsp.
 *        This file has similar hash set functions, using DnaHash on
 *        two input Dna, for removing duplicates and finding the
 *        intersection.  It also uses DnaHash as a hash map to find
 *        a histogram of counts from an input Dna.
 *    (6) Comparisons in running time, between DnaHash and RBTree, for
 *        large sets of strings and points, are given in prog/hashtest.c.
 *    (7) This is a very simple implementation, that expects that you
 *        know approximately (i.e., within a factor of 2 or 3) how many
 *        items are to be stored when you initialize the DnaHash.
 *        (It would be nice to modify the l_dnaHashAdd() function
 *        to increase the number of bins when the average occupation
 *        exceeds 40 or so.)
 *    (8) Useful rule of thumb:
 *        For a random hashing function (say, from strings to int64),
 *        the probability of a collision increases as N^2 for N much
 *        less than 2^32.  The quadratic behavior switches over to
 *        approaching 1.0 around 2^32.  So, for example, if you have
 *        10^7 strings, the probability of a collision using an int64
 *        key is about (10^7/10^9)^2 ~ 10^-4.  For a million strings
 *        you don't need to worry about collisons (10-6 probability),
 *        and can use the RBTree (sorting) implementation with confidence.
 */

#include "allheaders.h"

/*--------------------------------------------------------------------------*
 *                     Dna hash: Creation and destruction                   *
 *--------------------------------------------------------------------------*/
/*!
 *  l_dnaHashCreate()
 *
 *      Input: nbuckets (the number of buckets in the hash table,
 *                       which should be prime.)
 *             initsize (initial size of each allocated numa; 0 for default)
 *      Return: ptr to new dnahash, or null on error
 *
 *  Notes:
 *      (1) Actual dna are created only as required by l_dnaHashAdd()
 */
L_DNAHASH *
l_dnaHashCreate(l_int32  nbuckets,
                l_int32  initsize)
{
L_DNAHASH  *dahash;

    PROCNAME("l_dnaHashCreate");

    if (nbuckets <= 0)
        return (L_DNAHASH *)ERROR_PTR("negative hash size", procName, NULL);
    if ((dahash = (L_DNAHASH *)LEPT_CALLOC(1, sizeof(L_DNAHASH))) == NULL)
        return (L_DNAHASH *)ERROR_PTR("dahash not made", procName, NULL);
    if ((dahash->dna = (L_DNA **)LEPT_CALLOC(nbuckets, sizeof(L_DNA *)))
        == NULL) {
        LEPT_FREE(dahash);
        return (L_DNAHASH *)ERROR_PTR("dna ptr array not made", procName, NULL);
    }

    dahash->nbuckets = nbuckets;
    dahash->initsize = initsize;
    return dahash;
}


/*!
 *  l_dnaHashDestroy()
 *
 *      Input:  &dahash (<to be nulled, if it exists>)
 *      Return: void
 */
void
l_dnaHashDestroy(L_DNAHASH **pdahash)
{
L_DNAHASH  *dahash;
l_int32    i;

    PROCNAME("l_dnaHashDestroy");

    if (pdahash == NULL) {
        L_WARNING("ptr address is NULL!\n", procName);
        return;
    }

    if ((dahash = *pdahash) == NULL)
        return;

    for (i = 0; i < dahash->nbuckets; i++)
        l_dnaDestroy(&dahash->dna[i]);
    LEPT_FREE(dahash->dna);
    LEPT_FREE(dahash);
    *pdahash = NULL;
}


/*--------------------------------------------------------------------------*
 *                   Dna hash: Accessors and modifiers                      *
 *--------------------------------------------------------------------------*/
/*!
 *  l_dnaHashGetCount()
 *
 *      Input:  dahash
 *      Return: nbuckets (allocated, or 0 on error)
 */
l_int32
l_dnaHashGetCount(L_DNAHASH  *dahash)
{
l_int32  nbuckets;

    PROCNAME("l_dnaHashGetCount");

    if (!dahash)
        return ERROR_INT("dahash not defined", procName, 0);
    return dahash->nbuckets;
}


/*!
 *  l_dnaHashGetTotalCount()
 *
 *      Input:  dahash
 *      Return: n (number of numbers in all dna, or 0 on error)
 */
l_int32
l_dnaHashGetTotalCount(L_DNAHASH  *dahash)
{
l_int32  nbuckets, i, n;
L_DNA   *da;

    PROCNAME("l_dnaHashGetTotalCount");

    if (!dahash)
        return ERROR_INT("dahash not defined", procName, 0);

    for (i = 0, n = 0; i < dahash->nbuckets; i++) {
        da = l_dnaHashGetDna(dahash, i, L_NOCOPY);
        if (da)
            n += l_dnaGetCount(da);
    }

    return n;
}


/*!
 *  l_dnaHashGetDna()
 *
 *      Input:  dahash
 *              key  (key to be hashed into a bucket number)
 *              copyflag (L_NOCOPY, L_COPY, L_CLONE)
 *      Return: ptr to numa
 */
L_DNA *
l_dnaHashGetDna(L_DNAHASH  *dahash,
                l_uint64    key,
                l_int32     copyflag)
{
l_int32  bucket;
L_DNA   *da;

    PROCNAME("l_dnaHashGetDna");

    if (!dahash)
        return (L_DNA *)ERROR_PTR("dahash not defined", procName, NULL);
    bucket = key % dahash->nbuckets;
    da = dahash->dna[bucket];
    if (da) {
        if (copyflag == L_NOCOPY)
            return da;
        else if (copyflag == L_COPY)
            return l_dnaCopy(da);
        else
            return l_dnaClone(da);
    }
    else
        return NULL;
}


/*!
 *  l_dnaHashAdd()
 *
 *      Input:  dahash
 *              key  (key to be hashed into a bucket number)
 *              value  (float value to be appended to the specific dna)
 *      Return: 0 if OK; 1 on error
 */
l_int32
l_dnaHashAdd(L_DNAHASH  *dahash,
             l_uint64    key,
             l_float64   value)
{
l_int32  bucket;
L_DNA   *da;

    PROCNAME("l_dnaHashAdd");

    if (!dahash)
        return ERROR_INT("dahash not defined", procName, 1);
    bucket = key % dahash->nbuckets;
    da = dahash->dna[bucket];
    if (!da) {
        if ((da = l_dnaCreate(dahash->initsize)) == NULL)
            return ERROR_INT("da not made", procName, 1);
        dahash->dna[bucket] = da;
    }
    l_dnaAddNumber(da, value);
    return 0;
}


/*--------------------------------------------------------------------------*
 *                      DnaHash: Operations on Dna                          *
 *--------------------------------------------------------------------------*/
/*!
 *  l_dnaHashCreateFromDna()
 *
 *      Input:  da
 *      Return: dahash if OK; 1 on error
 *
 *  Notes:
 *      (1) The values stored in the @dahash are indices into @da;
 *          @dahash has no use without @da.
 */
L_DNAHASH *
l_dnaHashCreateFromDna(L_DNA  *da)
{
l_int32     i, n;
l_uint32    nsize;
l_uint64    key;
l_float64   val;
L_DNAHASH  *dahash;

    PROCNAME("l_dnaHashCreateFromDna");

    if (!da)
        return (L_DNAHASH *)ERROR_PTR("da not defined", procName, NULL);

    n = l_dnaGetCount(da);
    findNextLargerPrime(n / 20, &nsize);  /* buckets in hash table */

    dahash = l_dnaHashCreate(nsize, 8);
    for (i = 0; i < n; i++) {
        l_dnaGetDValue(da, i, &val);
        l_hashFloat64ToUint64(nsize, val, &key);
        l_dnaHashAdd(dahash, key, (l_float64)i);
    }

    return dahash;
}


/*!
 *  l_dnaRemoveDupsByHash()
 *
 *      Input:  das
 *              &dad (<return> hash set)
 *              &dahash (<optional return> dnahash used for lookup)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Generates a dna with unique values.
 *      (2) The dnahash is built up with dad to assure uniqueness.
 *          It can be used to find if an element is in the set:
 *              l_dnaFindValByHash(dad, dahash, val, &index)
 */
l_int32
l_dnaRemoveDupsByHash(L_DNA       *das,
                      L_DNA      **pdad,
                      L_DNAHASH  **pdahash)
{
l_int32     i, n, index, items;
l_uint32    nsize;
l_uint64    key;
l_float64   val;
L_DNA      *dad;
L_DNAHASH  *dahash;

    PROCNAME("l_dnaRemoveDupsByHash");

    if (pdahash) *pdahash = NULL;
    if (!pdad)
        return ERROR_INT("&dad not defined", procName, 1);
    *pdad = NULL;
    if (!das)
        return ERROR_INT("das not defined", procName, 1);

    n = l_dnaGetCount(das);
    findNextLargerPrime(n / 20, &nsize);  /* buckets in hash table */
    dahash = l_dnaHashCreate(nsize, 8);
    dad = l_dnaCreate(n);
    *pdad = dad;
    for (i = 0, items = 0; i < n; i++) {
        l_dnaGetDValue(das, i, &val);
        l_dnaFindValByHash(dad, dahash, val, &index);
        if (index < 0) {  /* not found */
            l_hashFloat64ToUint64(nsize, val, &key);
            l_dnaHashAdd(dahash, key, (l_float64)items);
            l_dnaAddNumber(dad, val);
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
 *  l_dnaMakeHistoByHash()
 *
 *      Input:  das
 *              &dahash (<return> hash map: val --> index)
 *              &dav (<return> array of values: index --> val)
 *              &dac (<return> histo array of counts: index --> count)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Generates and returns a dna of occurrences (histogram),
 *          an aligned dna of values, and an associated hashmap.
 *          The hashmap takes @dav and a value, and points into the
 *          histogram in @dac.
 *      (2) The dna of values, @dav, is aligned with the histogram @dac,
 *          and is needed for fast lookup.  It is a hash set, because
 *          the values are unique.
 *      (3) Lookup is simple:
 *              l_dnaFindValByHash(dav, dahash, val, &index);
 *              if (index >= 0)
 *                  l_dnaGetIValue(dac, index, &icount);
 *              else
 *                  icount = 0;
 */
l_int32
l_dnaMakeHistoByHash(L_DNA       *das,
                     L_DNAHASH  **pdahash,
                     L_DNA      **pdav,
                     L_DNA      **pdac)
{
l_int32     i, n, nitems, index, count;
l_uint32    nsize;
l_uint64    key;
l_float64   val;
L_DNA      *dac, *dav;
L_DNAHASH  *dahash;

    PROCNAME("l_dnaMakeHistoByHash");

    if (pdahash) *pdahash = NULL;
    if (pdac) *pdac = NULL;
    if (pdav) *pdav = NULL;
    if (!pdahash || !pdac || !pdav)
        return ERROR_INT("&dahash, &dac, &dav not all defined", procName, 1);
    if (!das)
        return ERROR_INT("das not defined", procName, 1);
    if ((n = l_dnaGetCount(das)) == 0)
        return ERROR_INT("no data in das", procName, 1);

    findNextLargerPrime(n / 20, &nsize);  /* buckets in hash table */
    dahash = l_dnaHashCreate(nsize, 8);
    dac = l_dnaCreate(n);  /* histogram */
    dav = l_dnaCreate(n);  /* the values */
    for (i = 0, nitems = 0; i < n; i++) {
        l_dnaGetDValue(das, i, &val);
            /* Is this value already stored in dav? */
        l_dnaFindValByHash(dav, dahash, val, &index);
        if (index >= 0) {  /* found */
            l_dnaGetIValue(dac, (l_float64)index, &count);
            l_dnaSetValue(dac, (l_float64)index, count + 1);
        } else {  /* not found */
            l_hashFloat64ToUint64(nsize, val, &key);
            l_dnaHashAdd(dahash, key, (l_float64)nitems);
            l_dnaAddNumber(dav, val);
            l_dnaAddNumber(dac, 1);
            nitems++;
        }
    }

    *pdahash = dahash;
    *pdac = dac;
    *pdav = dav;
    return 0;
}


/*!
 *  l_dnaIntersectionByHash()
 *
 *      Input:  da1, da2
 *      Return: dad (intersection of the number arrays), or null on error
 *
 *  Notes:
 *      (1) This uses the same method for building the intersection set
 *          as ptaIntersectionByHash() and sarrayIntersectionByHash().
 */
L_DNA *
l_dnaIntersectionByHash(L_DNA  *da1,
                        L_DNA  *da2)
{
l_int32     n1, n2, nsmall, nbuckets, i, index1, index2;
l_uint32    nsize2;
l_uint64    key;
l_float64   val;
L_DNAHASH  *dahash1, *dahash2;
L_DNA      *da_small, *da_big, *dad;

    PROCNAME("l_dnaIntersectionByHash");

    if (!da1)
        return (L_DNA *)ERROR_PTR("da1 not defined", procName, NULL);
    if (!da2)
        return (L_DNA *)ERROR_PTR("da2 not defined", procName, NULL);

        /* Put the elements of the biggest array into a dnahash */
    n1 = l_dnaGetCount(da1);
    n2 = l_dnaGetCount(da2);
    da_small = (n1 < n2) ? da1 : da2;   /* do not destroy da_small */
    da_big = (n1 < n2) ? da2 : da1;   /* do not destroy da_big */
    dahash1 = l_dnaHashCreateFromDna(da_big);

        /* Build up the intersection of numbers.  Add to @dad
         * if the number is in da_big (using dahash1) but hasn't
         * yet been seen in the traversal of da_small (using dahash2). */
    dad = l_dnaCreate(0);
    nsmall = l_dnaGetCount(da_small);
    findNextLargerPrime(nsmall / 20, &nsize2);  /* buckets in hash table */
    dahash2 = l_dnaHashCreate(nsize2, 0);
    nbuckets = l_dnaHashGetCount(dahash2);
    for (i = 0; i < nsmall; i++) {
        l_dnaGetDValue(da_small, i, &val);
        l_dnaFindValByHash(da_big, dahash1, val, &index1);
        if (index1 >= 0) {  /* found */
            l_dnaFindValByHash(da_small, dahash2, val, &index2);
            if (index2 == -1) {  /* not found */
                l_dnaAddNumber(dad, val);
                l_hashFloat64ToUint64(nbuckets, val, &key);
                l_dnaHashAdd(dahash2, key, (l_float64)i);
            }
        }
    }

    l_dnaHashDestroy(&dahash1);
    l_dnaHashDestroy(&dahash2);
    return dad;
}


/*!
 *  l_dnaFindValByHash()
 *
 *      Input:  da
 *              dahash (containing indices into @da)
 *              val  (searching for this number in @da)
 *              &index (<return> index into da if found; -1 otherwise)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Algo: hash @val into a key; hash the key to get the dna
 *                in @dahash (that holds indices into @da); traverse
 *                the dna of indices looking for @val in @da.
 */
l_int32
l_dnaFindValByHash(L_DNA      *da,
                   L_DNAHASH  *dahash,
                   l_float64   val,
                   l_int32    *pindex)
{
l_int32    i, nbuckets, nvals, indexval;
l_float64  vali;
l_uint64   key;
L_DNA     *da1;

    PROCNAME("l_dnaFindValByHash");

    if (!pindex)
        return ERROR_INT("&index not defined", procName, 1);
    *pindex = -1;
    if (!da)
        return ERROR_INT("da not defined", procName, 1);
    if (!dahash)
        return ERROR_INT("dahash not defined", procName, 1);

    nbuckets = l_dnaHashGetCount(dahash);
    l_hashFloat64ToUint64(nbuckets, val, &key);
    da1 = l_dnaHashGetDna(dahash, key, L_NOCOPY);
    if (!da1) return 0;

        /* Run through da1, looking for this @val */
    nvals = l_dnaGetCount(da1);
    for (i = 0; i < nvals; i++) {
        l_dnaGetIValue(da1, i, &indexval);
        l_dnaGetDValue(da, indexval, &vali);
        if (val == vali) {
            *pindex = indexval;
            return 0;
        }
    }

    return 0;
}


