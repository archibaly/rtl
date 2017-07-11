#ifndef _RTL_HASH_H_
#define _RTL_HASH_H_

#include <string.h>   /* memcmp, memset, strlen */
#include <stddef.h>   /* ptrdiff_t */
#include <stdlib.h>   /* exit */
#include <stdint.h>   /* uint8_t, uint32_t */

#define DECLTYPE(x) (__typeof(x))
#define DECLTYPE_ASSIGN(dst,src)                                                 \
do {                                                                             \
  (dst) = DECLTYPE(dst)(src);                                                    \
} while (0)

#ifndef rtl_hash_fatal
#define rtl_hash_fatal(msg) exit(1)        /* fatal error (out of memory,etc) */
#endif
#ifndef rtl_hash_malloc
#define rtl_hash_malloc(sz) malloc(sz)      /* malloc fcn                      */
#endif
#ifndef rtl_hash_free
#define rtl_hash_free(ptr,sz) free(ptr)     /* free fcn                        */
#endif
#ifndef rtl_hash_bzero
#define rtl_hash_bzero(a,n) memset(a,0,n)
#endif
#ifndef rtl_hash_memcmp
#define rtl_hash_memcmp(a,b,n) memcmp(a,b,n)
#endif
#ifndef rtl_hash_strlen
#define rtl_hash_strlen(s) strlen(s)
#endif

#ifndef rtl_hash_noexpand_fyi
#define rtl_hash_noexpand_fyi(tbl)          /* can be defined to log noexpand  */
#endif
#ifndef rtl_hash_expand_fyi
#define rtl_hash_expand_fyi(tbl)            /* can be defined to log expands   */
#endif

/* initial number of buckets */
#define RTL_HASH_INITIAL_NUM_BUCKETS 32U     /* initial number of buckets        */
#define RTL_HASH_INITIAL_NUM_BUCKETS_LOG2 5U /* lg2 of initial number of buckets */
#define RTL_HASH_BKT_CAPACITY_THRESH 10U     /* expand when bucket count reaches */

/* calculate the element whose hash handle address is hhp */
#define RTL_ELMT_FROM_HH(tbl,hhp) ((void*)(((char*)(hhp)) - ((tbl)->hho)))
/* calculate the hash handle from element address elp */
#define RTL_HH_FROM_ELMT(tbl,elp) ((rtl_hash_handle_t *)(((char*)(elp)) + ((tbl)->hho)))

#define RTL_HASH_VALUE(keyptr,keylen,hashv)                                        \
do {                                                                               \
  RTL_HASH_FCN(keyptr, keylen, hashv);                                             \
} while (0)

#define RTL_HASH_FIND_BYHASHVALUE(hh,head,keyptr,keylen,hashval,out)               \
do {                                                                               \
  (out) = NULL;                                                                    \
  if (head) {                                                                      \
    unsigned _hf_bkt;                                                              \
    RTL_HASH_TO_BKT(hashval, (head)->hh.tbl->num_buckets, _hf_bkt);                \
    if (RTL_HASH_BLOOM_TEST((head)->hh.tbl, hashval) != 0) {                       \
      RTL_HASH_FIND_IN_BKT((head)->hh.tbl, hh, (head)->hh.tbl->buckets[ _hf_bkt ], keyptr, keylen, hashval, out); \
    }                                                                              \
  }                                                                                \
} while (0)

#define RTL_HASH_FIND(hh,head,keyptr,keylen,out)                                   \
do {                                                                               \
  unsigned _hf_hashv;                                                              \
  RTL_HASH_VALUE(keyptr, keylen, _hf_hashv);                                       \
  RTL_HASH_FIND_BYHASHVALUE(hh, head, keyptr, keylen, _hf_hashv, out);             \
} while (0)

#ifdef RTL_HASH_BLOOM
#define RTL_HASH_BLOOM_BITLEN (1UL << RTL_HASH_BLOOM)
#define RTL_HASH_BLOOM_BYTELEN (RTL_HASH_BLOOM_BITLEN/8UL) + (((RTL_HASH_BLOOM_BITLEN%8UL)!=0UL) ? 1UL : 0UL)
#define RTL_HASH_BLOOM_MAKE(tbl)                                                    \
do {                                                                                \
  (tbl)->bloom_nbits = RTL_HASH_BLOOM;                                              \
  (tbl)->bloom_bv = (uint8_t*)rtl_hash_malloc(RTL_HASH_BLOOM_BYTELEN);              \
  if (!(tbl)->bloom_bv) {                                                           \
    rtl_hash_fatal("out of memory");                                                \
  }                                                                                 \
  rtl_hash_bzero((tbl)->bloom_bv, RTL_HASH_BLOOM_BYTELEN);                          \
  (tbl)->bloom_sig = RTL_HASH_BLOOM_SIGNATURE;                                      \
} while (0)

#define RTL_HASH_BLOOM_FREE(tbl)                                                    \
do {                                                                                \
  rtl_hash_free((tbl)->bloom_bv, RTL_HASH_BLOOM_BYTELEN);                           \
} while (0)

#define RTL_HASH_BLOOM_BITSET(bv,idx) (bv[(idx)/8U] |= (1U << ((idx)%8U)))
#define RTL_HASH_BLOOM_BITTEST(bv,idx) (bv[(idx)/8U] & (1U << ((idx)%8U)))

#define RTL_HASH_BLOOM_ADD(tbl,hashv)                                                \
  RTL_HASH_BLOOM_BITSET((tbl)->bloom_bv, (hashv & (uint32_t)((1UL << (tbl)->bloom_nbits) - 1U)))

#define RTL_HASH_BLOOM_TEST(tbl,hashv)                                               \
  RTL_HASH_BLOOM_BITTEST((tbl)->bloom_bv, (hashv & (uint32_t)((1UL << (tbl)->bloom_nbits) - 1U)))

#else
#define RTL_HASH_BLOOM_MAKE(tbl)
#define RTL_HASH_BLOOM_FREE(tbl)
#define RTL_HASH_BLOOM_ADD(tbl,hashv)
#define RTL_HASH_BLOOM_TEST(tbl,hashv) (1)
#define RTL_HASH_BLOOM_BYTELEN 0U
#endif

#define RTL_HASH_MAKE_TABLE(hh,head)                                             \
do {                                                                             \
  (head)->hh.tbl = (rtl_hash_table_t *)rtl_hash_malloc(sizeof(rtl_hash_table_t));\
  if (!(head)->hh.tbl) {                                                         \
    rtl_hash_fatal("out of memory");                                             \
  }                                                                              \
  rtl_hash_bzero((head)->hh.tbl, sizeof(rtl_hash_table_t));                      \
  (head)->hh.tbl->tail = &((head)->hh);                                          \
  (head)->hh.tbl->num_buckets = RTL_HASH_INITIAL_NUM_BUCKETS;                    \
  (head)->hh.tbl->log2_num_buckets = RTL_HASH_INITIAL_NUM_BUCKETS_LOG2;          \
  (head)->hh.tbl->hho = (char*)(&(head)->hh) - (char*)(head);                    \
  (head)->hh.tbl->buckets = (rtl_hash_bucket_t *)rtl_hash_malloc(                \
      RTL_HASH_INITIAL_NUM_BUCKETS * sizeof(struct rtl_hash_bucket));            \
  if (!(head)->hh.tbl->buckets) {                                                \
    rtl_hash_fatal("out of memory");                                             \
  }                                                                              \
  rtl_hash_bzero((head)->hh.tbl->buckets,                                        \
      RTL_HASH_INITIAL_NUM_BUCKETS * sizeof(struct rtl_hash_bucket));            \
  RTL_HASH_BLOOM_MAKE((head)->hh.tbl);                                           \
  (head)->hh.tbl->signature = RTL_HASH_SIGNATURE;                                \
} while (0)

#define RTL_HASH_REPLACE_BYHASHVALUE_INORDER(hh,head,fieldname,keylen_in,hashval,add,replaced,cmpfcn) \
do {                                                                             \
  (replaced) = NULL;                                                             \
  RTL_HASH_FIND_BYHASHVALUE(hh, head, &((add)->fieldname), keylen_in, hashval, replaced); \
  if (replaced) {                                                                \
    RTL_HASH_DELETE(hh, head, replaced);                                         \
  }                                                                              \
  RTL_HASH_ADD_KEYPTR_BYHASHVALUE_INORDER(hh, head, &((add)->fieldname), keylen_in, hashval, add, cmpfcn); \
} while (0)

#define RTL_HASH_REPLACE_BYHASHVALUE(hh,head,fieldname,keylen_in,hashval,add,replaced) \
do {                                                                             \
  (replaced) = NULL;                                                             \
  RTL_HASH_FIND_BYHASHVALUE(hh, head, &((add)->fieldname), keylen_in, hashval, replaced); \
  if (replaced) {                                                                \
    RTL_HASH_DELETE(hh, head, replaced);                                         \
  }                                                                              \
  RTL_HASH_ADD_KEYPTR_BYHASHVALUE(hh, head, &((add)->fieldname), keylen_in, hashval, add); \
} while (0)

#define RTL_HASH_REPLACE(hh,head,fieldname,keylen_in,add,replaced)               \
do {                                                                             \
  unsigned _hr_hashv;                                                            \
  RTL_HASH_VALUE(&((add)->fieldname), keylen_in, _hr_hashv);                     \
  RTL_HASH_REPLACE_BYHASHVALUE(hh, head, fieldname, keylen_in, _hr_hashv, add, replaced); \
} while (0)

#define RTL_HASH_REPLACE_INORDER(hh,head,fieldname,keylen_in,add,replaced,cmpfcn)\
do {                                                                             \
  unsigned _hr_hashv;                                                            \
  RTL_HASH_VALUE(&((add)->fieldname), keylen_in, _hr_hashv);                     \
  RTL_HASH_REPLACE_BYHASHVALUE_INORDER(hh, head, fieldname, keylen_in, _hr_hashv, add, replaced, cmpfcn); \
} while (0)

#define RTL_HASH_APPEND_LIST(hh, head, add)                                      \
do {                                                                             \
  (add)->hh.next = NULL;                                                         \
  (add)->hh.prev = RTL_ELMT_FROM_HH((head)->hh.tbl, (head)->hh.tbl->tail);       \
  (head)->hh.tbl->tail->next = (add);                                            \
  (head)->hh.tbl->tail = &((add)->hh);                                           \
} while (0)

#define RTL_HASH_AKBI_INNER_LOOP(hh,head,add,cmpfcn)                             \
do {                                                                             \
  do {                                                                           \
    if (cmpfcn((head)(_hs_iter), add) > 0) {                                     \
      break;                                                                     \
    }                                                                            \
  } while ((_hs_iter = RTL_HH_FROM_ELMT((head)->hh.tbl, _hs_iter)->next));       \
} while (0)

#define RTL_HASH_ADD_KEYPTR_BYHASHVALUE_INORDER(hh,head,keyptr,keylen_in,hashval,add,cmpfcn) \
do {                                                                             \
  unsigned _ha_bkt;                                                              \
  (add)->hh.hashv = (hashval);                                                   \
  (add)->hh.key = (char*) (keyptr);                                              \
  (add)->hh.keylen = (unsigned) (keylen_in);                                     \
  if (!(head)) {                                                                 \
    (add)->hh.next = NULL;                                                       \
    (add)->hh.prev = NULL;                                                       \
    (head) = (add);                                                              \
    RTL_HASH_MAKE_TABLE(hh, head);                                               \
  } else {                                                                       \
    void *_hs_iter = (head);                                                     \
    (add)->hh.tbl = (head)->hh.tbl;                                              \
    RTL_HASH_AKBI_INNER_LOOP(hh, head, add, cmpfcn);                             \
    if (_hs_iter) {                                                              \
      (add)->hh.next = _hs_iter;                                                 \
      if (((add)->hh.prev = RTL_HH_FROM_ELMT((head)->hh.tbl, _hs_iter)->prev)) { \
        RTL_HH_FROM_ELMT((head)->hh.tbl, (add)->hh.prev)->next = (add);          \
      } else {                                                                   \
        (head) = (add);                                                          \
      }                                                                          \
      RTL_HH_FROM_ELMT((head)->hh.tbl, _hs_iter)->prev = (add);                  \
    } else {                                                                     \
      RTL_HASH_APPEND_LIST(hh, head, add);                                       \
    }                                                                            \
  }                                                                              \
  (head)->hh.tbl->num_items++;                                                   \
  RTL_HASH_TO_BKT(hashval, (head)->hh.tbl->num_buckets, _ha_bkt);                \
  RTL_HASH_ADD_TO_BKT((head)->hh.tbl->buckets[_ha_bkt], &(add)->hh);             \
  RTL_HASH_BLOOM_ADD((head)->hh.tbl, hashval);                                   \
  RTL_HASH_EMIT_KEY(hh, head, keyptr, keylen_in);                                \
  RTL_HASH_FSCK(hh, head, "RTL_HASH_ADD_KEYPTR_BYHASHVALUE_INORDER");            \
} while (0)

#define RTL_HASH_ADD_KEYPTR_INORDER(hh,head,keyptr,keylen_in,add,cmpfcn)         \
do {                                                                             \
  unsigned _hs_hashv;                                                            \
  RTL_HASH_VALUE(keyptr, keylen_in, _hs_hashv);                                  \
  RTL_HASH_ADD_KEYPTR_BYHASHVALUE_INORDER(hh, head, keyptr, keylen_in, _hs_hashv, add, cmpfcn); \
} while (0)

#define RTL_HASH_ADD_BYHASHVALUE_INORDER(hh,head,fieldname,keylen_in,hashval,add,cmpfcn) \
  RTL_HASH_ADD_KEYPTR_BYHASHVALUE_INORDER(hh, head, &((add)->fieldname), keylen_in, hashval, add, cmpfcn)

#define RTL_HASH_ADD_INORDER(hh,head,fieldname,keylen_in,add,cmpfcn)             \
  RTL_HASH_ADD_KEYPTR_INORDER(hh, head, &((add)->fieldname), keylen_in, add, cmpfcn)

#define RTL_HASH_ADD_KEYPTR_BYHASHVALUE(hh,head,keyptr,keylen_in,hashval,add)    \
do {                                                                             \
  unsigned _ha_bkt;                                                              \
  (add)->hh.hashv = (hashval);                                                   \
  (add)->hh.key = (char*) (keyptr);                                              \
  (add)->hh.keylen = (unsigned) (keylen_in);                                     \
  if (!(head)) {                                                                 \
    (add)->hh.next = NULL;                                                       \
    (add)->hh.prev = NULL;                                                       \
    (head) = (add);                                                              \
    RTL_HASH_MAKE_TABLE(hh, head);                                               \
  } else {                                                                       \
    (add)->hh.tbl = (head)->hh.tbl;                                              \
    RTL_HASH_APPEND_LIST(hh, head, add);                                         \
  }                                                                              \
  (head)->hh.tbl->num_items++;                                                   \
  RTL_HASH_TO_BKT(hashval, (head)->hh.tbl->num_buckets, _ha_bkt);                \
  RTL_HASH_ADD_TO_BKT((head)->hh.tbl->buckets[_ha_bkt], &(add)->hh);             \
  RTL_HASH_BLOOM_ADD((head)->hh.tbl, hashval);                                   \
  RTL_HASH_EMIT_KEY(hh, head, keyptr, keylen_in);                                \
  RTL_HASH_FSCK(hh, head, "RTL_HASH_ADD_KEYPTR_BYHASHVALUE");                    \
} while (0)

#define RTL_HASH_ADD_KEYPTR(hh,head,keyptr,keylen_in,add)                        \
do {                                                                             \
  unsigned _ha_hashv;                                                            \
  RTL_HASH_VALUE(keyptr, keylen_in, _ha_hashv);                                  \
  RTL_HASH_ADD_KEYPTR_BYHASHVALUE(hh, head, keyptr, keylen_in, _ha_hashv, add);  \
} while (0)

#define RTL_HASH_ADD_BYHASHVALUE(hh,head,fieldname,keylen_in,hashval,add)        \
  RTL_HASH_ADD_KEYPTR_BYHASHVALUE(hh, head, &((add)->fieldname), keylen_in, hashval, add)

#define RTL_HASH_ADD(hh,head,fieldname,keylen_in,add)                            \
  RTL_HASH_ADD_KEYPTR(hh, head, &((add)->fieldname), keylen_in, add)

#define RTL_HASH_TO_BKT(hashv,num_bkts,bkt)                                      \
do {                                                                             \
  bkt = ((hashv) & ((num_bkts) - 1U));                                           \
} while (0)

/* delete "delptr" from the hash table.
 * "the usual" patch-up process for the app-order doubly-linked-list.
 * The use of _hd_hh_del below deserves special explanation.
 * These used to be expressed using (delptr) but that led to a bug
 * if someone used the same symbol for the head and deletee, like
 *  RTL_HASH_DELETE(hh,users,users);
 * We want that to work, but by changing the head (users) below
 * we were forfeiting our ability to further refer to the deletee (users)
 * in the patch-up process. Solution: use scratch space to
 * copy the deletee pointer, then the latter references are via that
 * scratch pointer rather than through the repointed (users) symbol.
 */
#define RTL_HASH_DELETE(hh,head,delptr)                                              \
    RTL_HASH_DELETE_HH(hh, head, &(delptr)->hh)

#define RTL_HASH_DELETE_HH(hh,head,delptrhh)                                         \
do {                                                                                 \
  struct rtl_hash_handle *_hd_hh_del = (delptrhh);                                   \
  if ((_hd_hh_del->prev == NULL) && (_hd_hh_del->next == NULL)) {                    \
    RTL_HASH_BLOOM_FREE((head)->hh.tbl);                                             \
    rtl_hash_free((head)->hh.tbl->buckets,                                           \
                (head)->hh.tbl->num_buckets * sizeof(struct rtl_hash_bucket));       \
    rtl_hash_free((head)->hh.tbl, sizeof(rtl_hash_table_t));                         \
    (head) = NULL;                                                                   \
  } else {                                                                           \
    unsigned _hd_bkt;                                                                \
    if (_hd_hh_del == (head)->hh.tbl->tail) {                                        \
      (head)->hh.tbl->tail = RTL_HH_FROM_ELMT((head)->hh.tbl, _hd_hh_del->prev);     \
    }                                                                                \
    if (_hd_hh_del->prev != NULL) {                                                  \
      RTL_HH_FROM_ELMT((head)->hh.tbl, _hd_hh_del->prev)->next = _hd_hh_del->next;   \
    } else {                                                                         \
      DECLTYPE_ASSIGN(head, _hd_hh_del->next);                                       \
    }                                                                                \
    if (_hd_hh_del->next != NULL) {                                                  \
      RTL_HH_FROM_ELMT((head)->hh.tbl, _hd_hh_del->next)->prev = _hd_hh_del->prev;   \
    }                                                                                \
    RTL_HASH_TO_BKT(_hd_hh_del->hashv, (head)->hh.tbl->num_buckets, _hd_bkt);        \
    RTL_HASH_DEL_IN_BKT((head)->hh.tbl->buckets[_hd_bkt], _hd_hh_del);               \
    (head)->hh.tbl->num_items--;                                                     \
  }                                                                                  \
  RTL_HASH_FSCK(hh, head, "RTL_HASH_DELETE");                                        \
} while (0)


/* convenience forms of RTL_HASH_FIND/RTL_HASH_ADD/RTL_HASH_DEL */
#define RTL_HASH_FIND_STR(head,findstr,out)                                          \
    RTL_HASH_FIND(hh,head,findstr,(unsigned)rtl_hash_strlen(findstr),out)
#define RTL_HASH_ADD_STR(head,strfield,add)                                          \
    RTL_HASH_ADD(hh,head,strfield[0],(unsigned)rtl_hash_strlen(add->strfield),add)
#define RTL_HASH_REPLACE_STR(head,strfield,add,replaced)                             \
    RTL_HASH_REPLACE(hh,head,strfield[0],(unsigned)rtl_hash_strlen(add->strfield),add,replaced)
#define RTL_HASH_FIND_INT(head,findint,out)                                          \
    RTL_HASH_FIND(hh,head,findint,sizeof(int),out)
#define RTL_HASH_ADD_INT(head,intfield,add)                                          \
    RTL_HASH_ADD(hh,head,intfield,sizeof(int),add)
#define RTL_HASH_REPLACE_INT(head,intfield,add,replaced)                             \
    RTL_HASH_REPLACE(hh,head,intfield,sizeof(int),add,replaced)
#define RTL_HASH_FIND_PTR(head,findptr,out)                                          \
    RTL_HASH_FIND(hh,head,findptr,sizeof(void *),out)
#define RTL_HASH_ADD_PTR(head,ptrfield,add)                                          \
    RTL_HASH_ADD(hh,head,ptrfield,sizeof(void *),add)
#define RTL_HASH_REPLACE_PTR(head,ptrfield,add,replaced)                             \
    RTL_HASH_REPLACE(hh,head,ptrfield,sizeof(void *),add,replaced)
#define RTL_HASH_DEL(head,delptr)                                                    \
    RTL_HASH_DELETE(hh,head,delptr)

/* RTL_HASH_FSCK checks hash integrity on every add/delete when RTL_HASH_DEBUG is defined.
 * This is for uthash developer only; it compiles away if RTL_HASH_DEBUG isn't defined.
 */
#ifdef RTL_HASH_DEBUG
#define RTL_HASH_OOPS(...) do { fprintf(stderr,__VA_ARGS__); exit(-1); } while (0)
#define RTL_HASH_FSCK(hh,head,where)                                             \
do {                                                                             \
  struct rtl_hash_handle *_thh;                                                  \
  if (head) {                                                                    \
    unsigned _bkt_i;                                                             \
    unsigned _count = 0;                                                         \
    char *_prev;                                                                 \
    for (_bkt_i = 0; _bkt_i < (head)->hh.tbl->num_buckets; ++_bkt_i) {           \
      unsigned _bkt_count = 0;                                                   \
      _thh = (head)->hh.tbl->buckets[_bkt_i].hh_head;                            \
      _prev = NULL;                                                              \
      while (_thh) {                                                             \
        if (_prev != (char*)(_thh->hh_prev)) {                                   \
          RTL_HASH_OOPS("%s: invalid hh_prev %p, actual %p\n",                   \
              (where), (void*)_thh->hh_prev, (void*)_prev);                      \
        }                                                                        \
        _bkt_count++;                                                            \
        _prev = (char*)(_thh);                                                   \
        _thh = _thh->hh_next;                                                    \
      }                                                                          \
      _count += _bkt_count;                                                      \
      if ((head)->hh.tbl->buckets[_bkt_i].count !=  _bkt_count) {                \
        RTL_HASH_OOPS("%s: invalid bucket count %u, actual %u\n",                \
            (where), (head)->hh.tbl->buckets[_bkt_i].count, _bkt_count);         \
      }                                                                          \
    }                                                                            \
    if (_count != (head)->hh.tbl->num_items) {                                   \
      RTL_HASH_OOPS("%s: invalid hh item count %u, actual %u\n",                 \
          (where), (head)->hh.tbl->num_items, _count);                           \
    }                                                                            \
    _count = 0;                                                                  \
    _prev = NULL;                                                                \
    _thh =  &(head)->hh;                                                         \
    while (_thh) {                                                               \
      _count++;                                                                  \
      if (_prev != (char*)_thh->prev) {                                          \
        RTL_HASH_OOPS("%s: invalid prev %p, actual %p\n",                        \
            (where), (void*)_thh->prev, (void*)_prev);                           \
      }                                                                          \
      _prev = (char*)RTL_ELMT_FROM_HH((head)->hh.tbl, _thh);                     \
      _thh = (_thh->next ? RTL_HH_FROM_ELMT((head)->hh.tbl, _thh->next) : NULL); \
    }                                                                            \
    if (_count != (head)->hh.tbl->num_items) {                                   \
      RTL_HASH_OOPS("%s: invalid app item count %u, actual %u\n",                \
          (where), (head)->hh.tbl->num_items, _count);                           \
    }                                                                            \
  }                                                                              \
} while (0)
#else
#define RTL_HASH_FSCK(hh,head,where)
#endif

/* When compiled with -DRTL_HASH_EMIT_KEYS, length-prefixed keys are emitted to
 * the descriptor to which this macro is defined for tuning the hash function.
 * The app can #include <unistd.h> to get the prototype for write(2). */
#ifdef RTL_HASH_EMIT_KEYS
#define RTL_HASH_EMIT_KEY(hh,head,keyptr,fieldlen)                               \
do {                                                                             \
  unsigned _klen = fieldlen;                                                     \
  write(RTL_HASH_EMIT_KEYS, &_klen, sizeof(_klen));                              \
  write(RTL_HASH_EMIT_KEYS, keyptr, (unsigned long)fieldlen);                    \
} while (0)
#else
#define RTL_HASH_EMIT_KEY(hh,head,keyptr,fieldlen)
#endif

/* default to Jenkin's hash unless overridden e.g. DRTL_HASH_FUNCTION=RTL_HASH_SAX */
#ifdef RTL_HASH_FUNCTION
#define RTL_HASH_FCN RTL_HASH_FUNCTION
#else
#define RTL_HASH_FCN RTL_HASH_JEN
#endif

/* The Bernstein hash function, used in Perl prior to v5.6. Note (x<<5+x)=x*33. */
#define RTL_HASH_BER(key,keylen,hashv)                                           \
do {                                                                             \
  unsigned _hb_keylen = (unsigned)keylen;                                        \
  const unsigned char *_hb_key = (const unsigned char*)(key);                    \
  (hashv) = 0;                                                                   \
  while (_hb_keylen-- != 0U) {                                                   \
    (hashv) = (((hashv) << 5) + (hashv)) + *_hb_key++;                           \
  }                                                                              \
} while (0)


/* SAX/FNV/OAT/JEN hash functions are macro variants of those listed at
 * http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx */
#define RTL_HASH_SAX(key,keylen,hashv)                                           \
do {                                                                             \
  unsigned _sx_i;                                                                \
  const unsigned char *_hs_key = (const unsigned char*)(key);                    \
  hashv = 0;                                                                     \
  for (_sx_i=0; _sx_i < keylen; _sx_i++) {                                       \
    hashv ^= (hashv << 5) + (hashv >> 2) + _hs_key[_sx_i];                       \
  }                                                                              \
} while (0)
/* FNV-1a variation */
#define RTL_HASH_FNV(key,keylen,hashv)                                           \
do {                                                                             \
  unsigned _fn_i;                                                                \
  const unsigned char *_hf_key = (const unsigned char*)(key);                    \
  (hashv) = 2166136261U;                                                         \
  for (_fn_i=0; _fn_i < keylen; _fn_i++) {                                       \
    hashv = hashv ^ _hf_key[_fn_i];                                              \
    hashv = hashv * 16777619U;                                                   \
  }                                                                              \
} while (0)

#define RTL_HASH_OAT(key,keylen,hashv)                                           \
do {                                                                             \
  unsigned _ho_i;                                                                \
  const unsigned char *_ho_key=(const unsigned char*)(key);                      \
  hashv = 0;                                                                     \
  for(_ho_i=0; _ho_i < keylen; _ho_i++) {                                        \
      hashv += _ho_key[_ho_i];                                                   \
      hashv += (hashv << 10);                                                    \
      hashv ^= (hashv >> 6);                                                     \
  }                                                                              \
  hashv += (hashv << 3);                                                         \
  hashv ^= (hashv >> 11);                                                        \
  hashv += (hashv << 15);                                                        \
} while (0)

#define RTL_HASH_JEN_MIX(a,b,c)                                                  \
do {                                                                             \
  a -= b; a -= c; a ^= ( c >> 13 );                                              \
  b -= c; b -= a; b ^= ( a << 8 );                                               \
  c -= a; c -= b; c ^= ( b >> 13 );                                              \
  a -= b; a -= c; a ^= ( c >> 12 );                                              \
  b -= c; b -= a; b ^= ( a << 16 );                                              \
  c -= a; c -= b; c ^= ( b >> 5 );                                               \
  a -= b; a -= c; a ^= ( c >> 3 );                                               \
  b -= c; b -= a; b ^= ( a << 10 );                                              \
  c -= a; c -= b; c ^= ( b >> 15 );                                              \
} while (0)

#define RTL_HASH_JEN(key,keylen,hashv)                                           \
do {                                                                             \
  unsigned _hj_i,_hj_j,_hj_k;                                                    \
  unsigned const char *_hj_key=(unsigned const char*)(key);                      \
  hashv = 0xfeedbeefu;                                                           \
  _hj_i = _hj_j = 0x9e3779b9u;                                                   \
  _hj_k = (unsigned)(keylen);                                                    \
  while (_hj_k >= 12U) {                                                         \
    _hj_i +=    (_hj_key[0] + ( (unsigned)_hj_key[1] << 8 )                      \
        + ( (unsigned)_hj_key[2] << 16 )                                         \
        + ( (unsigned)_hj_key[3] << 24 ) );                                      \
    _hj_j +=    (_hj_key[4] + ( (unsigned)_hj_key[5] << 8 )                      \
        + ( (unsigned)_hj_key[6] << 16 )                                         \
        + ( (unsigned)_hj_key[7] << 24 ) );                                      \
    hashv += (_hj_key[8] + ( (unsigned)_hj_key[9] << 8 )                         \
        + ( (unsigned)_hj_key[10] << 16 )                                        \
        + ( (unsigned)_hj_key[11] << 24 ) );                                     \
                                                                                 \
     RTL_HASH_JEN_MIX(_hj_i, _hj_j, hashv);                                      \
                                                                                 \
     _hj_key += 12;                                                              \
     _hj_k -= 12U;                                                               \
  }                                                                              \
  hashv += (unsigned)(keylen);                                                   \
  switch ( _hj_k ) {                                                             \
    case 11: hashv += ( (unsigned)_hj_key[10] << 24 ); /* FALLTHROUGH */         \
    case 10: hashv += ( (unsigned)_hj_key[9] << 16 );  /* FALLTHROUGH */         \
    case 9:  hashv += ( (unsigned)_hj_key[8] << 8 );   /* FALLTHROUGH */         \
    case 8:  _hj_j += ( (unsigned)_hj_key[7] << 24 );  /* FALLTHROUGH */         \
    case 7:  _hj_j += ( (unsigned)_hj_key[6] << 16 );  /* FALLTHROUGH */         \
    case 6:  _hj_j += ( (unsigned)_hj_key[5] << 8 );   /* FALLTHROUGH */         \
    case 5:  _hj_j += _hj_key[4];                      /* FALLTHROUGH */         \
    case 4:  _hj_i += ( (unsigned)_hj_key[3] << 24 );  /* FALLTHROUGH */         \
    case 3:  _hj_i += ( (unsigned)_hj_key[2] << 16 );  /* FALLTHROUGH */         \
    case 2:  _hj_i += ( (unsigned)_hj_key[1] << 8 );   /* FALLTHROUGH */         \
    case 1:  _hj_i += _hj_key[0];                                                \
  }                                                                              \
  RTL_HASH_JEN_MIX(_hj_i, _hj_j, hashv);                                         \
} while (0)

/* The Paul Hsieh hash function */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__)             \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)             \
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif
#define RTL_HASH_SFH(key,keylen,hashv)                                           \
do {                                                                             \
  unsigned const char *_sfh_key=(unsigned const char*)(key);                     \
  uint32_t _sfh_tmp, _sfh_len = (uint32_t)keylen;                                \
                                                                                 \
  unsigned _sfh_rem = _sfh_len & 3U;                                             \
  _sfh_len >>= 2;                                                                \
  hashv = 0xcafebabeu;                                                           \
                                                                                 \
  /* Main loop */                                                                \
  for (;_sfh_len > 0U; _sfh_len--) {                                             \
    hashv    += get16bits (_sfh_key);                                            \
    _sfh_tmp  = ((uint32_t)(get16bits (_sfh_key+2)) << 11) ^ hashv;              \
    hashv     = (hashv << 16) ^ _sfh_tmp;                                        \
    _sfh_key += 2U*sizeof (uint16_t);                                            \
    hashv    += hashv >> 11;                                                     \
  }                                                                              \
                                                                                 \
  /* Handle end cases */                                                         \
  switch (_sfh_rem) {                                                            \
    case 3: hashv += get16bits (_sfh_key);                                       \
            hashv ^= hashv << 16;                                                \
            hashv ^= (uint32_t)(_sfh_key[sizeof (uint16_t)]) << 18;              \
            hashv += hashv >> 11;                                                \
            break;                                                               \
    case 2: hashv += get16bits (_sfh_key);                                       \
            hashv ^= hashv << 11;                                                \
            hashv += hashv >> 17;                                                \
            break;                                                               \
    case 1: hashv += *_sfh_key;                                                  \
            hashv ^= hashv << 10;                                                \
            hashv += hashv >> 1;                                                 \
  }                                                                              \
                                                                                 \
  /* Force "avalanching" of final 127 bits */                                    \
  hashv ^= hashv << 3;                                                           \
  hashv += hashv >> 5;                                                           \
  hashv ^= hashv << 4;                                                           \
  hashv += hashv >> 17;                                                          \
  hashv ^= hashv << 25;                                                          \
  hashv += hashv >> 6;                                                           \
} while (0)

#ifdef RTL_HASH_USING_NO_STRICT_ALIASING
/* The MurmurHash exploits some CPU's (x86,x86_64) tolerance for unaligned reads.
 * For other types of CPU's (e.g. Sparc) an unaligned read causes a bus error.
 * MurmurHash uses the faster approach only on CPU's where we know it's safe.
 *
 * Note the preprocessor built-in defines can be emitted using:
 *
 *   gcc -m64 -dM -E - < /dev/null                  (on gcc)
 *   cc -## a.c (where a.c is a simple test file)   (Sun Studio)
 */
#if (defined(__i386__) || defined(__x86_64__)  || defined(_M_IX86))
#define MUR_GETBLOCK(p,i) p[i]
#else /* non intel */
#define MUR_PLUS0_ALIGNED(p) (((unsigned long)p & 3UL) == 0UL)
#define MUR_PLUS1_ALIGNED(p) (((unsigned long)p & 3UL) == 1UL)
#define MUR_PLUS2_ALIGNED(p) (((unsigned long)p & 3UL) == 2UL)
#define MUR_PLUS3_ALIGNED(p) (((unsigned long)p & 3UL) == 3UL)
#define WP(p) ((uint32_t*)((unsigned long)(p) & ~3UL))
#if (defined(__BIG_ENDIAN__) || defined(SPARC) || defined(__ppc__) || defined(__ppc64__))
#define MUR_THREE_ONE(p) ((((*WP(p))&0x00ffffff) << 8) | (((*(WP(p)+1))&0xff000000) >> 24))
#define MUR_TWO_TWO(p)   ((((*WP(p))&0x0000ffff) <<16) | (((*(WP(p)+1))&0xffff0000) >> 16))
#define MUR_ONE_THREE(p) ((((*WP(p))&0x000000ff) <<24) | (((*(WP(p)+1))&0xffffff00) >>  8))
#else /* assume little endian non-intel */
#define MUR_THREE_ONE(p) ((((*WP(p))&0xffffff00) >> 8) | (((*(WP(p)+1))&0x000000ff) << 24))
#define MUR_TWO_TWO(p)   ((((*WP(p))&0xffff0000) >>16) | (((*(WP(p)+1))&0x0000ffff) << 16))
#define MUR_ONE_THREE(p) ((((*WP(p))&0xff000000) >>24) | (((*(WP(p)+1))&0x00ffffff) <<  8))
#endif
#define MUR_GETBLOCK(p,i) (MUR_PLUS0_ALIGNED(p) ? ((p)[i]) :           \
                            (MUR_PLUS1_ALIGNED(p) ? MUR_THREE_ONE(p) : \
                             (MUR_PLUS2_ALIGNED(p) ? MUR_TWO_TWO(p) :  \
                                                      MUR_ONE_THREE(p))))
#endif
#define MUR_ROTL32(x,r) (((x) << (r)) | ((x) >> (32 - (r))))
#define MUR_FMIX(_h) \
do {                 \
  _h ^= _h >> 16;    \
  _h *= 0x85ebca6bu; \
  _h ^= _h >> 13;    \
  _h *= 0xc2b2ae35u; \
  _h ^= _h >> 16;    \
} while (0)

#define RTL_HASH_MUR(key,keylen,hashv)                                               \
do {                                                                                 \
  const uint8_t *_mur_data = (const uint8_t*)(key);                                  \
  const int _mur_nblocks = (int)(keylen) / 4;                                        \
  uint32_t _mur_h1 = 0xf88D5353u;                                                    \
  uint32_t _mur_c1 = 0xcc9e2d51u;                                                    \
  uint32_t _mur_c2 = 0x1b873593u;                                                    \
  uint32_t _mur_k1 = 0;                                                              \
  const uint8_t *_mur_tail;                                                          \
  const uint32_t *_mur_blocks = (const uint32_t*)(_mur_data+(_mur_nblocks*4));       \
  int _mur_i;                                                                        \
  for (_mur_i = -_mur_nblocks; _mur_i != 0; _mur_i++) {                              \
    _mur_k1 = MUR_GETBLOCK(_mur_blocks,_mur_i);                                      \
    _mur_k1 *= _mur_c1;                                                              \
    _mur_k1 = MUR_ROTL32(_mur_k1,15);                                                \
    _mur_k1 *= _mur_c2;                                                              \
                                                                                     \
    _mur_h1 ^= _mur_k1;                                                              \
    _mur_h1 = MUR_ROTL32(_mur_h1,13);                                                \
    _mur_h1 = (_mur_h1*5U) + 0xe6546b64u;                                            \
  }                                                                                  \
  _mur_tail = (const uint8_t*)(_mur_data + (_mur_nblocks*4));                        \
  _mur_k1=0;                                                                         \
  switch ((keylen) & 3U) {                                                           \
    case 0: break;                                                                   \
    case 3: _mur_k1 ^= (uint32_t)_mur_tail[2] << 16; /* FALLTHROUGH */               \
    case 2: _mur_k1 ^= (uint32_t)_mur_tail[1] << 8;  /* FALLTHROUGH */               \
    case 1: _mur_k1 ^= (uint32_t)_mur_tail[0];                                       \
    _mur_k1 *= _mur_c1;                                                              \
    _mur_k1 = MUR_ROTL32(_mur_k1,15);                                                \
    _mur_k1 *= _mur_c2;                                                              \
    _mur_h1 ^= _mur_k1;                                                              \
  }                                                                                  \
  _mur_h1 ^= (uint32_t)(keylen);                                                     \
  MUR_FMIX(_mur_h1);                                                                 \
  hashv = _mur_h1;                                                                   \
} while (0)
#endif  /* RTL_HASH_USING_NO_STRICT_ALIASING */

/* iterate over items in a known bucket to find desired item */
#define RTL_HASH_FIND_IN_BKT(tbl,hh,head,keyptr,keylen_in,hashval,out)               \
do {                                                                                 \
  if ((head).hh_head != NULL) {                                                      \
    DECLTYPE_ASSIGN(out, RTL_ELMT_FROM_HH(tbl, (head).hh_head));                     \
  } else {                                                                           \
    (out) = NULL;                                                                    \
  }                                                                                  \
  while ((out) != NULL) {                                                            \
    if ((out)->hh.hashv == (hashval) && (out)->hh.keylen == (keylen_in)) {           \
      if (rtl_hash_memcmp((out)->hh.key, keyptr, keylen_in) == 0) {                  \
        break;                                                                       \
      }                                                                              \
    }                                                                                \
    if ((out)->hh.hh_next != NULL) {                                                 \
      DECLTYPE_ASSIGN(out, RTL_ELMT_FROM_HH(tbl, (out)->hh.hh_next));                \
    } else {                                                                         \
      (out) = NULL;                                                                  \
    }                                                                                \
  }                                                                                  \
} while (0)

/* add an item to a bucket  */
#define RTL_HASH_ADD_TO_BKT(head,addhh)                                              \
do {                                                                                 \
  rtl_hash_bucket_t *_ha_head = &(head);                                             \
  _ha_head->count++;                                                                 \
  (addhh)->hh_next = _ha_head->hh_head;                                              \
  (addhh)->hh_prev = NULL;                                                           \
  if (_ha_head->hh_head != NULL) {                                                   \
    _ha_head->hh_head->hh_prev = (addhh);                                            \
  }                                                                                  \
  _ha_head->hh_head = (addhh);                                                       \
  if ((_ha_head->count >= ((_ha_head->expand_mult + 1U) * RTL_HASH_BKT_CAPACITY_THRESH)) \
      && !(addhh)->tbl->noexpand) {                                                  \
    RTL_HASH_EXPAND_BUCKETS((addhh)->tbl);                                           \
  }                                                                                  \
} while (0)

/* remove an item from a given bucket */
#define RTL_HASH_DEL_IN_BKT(head,delhh)                                              \
do {                                                                                 \
  rtl_hash_bucket_t *_hd_head = &(head);                                             \
  _hd_head->count--;                                                                 \
  if (_hd_head->hh_head == (delhh)) {                                                \
    _hd_head->hh_head = (delhh)->hh_next;                                            \
  }                                                                                  \
  if ((delhh)->hh_prev) {                                                            \
    (delhh)->hh_prev->hh_next = (delhh)->hh_next;                                    \
  }                                                                                  \
  if ((delhh)->hh_next) {                                                            \
    (delhh)->hh_next->hh_prev = (delhh)->hh_prev;                                    \
  }                                                                                  \
} while (0)

/* Bucket expansion has the effect of doubling the number of buckets
 * and redistributing the items into the new buckets. Ideally the
 * items will distribute more or less evenly into the new buckets
 * (the extent to which this is true is a measure of the quality of
 * the hash function as it applies to the key domain).
 *
 * With the items distributed into more buckets, the chain length
 * (item count) in each bucket is reduced. Thus by expanding buckets
 * the hash keeps a bound on the chain length. This bounded chain
 * length is the essence of how a hash provides constant time lookup.
 *
 * The calculation of tbl->ideal_chain_maxlen below deserves some
 * explanation. First, keep in mind that we're calculating the ideal
 * maximum chain length based on the *new* (doubled) bucket count.
 * In fractions this is just n/b (n=number of items,b=new num buckets).
 * Since the ideal chain length is an integer, we want to calculate
 * ceil(n/b). We don't depend on floating point arithmetic in this
 * hash, so to calculate ceil(n/b) with integers we could write
 *
 *      ceil(n/b) = (n/b) + ((n%b)?1:0)
 *
 * and in fact a previous version of this hash did just that.
 * But now we have improved things a bit by recognizing that b is
 * always a power of two. We keep its base 2 log handy (call it lb),
 * so now we can write this with a bit shift and logical AND:
 *
 *      ceil(n/b) = (n>>lb) + ( (n & (b-1)) ? 1:0)
 *
 */
#define RTL_HASH_EXPAND_BUCKETS(tbl)                                                 \
do {                                                                                 \
  unsigned _he_bkt;                                                                  \
  unsigned _he_bkt_i;                                                                \
  struct rtl_hash_handle *_he_thh, *_he_hh_nxt;                                      \
  rtl_hash_bucket_t *_he_new_buckets, *_he_newbkt;                                   \
  _he_new_buckets = (rtl_hash_bucket_t *)rtl_hash_malloc(                            \
           2UL * (tbl)->num_buckets * sizeof(struct rtl_hash_bucket));               \
  if (!_he_new_buckets) {                                                            \
    rtl_hash_fatal("out of memory");                                                 \
  }                                                                                  \
  rtl_hash_bzero(_he_new_buckets,                                                    \
          2UL * (tbl)->num_buckets * sizeof(struct rtl_hash_bucket));                \
  (tbl)->ideal_chain_maxlen =                                                        \
     ((tbl)->num_items >> ((tbl)->log2_num_buckets+1U)) +                            \
     ((((tbl)->num_items & (((tbl)->num_buckets*2U)-1U)) != 0U) ? 1U : 0U);          \
  (tbl)->nonideal_items = 0;                                                         \
  for (_he_bkt_i = 0; _he_bkt_i < (tbl)->num_buckets; _he_bkt_i++) {                 \
    _he_thh = (tbl)->buckets[ _he_bkt_i ].hh_head;                                   \
    while (_he_thh != NULL) {                                                        \
      _he_hh_nxt = _he_thh->hh_next;                                                 \
      RTL_HASH_TO_BKT(_he_thh->hashv, (tbl)->num_buckets * 2U, _he_bkt);             \
      _he_newbkt = &(_he_new_buckets[_he_bkt]);                                      \
      if (++(_he_newbkt->count) > (tbl)->ideal_chain_maxlen) {                       \
        (tbl)->nonideal_items++;                                                     \
        _he_newbkt->expand_mult = _he_newbkt->count / (tbl)->ideal_chain_maxlen;     \
      }                                                                              \
      _he_thh->hh_prev = NULL;                                                       \
      _he_thh->hh_next = _he_newbkt->hh_head;                                        \
      if (_he_newbkt->hh_head != NULL) {                                             \
        _he_newbkt->hh_head->hh_prev = _he_thh;                                      \
      }                                                                              \
      _he_newbkt->hh_head = _he_thh;                                                 \
      _he_thh = _he_hh_nxt;                                                          \
    }                                                                                \
  }                                                                                  \
  rtl_hash_free((tbl)->buckets, (tbl)->num_buckets * sizeof(struct rtl_hash_bucket));\
  (tbl)->num_buckets *= 2U;                                                          \
  (tbl)->log2_num_buckets++;                                                         \
  (tbl)->buckets = _he_new_buckets;                                                  \
  (tbl)->ineff_expands = ((tbl)->nonideal_items > ((tbl)->num_items >> 1)) ?         \
      ((tbl)->ineff_expands+1U) : 0U;                                                \
  if ((tbl)->ineff_expands > 1U) {                                                   \
    (tbl)->noexpand = 1;                                                             \
    rtl_hash_noexpand_fyi(tbl);                                                      \
  }                                                                                  \
  rtl_hash_expand_fyi(tbl);                                                          \
} while (0)


/* This is an adaptation of Simon Tatham's O(n log(n)) mergesort */
/* Note that RTL_HASH_SORT assumes the hash handle name to be hh.
 * RTL_HASH_SRT was added to allow the hash handle name to be passed in. */
#define RTL_HASH_SORT(head,cmpfcn) RTL_HASH_SRT(hh,head,cmpfcn)
#define RTL_HASH_SRT(hh,head,cmpfcn)                                                 \
do {                                                                                 \
  unsigned _hs_i;                                                                    \
  unsigned _hs_looping,_hs_nmerges,_hs_insize,_hs_psize,_hs_qsize;                   \
  struct rtl_hash_handle *_hs_p, *_hs_q, *_hs_e, *_hs_list, *_hs_tail;               \
  if (head != NULL) {                                                                \
    _hs_insize = 1;                                                                  \
    _hs_looping = 1;                                                                 \
    _hs_list = &((head)->hh);                                                        \
    while (_hs_looping != 0U) {                                                      \
      _hs_p = _hs_list;                                                              \
      _hs_list = NULL;                                                               \
      _hs_tail = NULL;                                                               \
      _hs_nmerges = 0;                                                               \
      while (_hs_p != NULL) {                                                        \
        _hs_nmerges++;                                                               \
        _hs_q = _hs_p;                                                               \
        _hs_psize = 0;                                                               \
        for (_hs_i = 0; _hs_i < _hs_insize; ++_hs_i) {                               \
          _hs_psize++;                                                               \
          _hs_q = ((_hs_q->next != NULL) ?                                           \
            RTL_HH_FROM_ELMT((head)->hh.tbl, _hs_q->next) : NULL);                   \
          if (_hs_q == NULL) {                                                       \
            break;                                                                   \
          }                                                                          \
        }                                                                            \
        _hs_qsize = _hs_insize;                                                      \
        while ((_hs_psize != 0U) || ((_hs_qsize != 0U) && (_hs_q != NULL))) {        \
          if (_hs_psize == 0U) {                                                     \
            _hs_e = _hs_q;                                                           \
            _hs_q = ((_hs_q->next != NULL) ?                                         \
              RTL_HH_FROM_ELMT((head)->hh.tbl, _hs_q->next) : NULL);                 \
            _hs_qsize--;                                                             \
          } else if ((_hs_qsize == 0U) || (_hs_q == NULL)) {                         \
            _hs_e = _hs_p;                                                           \
            if (_hs_p != NULL) {                                                     \
              _hs_p = ((_hs_p->next != NULL) ?                                       \
                RTL_HH_FROM_ELMT((head)->hh.tbl, _hs_p->next) : NULL);               \
            }                                                                        \
            _hs_psize--;                                                             \
          } else if ((cmpfcn(                                                        \
                DECLTYPE(head)(RTL_ELMT_FROM_HH((head)->hh.tbl, _hs_p)),             \
                DECLTYPE(head)(RTL_ELMT_FROM_HH((head)->hh.tbl, _hs_q))              \
                )) <= 0) {                                                           \
            _hs_e = _hs_p;                                                           \
            if (_hs_p != NULL) {                                                     \
              _hs_p = ((_hs_p->next != NULL) ?                                       \
                RTL_HH_FROM_ELMT((head)->hh.tbl, _hs_p->next) : NULL);               \
            }                                                                        \
            _hs_psize--;                                                             \
          } else {                                                                   \
            _hs_e = _hs_q;                                                           \
            _hs_q = ((_hs_q->next != NULL) ?                                         \
              RTL_HH_FROM_ELMT((head)->hh.tbl, _hs_q->next) : NULL);                 \
            _hs_qsize--;                                                             \
          }                                                                          \
          if ( _hs_tail != NULL ) {                                                  \
            _hs_tail->next = ((_hs_e != NULL) ?                                      \
              RTL_ELMT_FROM_HH((head)->hh.tbl, _hs_e) : NULL);                       \
          } else {                                                                   \
            _hs_list = _hs_e;                                                        \
          }                                                                          \
          if (_hs_e != NULL) {                                                       \
            _hs_e->prev = ((_hs_tail != NULL) ?                                      \
              RTL_ELMT_FROM_HH((head)->hh.tbl, _hs_tail) : NULL);                    \
          }                                                                          \
          _hs_tail = _hs_e;                                                          \
        }                                                                            \
        _hs_p = _hs_q;                                                               \
      }                                                                              \
      if (_hs_tail != NULL) {                                                        \
        _hs_tail->next = NULL;                                                       \
      }                                                                              \
      if (_hs_nmerges <= 1U) {                                                       \
        _hs_looping = 0;                                                             \
        (head)->hh.tbl->tail = _hs_tail;                                             \
        DECLTYPE_ASSIGN(head, RTL_ELMT_FROM_HH((head)->hh.tbl, _hs_list));           \
      }                                                                              \
      _hs_insize *= 2U;                                                              \
    }                                                                                \
    RTL_HASH_FSCK(hh, head, "RTL_HASH_SRT");                                         \
  }                                                                                  \
} while (0)

/* This function selects items from one hash into another hash.
 * The end result is that the selected items have dual presence
 * in both hashes. There is no copy of the items made; rather
 * they are added into the new hash through a secondary hash
 * hash handle that must be present in the structure. */
#define RTL_HASH_SELECT(hh_dst, dst, hh_src, src, cond)                              \
do {                                                                                 \
  unsigned _src_bkt, _dst_bkt;                                                       \
  void *_last_elt = NULL, *_elt;                                                     \
  rtl_hash_handle_t *_src_hh, *_dst_hh, *_last_elt_hh=NULL;                          \
  ptrdiff_t _dst_hho = ((char*)(&(dst)->hh_dst) - (char*)(dst));                     \
  if ((src) != NULL) {                                                               \
    for (_src_bkt=0; _src_bkt < (src)->hh_src.tbl->num_buckets; _src_bkt++) {        \
      for (_src_hh = (src)->hh_src.tbl->buckets[_src_bkt].hh_head;                   \
        _src_hh != NULL;                                                             \
        _src_hh = _src_hh->hh_next) {                                                \
        _elt = RTL_ELMT_FROM_HH((src)->hh_src.tbl, _src_hh);                         \
        if (cond(_elt)) {                                                            \
          _dst_hh = (rtl_hash_handle_t *)(((char*)_elt) + _dst_hho);                 \
          _dst_hh->key = _src_hh->key;                                               \
          _dst_hh->keylen = _src_hh->keylen;                                         \
          _dst_hh->hashv = _src_hh->hashv;                                           \
          _dst_hh->prev = _last_elt;                                                 \
          _dst_hh->next = NULL;                                                      \
          if (_last_elt_hh != NULL) {                                                \
            _last_elt_hh->next = _elt;                                               \
          }                                                                          \
          if ((dst) == NULL) {                                                       \
            DECLTYPE_ASSIGN(dst, _elt);                                              \
            RTL_HASH_MAKE_TABLE(hh_dst, dst);                                        \
          } else {                                                                   \
            _dst_hh->tbl = (dst)->hh_dst.tbl;                                        \
          }                                                                          \
          RTL_HASH_TO_BKT(_dst_hh->hashv, _dst_hh->tbl->num_buckets, _dst_bkt);      \
          RTL_HASH_ADD_TO_BKT(_dst_hh->tbl->buckets[_dst_bkt], _dst_hh);             \
          RTL_HASH_BLOOM_ADD(_dst_hh->tbl, _dst_hh->hashv);                          \
          (dst)->hh_dst.tbl->num_items++;                                            \
          _last_elt = _elt;                                                          \
          _last_elt_hh = _dst_hh;                                                    \
        }                                                                            \
      }                                                                              \
    }                                                                                \
  }                                                                                  \
  RTL_HASH_FSCK(hh_dst, dst, "RTL_HASH_SELECT");                                     \
} while (0)

#define RTL_HASH_CLEAR(hh,head)                                                      \
do {                                                                                 \
  if ((head) != NULL) {                                                              \
    RTL_HASH_BLOOM_FREE((head)->hh.tbl);                                             \
    rtl_hash_free((head)->hh.tbl->buckets,                                           \
                (head)->hh.tbl->num_buckets*sizeof(struct rtl_hash_bucket));         \
    rtl_hash_free((head)->hh.tbl, sizeof(rtl_hash_table_t));                         \
    (head) = NULL;                                                                   \
  }                                                                                  \
} while (0)

#define RTL_HASH_OVERHEAD(hh,head)                                                   \
 (((head) != NULL) ? (                                                               \
 (size_t)(((head)->hh.tbl->num_items   * sizeof(rtl_hash_handle_t))   +              \
          ((head)->hh.tbl->num_buckets * sizeof(rtl_hash_bucket_t))   +              \
           sizeof(rtl_hash_table_t)                                   +              \
           (RTL_HASH_BLOOM_BYTELEN))) : 0U)

#define RTL_HASH_ITER(hh,head,el,tmp)                                                \
for(((el)=(head)), ((tmp)=DECLTYPE(el)((head!=NULL)?(head)->hh.next:NULL));          \
  (el) != NULL; ((el)=(tmp)), ((tmp)=DECLTYPE(el)((tmp!=NULL)?(tmp)->hh.next:NULL)))

/* obtain a count of items in the hash */
#define RTL_HASH_COUNT(head) RTL_HASH_CNT(hh,head)
#define RTL_HASH_CNT(hh,head) ((head != NULL)?((head)->hh.tbl->num_items):0U)

typedef struct rtl_hash_bucket {
   struct rtl_hash_handle *hh_head;
   unsigned count;
   /* expand_mult is normally set to 0. In this situation, the max chain length
    * threshold is enforced at its default value, RTL_HASH_BKT_CAPACITY_THRESH. (If
    * the bucket's chain exceeds this length, bucket expansion is triggered).
    * However, setting expand_mult to a non-zero value delays bucket expansion
    * (that would be triggered by additions to this particular bucket)
    * until its chain length reaches a *multiple* of RTL_HASH_BKT_CAPACITY_THRESH.
    * (The multiplier is simply expand_mult+1). The whole idea of this
    * multiplier is to reduce bucket expansions, since they are expensive, in
    * situations where we know that a particular bucket tends to be overused.
    * It is better to let its chain length grow to a longer yet-still-bounded
    * value, than to do an O(n) bucket expansion too often.
    */
   unsigned expand_mult;
} rtl_hash_bucket_t;

/* random signature used only to find hash tables in external analysis */
#define RTL_HASH_SIGNATURE 0xa0111fe1u
#define RTL_HASH_BLOOM_SIGNATURE 0xb12220f2u

typedef struct rtl_hash_table {
   struct rtl_hash_bucket *buckets;
   unsigned num_buckets, log2_num_buckets;
   unsigned num_items;
   struct rtl_hash_handle *tail; /* tail hh in app order, for fast append */
   ptrdiff_t hho; /* hash handle offset (byte pos of hash handle in element */

   /* in an ideal situation (all buckets used equally), no bucket would have
    * more than ceil(#items/#buckets) items. that's the ideal chain length.
	*/
   unsigned ideal_chain_maxlen;

   /* nonideal_items is the number of items in the hash whose chain position
    * exceeds the ideal chain maxlen. these items pay the penalty for an uneven
    * hash distribution; reaching them in a chain traversal takes >ideal steps
	*/
   unsigned nonideal_items;

   /* ineffective expands occur when a bucket doubling was performed, but
    * afterward, more than half the items in the hash had nonideal chain
    * positions. If this happens on two consecutive expansions we inhibit any
    * further expansion, as it's not helping; this happens when the hash
    * function isn't a good fit for the key domain. When expansion is inhibited
    * the hash will still work, albeit no longer in constant time.
	*/
   unsigned ineff_expands, noexpand;

   uint32_t signature; /* used only to find hash tables in external analysis */
#ifdef RTL_HASH_BLOOM
   uint32_t bloom_sig; /* used only to test bloom exists in external analysis */
   uint8_t *bloom_bv;
   uint8_t bloom_nbits;
#endif
} rtl_hash_table_t;

typedef struct rtl_hash_handle {
   struct rtl_hash_table *tbl;
   void *prev;                       /* prev element in app order      */
   void *next;                       /* next element in app order      */
   struct rtl_hash_handle *hh_prev;  /* previous hh in bucket order    */
   struct rtl_hash_handle *hh_next;  /* next hh in bucket order        */
   void *key;                        /* ptr to enclosing struct's key  */
   unsigned keylen;                  /* enclosing struct's key len     */
   unsigned hashv;                   /* result of hash-fcn(key)        */
} rtl_hash_handle_t;

#endif /* _RTL_HASH_H_ */
