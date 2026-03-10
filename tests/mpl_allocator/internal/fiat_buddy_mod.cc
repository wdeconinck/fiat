/*
 * (C) Copyright 2022- ECMWF.
 * (C) Copyright 2022- Meteo-France.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define BUDDY_CPP_NAMESPACE fiat_detail
#define BUDDY_ALLOC_IMPLEMENTATION
#include "buddy_alloc.h"
#undef BUDDY_ALLOC_IMPLEMENTATION

using BUDDY_CPP_NAMESPACE::buddy;
using BUDDY_CPP_NAMESPACE::buddy_sizeof;
using BUDDY_CPP_NAMESPACE::buddy_init;
using BUDDY_CPP_NAMESPACE::buddy_free;

typedef struct
{
  unsigned char * metadata;
  unsigned char * arena;
  struct buddy * buddy;
  size_t size;
} c_fiat_buddy_t;

extern "C" 
{

void c_fiat_buddy_new (c_fiat_buddy_t ** heap, size_t size)
{
  *heap = (c_fiat_buddy_t *)malloc (sizeof (c_fiat_buddy_t));
  (*heap)->metadata = (unsigned char *)malloc (buddy_sizeof (size));
  (*heap)->arena    = (unsigned char *)malloc (size);
  (*heap)->buddy    = buddy_init ((*heap)->metadata, (*heap)->arena, size);
  (*heap)->size     = size;
}

void c_fiat_buddy_delete (c_fiat_buddy_t * heap)
{
  if (heap == NULL)
    return;
  if (heap->metadata)
    free (heap->metadata); 
  heap->metadata = NULL;
  if (heap->arena)
    free (heap->arena);    
  heap->arena = NULL;
  heap->buddy = NULL;
  free (heap);
}

void c_fiat_buddy_allocate (c_fiat_buddy_t * heap, size_t size, void ** ptr)
{
  *ptr = buddy_malloc (heap->buddy, size);
  if (*ptr == NULL)
    abort ();
}

void c_fiat_buddy_deallocate (c_fiat_buddy_t * heap, void * ptr)
{
  char * b = (char *)ptr, * u = b + heap->size;
  if ((b <= ptr) && (ptr < u))
    {
      buddy_free (heap->buddy, ptr);  
    }
  else
    {
      abort ();
    }
}

}
