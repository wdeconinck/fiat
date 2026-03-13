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
#include <string>

#include "abor1.h"

#define FIAT_BUDDY_ABORT(...) do { \
    char buffer[1024]; \
    snprintf(buffer, sizeof(buffer), __VA_ARGS__); \
    ABOR1(buffer); \
} while (0)

namespace {
#define BUDDY_CPP_MANGLED
#define BUDDY_ALLOC_IMPLEMENTATION
#include "buddy_alloc/buddy_alloc.h"
#undef BUDDY_ALLOC_IMPLEMENTATION
}

#define FIAT_BUDDY_DEBUG 0

namespace fiat {

struct buddy_alloc {
  unsigned char* metadata;
  unsigned char* arena;
  struct buddy* buddy;
  size_t size;
  size_t allocated;
};

struct buddy_alloc_pool_node {
     buddy_alloc* value;
     buddy_alloc_pool_node* next;
};

struct buddy_alloc_pool {
    buddy_alloc_pool_node* head;
    size_t grow_factor;
};

namespace {

buddy_alloc* buddy_alloc_new (size_t size) {
    buddy_alloc* b = new buddy_alloc;
    if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_new: size %zu\n", size);
    if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_new: buddy_sizeof (size) %zu\n", buddy_sizeof(size));
    b->metadata  = new unsigned char[buddy_sizeof (size)];
    b->arena     = new unsigned char[size];
    b->buddy     = buddy_init (b->metadata, b->arena, size);
    b->size      = size;
    b->allocated = 0;
    return b;
}

void buddy_alloc_delete (buddy_alloc* b) {
  if (b == nullptr) {
    return;
  }
  delete[] b->metadata;
  delete[] b->arena;
  delete b;
}

void* buddy_alloc_allocate (buddy_alloc* b, size_t size) {
    if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_allocate: size %zu", size);
    void* ptr = buddy_malloc (b->buddy, size);
    if (ptr) {
        if (FIAT_BUDDY_DEBUG) fprintf(stderr, " .. OK\n");
        b->allocated += buddy_alloc_size (b->buddy, ptr);
    }
    else {
        if (FIAT_BUDDY_DEBUG) fprintf(stderr, " .. FAILED\n");
    }
    return ptr;
}

int buddy_alloc_deallocate (buddy_alloc* b, void* ptr, size_t size) {
  uintptr_t base = (uintptr_t)b->arena;
  if ((base <= (uintptr_t)ptr) && ((uintptr_t)ptr < base + b->size)) {
    if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_deallocate: size %zu\n", size);
    b->allocated -= buddy_alloc_size (b->buddy, ptr);
    buddy_free (b->buddy, ptr);
    return 0; // SUCCESS
  }
  return -1; // ERROR_OUT_OF_BOUNDS
}

buddy_alloc_pool_node* buddy_alloc_pool_node_new(size_t size) {
    if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_pool_node_new: size %zu\n", size);
    buddy_alloc_pool_node* node = new buddy_alloc_pool_node;
    node->value = buddy_alloc_new(size);
    node->next  = nullptr;
    return node;
}

void buddy_alloc_pool_node_delete(buddy_alloc_pool_node* node) {
    if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_pool_node_delete\n");
    if (node == nullptr) {
        return;
    }
    buddy_alloc_delete(node->value);
    delete node;
}

buddy_alloc_pool* buddy_alloc_pool_new (size_t size, size_t grow_factor) {
    buddy_alloc_pool *pool = new buddy_alloc_pool;
    pool->grow_factor = grow_factor;
    if (size > 0) {
        if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_pool_new: Initialize first pool node with size %zu\n", size);
        pool->head = buddy_alloc_pool_node_new(size);
    }
    else {
        if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_pool_new: Delay first pool node creation to first allocation as size = 0 \n");
        pool->head = nullptr; // To be initialized upon first buddy_alloc_pool_allocate call.
    }
    return pool;
}

void buddy_alloc_pool_delete (buddy_alloc_pool* pool) {
  if (pool == nullptr) {
    return;
  }
  buddy_alloc_pool_node* curr = pool->head;
  while (curr) {
    buddy_alloc_pool_node* next = curr->next;
    buddy_alloc_pool_node_delete(curr);
    curr = next;
  }
  delete pool;
}

void* buddy_alloc_pool_allocate(buddy_alloc_pool* pool, size_t size) {
    if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_pool_allocate: size %zu\n", size);
    void* ptr = nullptr;

    // Initialize if not yet initialized
    if (pool->head == nullptr) {
      size_t initial_pool_size = 256*1024*1024; // Default to 256MB if no size is provided
      if (size > initial_pool_size) {
        initial_pool_size = size;
      }
      if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_pool_allocate: Initialize first pool node with size %zu\n", initial_pool_size);
      pool->head = buddy_alloc_pool_node_new(initial_pool_size);
    }

    buddy_alloc_pool_node* curr = pool->head;
    while (curr) {
        ptr = buddy_alloc_allocate(curr->value, size);
        if (ptr) {
          return ptr;
        }

        // If last node and still no space, grow
        if (!curr->next) {
            if (pool->grow_factor == 0) {
                FIAT_BUDDY_ABORT("FIAT_BUDDY_ALLOCATE: Failed to allocate %zuB. No more space in pool and grow_factor is 0.", size);
            }
            size_t next_size = curr->value->size * pool->grow_factor;
            while( next_size < size ) {
              next_size *= pool->grow_factor;
            }
            if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_pool_allocate: New pool node with size %zu\n", next_size);
            curr->next = buddy_alloc_pool_node_new(next_size);
        }
        curr = curr->next;
    }
    FIAT_BUDDY_ABORT("FIAT_BUDDY_ALLOCATE: Failed to allocate %zuB. This should not happen as new pool nodes are created until the allocation succeeds.", size);
    return ptr;
}

void buddy_alloc_pool_deallocate(buddy_alloc_pool* pool, void* ptr, size_t size) {
    if (FIAT_BUDDY_DEBUG) fprintf(stderr, "buddy_alloc_pool_deallocate: size %zu\n", size);
    if (ptr == nullptr) {
      return;
    }

    buddy_alloc_pool_node* curr = pool->head;
    while (curr) {
        if (buddy_alloc_deallocate(curr->value, ptr, size) == 0) {
            return;
        }
        curr = curr->next;
    }
    FIAT_BUDDY_ABORT("FIAT_BUDDY_DEALLOCATE: Failed to deallocate memory. ptr \"%p\" is out of bounds.", ptr);
}


void buddy_alloc_pool_reserve (buddy_alloc_pool* pool, size_t size) {
    void* ptr = buddy_alloc_pool_allocate (pool, size);
    buddy_alloc_pool_deallocate (pool, ptr, size);
}

size_t buddy_alloc_pool_capacity (buddy_alloc_pool* pool) {
    if (pool->head == nullptr) {
      return 0;
    }
    buddy_alloc_pool_node* curr = pool->head;
    size_t capacity = 0;
    while (curr) {
        capacity += curr->value->size;
        curr = curr->next;
    }
    return capacity;
}

size_t buddy_alloc_pool_allocated (buddy_alloc_pool* pool) {
    if (pool->head == nullptr) {
      return 0;
    }
    buddy_alloc_pool_node* curr = pool->head;
    size_t allocated = 0;
    while (curr) {
        allocated += curr->value->allocated;
        curr = curr->next;
    }
    return allocated;
}

} // anonymous namespace

extern "C" {

  // Multiple buddy_allocs in linked-list which grows when needed.
  void c_fiat_buddy_new (buddy_alloc_pool* &pool, size_t size, size_t grow_factor) {
    pool = buddy_alloc_pool_new (size, grow_factor);
  }

  void c_fiat_buddy_delete (buddy_alloc_pool* pool) {
    buddy_alloc_pool_delete (pool);
  }

  void c_fiat_buddy_allocate (buddy_alloc_pool* pool, size_t size, void* &ptr) {
    ptr = buddy_alloc_pool_allocate (pool, size);
  }

  void c_fiat_buddy_deallocate (buddy_alloc_pool* pool, void* ptr, size_t size) {
    buddy_alloc_pool_deallocate (pool, ptr, size);
  }

  void c_fiat_buddy_reserve (buddy_alloc_pool* pool, size_t size) {
    buddy_alloc_pool_reserve (pool, size);
  }

  void c_fiat_buddy_capacity (buddy_alloc_pool* pool, size_t& capacity) {
    capacity = buddy_alloc_pool_capacity (pool);
  }
  void c_fiat_buddy_allocated (buddy_alloc_pool* pool, size_t& allocated) {
    allocated = buddy_alloc_pool_allocated (pool);
  }

}

} //namespace fiat
