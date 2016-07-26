/**
 * @copyright
 * Copyright (c) 2016 Stanislav Ivochkin
 * Licensed under the MIT License:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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

#if DFK_DEBUG
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
#if DFK_DEBUG
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
#if DFK_DEBUG
  hook->list = NULL;
#endif
}


void dfk_list_hook_free(dfk_list_hook_t* hook)
{
  DFK_UNUSED(hook);
  assert(hook);
#if DFK_DEBUG
  hook->prev = NULL;
  hook->next = NULL;
  assert(!hook->list);
#endif
}


void dfk_list_append(dfk_list_t* list, dfk_list_hook_t* hook)
{
  assert(list);
  assert(hook);
#if DFK_DEBUG
  assert(!hook->list);
#endif

  hook->next = NULL;
  hook->prev = list->tail;
#if DFK_DEBUG
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
#if DFK_DEBUG
  assert(!hook->list);
#endif

  hook->prev = NULL;
  hook->next = list->head;
#if DFK_DEBUG
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
#if DFK_DEBUG
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
#if DFK_DEBUG
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
#if DFK_DEBUG
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

