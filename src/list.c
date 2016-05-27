/**
 * @copyright
 * Copyright (c) 2016, Stanislav Ivochkin. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <assert.h>
#include <dfk/internal.h>
#include <dfk/internal/list.h>


#ifdef NDEBUG
#define DFK_LIST_CHECK_INVARIANTS(list) DFK_UNUSED(list)
#else
static void dfk_list_check_invariants(dfk_list_t* list)
{
  assert(list);

  if (list->size == 0) {
    assert(!list->head);
    assert(!list->tail);
    return;
  }

  /* size > 0 */
  assert(list->head);
  assert(list->tail);
  if (list->size == 1) {
    assert(list->head == list->tail);
  }

  assert(!list->head->prev);
  assert(!list->tail->next);

  {
    /* check head -> tail traversal */
    size_t nhops = 0;
    dfk_list_hook_t* it = list->head;
    while (it && (nhops < list->size)) {
      if (it == list->tail) {
        break;
      }
      it = it->next;
      ++nhops;
    }
    assert(it == list->tail);
    assert(nhops + 1 == list->size);
  }

  {
    /* check tail -> head traversal */
    size_t nhops = 0;
    dfk_list_hook_t* it = list->tail;
    while (it && (nhops < list->size)) {
      if (it == list->head) {
        break;
      }
      it = it->prev;
      ++nhops;
    }
    assert(it == list->head);
    assert(nhops + 1 == list->size);
  }

#ifdef DFK_DEBUG
  {
    /* assert that all elements have correct .list property
     * we have check head -> tail traversal before, so `while' loop
     * condition will eventually exit
     */
    dfk_list_hook_t* it = list->head;
    while (it) {
      assert(it->list == list);
      it = it->next;
    }
  }
#endif
}

#define DFK_LIST_CHECK_INVARIANTS(list) dfk_list_check_invariants((list))
#endif /* NDEBUG */


void dfk_list_init(dfk_list_t* list)
{
  assert(list);
  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_free(dfk_list_t* list)
{
  DFK_UNUSED(list);
  assert(list);
#ifdef DFK_DEBUG
  {
    dfk_list_hook_t* it = list->head;
    while (it) {
      dfk_list_hook_t* next = it->next;
      it->prev = NULL;
      it->list = NULL;
      it->next = NULL;
      it = next;
    }
  }
  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
#endif
}


void dfk_list_hook_init(dfk_list_hook_t* hook)
{
  assert(hook);
  hook->next = NULL;
  hook->prev = NULL;
#ifdef DFK_DEBUG
  hook->list = NULL;
#endif
}


void dfk_list_hook_free(dfk_list_hook_t* hook)
{
  DFK_UNUSED(hook);
  assert(hook);
#ifdef DFK_DEBUG
  hook->prev = NULL;
  hook->next = NULL;
  assert(!hook->list);
#endif
}


void dfk_list_append(dfk_list_t* list, dfk_list_hook_t* hook)
{
  assert(list);
  assert(hook);
#ifdef DFK_DEBUG
  assert(!hook->list);
#endif

  hook->next = NULL;
  hook->prev = list->tail;
#ifdef DFK_DEBUG
  hook->list = list;
#endif
  if (list->tail) {
    list->tail->next = hook;
  }
  if (!list->head) {
    list->head = hook;
  }
  list->tail = hook;
  ++list->size;

  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_prepend(dfk_list_t* list, dfk_list_hook_t* hook)
{
  assert(list);
  assert(hook);
#ifdef DFK_DEBUG
  assert(!hook->list);
#endif

  hook->prev = NULL;
  hook->next = list->head;
#ifdef DFK_DEBUG
  hook->list = list;
#endif

  if (list->head) {
    list->head->prev = hook;
  }
  list->head = hook;
  if (!list->tail) {
    list->tail = hook;
  }
  ++list->size;

  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_clear(dfk_list_t* list)
{
  assert(list);
#ifdef DFK_DEBUG
  {
    dfk_list_hook_t* i = list->head;
    while (i) {
      i->list = NULL;
      i = i->next;
    }
  }
#endif
  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_erase(dfk_list_t* list, dfk_list_hook_t* hook)
{
  assert(list);
  assert(hook);
#ifdef DFK_DEBUG
  assert(hook->list == list);
#endif

  assert(list->head);
  assert(list->tail);
  assert(list->size);


  if (list->head == list->tail) {
    /* The list of size 1 */
    assert(list->head == hook);
    assert(list->size == 1);
    list->head = NULL;
    list->tail = NULL;
  } else if (!hook->prev) {
    /* Remove from head */
    assert(list->head == hook);
    assert(list->head->next);
    hook->next->prev = NULL;
    list->head = list->head->next;
  } else if (!hook->next) {
    /* Remove from tail */
    assert(list->tail == hook);
    assert(list->tail->prev);
    hook->prev->next = NULL;
    list->tail = list->tail->prev;
  } else {
    /* Remove from middle */
    assert(list->size > 2);
    assert(hook != list->head);
    assert(hook != list->tail);
    assert(hook->prev);
    assert(hook->next);
    hook->prev->next = hook->next;
    hook->next->prev = hook->prev;
  }

  list->size--;

  hook->next = NULL;
  hook->prev = NULL;
#ifdef DFK_DEBUG
  hook->list = NULL;
#endif

  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_pop_front(dfk_list_t* list)
{
  assert(list);
  assert(list->head);
  dfk_list_erase(list, list->head);
  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_pop_back(dfk_list_t* list)
{
  assert(list);
  assert(list->tail);
  dfk_list_erase(list, list->tail);
  DFK_LIST_CHECK_INVARIANTS(list);
}


size_t dfk_list_size(dfk_list_t* list)
{
  assert(list);
  DFK_LIST_CHECK_INVARIANTS(list);
  return list->size;
}

