/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <string.h>
#include <assert.h>
#include <dfk/internal.h>
#include <dfk/list.h>


#ifdef NDEBUG

#define DFK_LIST_CHECK_INVARIANTS(list) DFK_UNUSED(list)
#define DFK_LIST_HOOK_CHECK_INVARIANTS(hook) DFK_UNUSED(hook)
#define DFK_LIST_IT_CHECK_INVARIANTS(it) DFK_UNUSED(it)
#define DFK_LIST_RIT_CHECK_INVARIANTS(rit) DFK_UNUSED(rit)

#else

static void dfk__list_check_invariants(dfk_list_t* list)
{
  assert(list);

  /*
   * Empty list.
   * list._head and list._tail can be equal to NULL only when list is empty.
   * In this case they both should be equal to NULL.
   */
  if (!list->_head || !list->_tail) {
    assert(!list->_head);
    assert(!list->_tail);
#if DFK_LIST_CONSTANT_TIME_SIZE
    assert(list->_size == 0);
#endif
    return;
  }

  /*
   * List of size 1
   */
  if (list->_head == list->_tail) {
    assert(!list->_head->_next);
    assert(!list->_head->_prev);
#if DFK_LIST_CONSTANT_TIME_SIZE
    assert(list->_size == 1);
#endif
    DFK_IF_DEBUG(assert(list->_head->_list == list));
    return;
  }

  /*
   * For non-empty list check that list._head._prev and
   * list._tail._next are NULL
   */
  assert(!list->_head->_prev);
  assert(!list->_tail->_next);

  {
    /*
     * Check that head -> tail traversal
     * 1) has no loop
     * 2) has exactly list._size hops, if list._size is available
     * 3) ends at list->_tail
     *
     * Loop detection implemented via an extra pointer that traverses
     * list by two hops at once. If list has no loop, two pointers should
     * never become equal.
     */
    size_t nhops = 0;
    dfk_list_hook_t* i = list->_head;
    dfk_list_hook_t* j = list->_head;
    /* while (i) will not detect case 3) */
    while (i != list->_tail) {
      assert(i); /* List ended unexpectedly */
      i = i->_next;
      if (j) {
        j = j->_next;
      }
      if (j) {
        j = j->_next;
      }
      assert(i != j); /* loop detected */
      ++nhops;
#if DFK_LIST_CONSTANT_TIME_SIZE
      assert(nhops < list->_size);
#endif
    }
    assert(i == list->_tail);
    assert(!j);
#if DFK_LIST_CONSTANT_TIME_SIZE
    assert(nhops + 1 == list->_size);
#endif
  }

  {
    /*
     * check tail -> head traversal, using the same technique
     * as for head -> tail
     */
    size_t nhops = 0;
    dfk_list_hook_t* i = list->_tail;
    dfk_list_hook_t* j = list->_tail;
    while (i != list->_head) {
      assert(i); /* List ended unexpectedly */
      i = i->_prev;
      if (j) {
        j = j->_prev;
      }
      if (j) {
        j = j->_prev;
      }
      assert(i != j); /* loop detected */
      ++nhops;
#if DFK_LIST_CONSTANT_TIME_SIZE
      assert(nhops < list->_size);
#endif
    }
    assert(i == list->_head);
    assert(!j);
#if DFK_LIST_CONSTANT_TIME_SIZE
    assert(nhops + 1 == list->_size);
#endif
  }

#if DFK_DEBUG
  {
    /*
     * assert that all elements have correct .list property
     * we have check head -> tail traversal before, so the `while' loop
     * will eventually end
     */
    dfk_list_hook_t* i = list->_head;
    while (i) {
      assert(i->_list == list);
      i = i->_next;
    }
  }
#endif
}

static void dfk__list_hook_check_invariants(dfk_list_hook_t* hook)
{
  assert(hook);
  {
    /*
     * Traverse forward using _next property
     */
    dfk_list_hook_t* i = hook;
    dfk_list_hook_t* j = hook;
    while (i->_next) {
      i = i->_next;
      if (j) {
        j = j->_next;
      }
      if (j) {
        j = j->_next;
      }
      assert(i != j); /* loop detected */
    }
#if DFK_DEBUG
    if (hook->_list) {
      assert(i == hook->_list->_tail);
    }
#endif
  }
  {
    /*
     * Traverse backward using _prev property
     */
    dfk_list_hook_t* i = hook;
    dfk_list_hook_t* j = hook;
    while (i->_prev) {
      i = i->_prev;
      if (j) {
        j = j->_prev;
      }
      if (j) {
        j = j->_prev;
      }
      assert(i != j); /* loop detected */
    }
#if DFK_DEBUG
    if (hook->_list) {
      assert(i == hook->_list->_head);
    }
#endif
  }
#if DFK_DEBUG
  if (hook->_list) {
    dfk__list_check_invariants(hook->_list);
  }
#endif
}

static void dfk__list_it_check_invariants(dfk_list_it* it)
{
  assert(it);
  if (it->value) {
    dfk__list_hook_check_invariants(it->value);
    DFK_IF_DEBUG(assert(it->_list == it->value->_list));
  }
  DFK_IF_DEBUG(dfk__list_check_invariants(it->_list));
}

static void dfk__list_rit_check_invariants(dfk_list_rit* rit)
{
  assert(rit);
  if (rit->value) {
    dfk__list_hook_check_invariants(rit->value);
    DFK_IF_DEBUG(assert(rit->_list == rit->value->_list));
  }
  DFK_IF_DEBUG(dfk__list_check_invariants(rit->_list));
}

#define DFK_LIST_CHECK_INVARIANTS(list) \
  dfk__list_check_invariants((list))

#define DFK_LIST_HOOK_CHECK_INVARIANTS(hook) \
  dfk__list_hook_check_invariants((hook))

#define DFK_LIST_IT_CHECK_INVARIANTS(it) \
  dfk__list_it_check_invariants((it))

#define DFK_LIST_RIT_CHECK_INVARIANTS(rit) \
  dfk__list_rit_check_invariants(rit)

#endif /* NDEBUG */


void dfk_list_init(dfk_list_t* list)
{
  assert(list);
  list->_head = NULL;
  list->_tail = NULL;
#if DFK_LIST_CONSTANT_TIME_SIZE
  list->_size = 0;
#endif
  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_hook_init(dfk_list_hook_t* hook)
{
  assert(hook);
  hook->_next = NULL;
  hook->_prev = NULL;
  DFK_IF_DEBUG(hook->_list = NULL);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
}


void dfk_list_append(dfk_list_t* list, dfk_list_hook_t* hook)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
  DFK_IF_DEBUG(assert(!hook->_list));

  hook->_next = NULL;
  hook->_prev = list->_tail;
  DFK_IF_DEBUG(hook->_list = list);
  if (list->_tail) {
    list->_tail->_next = hook;
  }
  if (!list->_head) {
    list->_head = hook;
  }
  list->_tail = hook;
#if DFK_LIST_CONSTANT_TIME_SIZE
  ++list->_size;
#endif

  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
}


void dfk_list_prepend(dfk_list_t* list, dfk_list_hook_t* hook)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
  DFK_IF_DEBUG(assert(!hook->_list));

  hook->_prev = NULL;
  hook->_next = list->_head;
  DFK_IF_DEBUG(hook->_list = list);

  if (list->_head) {
    list->_head->_prev = hook;
  }
  list->_head = hook;
  if (!list->_tail) {
    list->_tail = hook;
  }
#if DFK_LIST_CONSTANT_TIME_SIZE
  ++list->_size;
#endif

  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
}


void dfk_list_insert(dfk_list_t* list, dfk_list_hook_t* hook,
    dfk_list_it* position)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
  DFK_LIST_IT_CHECK_INVARIANTS(position);
#if DFK_LIST_MEMORY_OPTIMIZED
  /** @todo implement */
#else
  dfk_list_hook_t* before = NULL; /* Points at the element before `position' */
  if (position->value) {
    before = position->value->_prev;
    position->value->_prev = hook;
  } else if (list->_tail) {
    /*
     * Iterator `positition' points at end, list is not empty,
     * therefore, we append to the end
     */
    before = list->_tail;
  }
  if (before) {
    before->_next = hook;
  }
  hook->_prev = before;
  hook->_next = position->value;
  if (!hook->_prev) {
    list->_head = hook;
  }
  if (!hook->_next) {
    list->_tail = hook;
  }
#endif
#if DFK_LIST_CONSTANT_TIME_SIZE
  list->_size++;
#endif
  DFK_IF_DEBUG(hook->_list = list);
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
  DFK_LIST_IT_CHECK_INVARIANTS(position);
}


void dfk_list_rinsert(dfk_list_t* list, dfk_list_hook_t* hook,
    dfk_list_rit* position)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
  DFK_LIST_RIT_CHECK_INVARIANTS(position);
#if DFK_LIST_MEMORY_OPTIMIZED
  /** @todo implement */
#else
  dfk_list_hook_t* before = NULL; /* Points at the element before `position' */
  if (position->value) {
    before = position->value->_next;
    position->value->_next = hook;
  } else if (list->_tail) {
    /*
     * Iterator `positition' points at end, list is not empty,
     * therefore, we append to the end
     */
    before = list->_head;
  }
  if (before) {
    before->_prev = hook;
  }
  hook->_next = before;
  hook->_prev = position->value;
  if (!hook->_prev) {
    list->_head = hook;
  }
  if (!hook->_next) {
    list->_tail = hook;
  }
#endif
#if DFK_LIST_CONSTANT_TIME_SIZE
  list->_size++;
#endif
  DFK_IF_DEBUG(hook->_list = list);
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
  DFK_LIST_RIT_CHECK_INVARIANTS(position);
}


void dfk_list_clear(dfk_list_t* list)
{
  DFK_LIST_CHECK_INVARIANTS(list);
#if DFK_DEBUG
  dfk_list_hook_t* i = list->_head;
  while (i) {
    dfk_list_hook_t* next = i->_next;
    i->_list = DFK_PDEADBEEF;
    i->_prev = DFK_PDEADBEEF;
    i->_next = DFK_PDEADBEEF;
    i = next;
  }
#endif
  list->_head = NULL;
  list->_tail = NULL;
#if DFK_LIST_CONSTANT_TIME_SIZE
  list->_size = 0;
#endif
  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_erase(dfk_list_t* list, dfk_list_it* it)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_IT_CHECK_INVARIANTS(it);
  DFK_IF_DEBUG(assert(it->value->_list == list));

  assert(list->_head);
  assert(list->_tail);
  assert(dfk_list_size(list));

  dfk_list_hook_t* hook = it->value;

  /* re-build the list */
  if (list->_head == list->_tail) {
    /* The list of size 1 */
    assert(list->_head == hook);
    assert(list->_size == 1);
    list->_head = NULL;
    list->_tail = NULL;
  } else if (!hook->_prev) {
    /* Remove from head */
    assert(list->_head == hook);
    assert(list->_head->_next);
    hook->_next->_prev = NULL;
    list->_head = list->_head->_next;
  } else if (!hook->_next) {
    /* Remove from tail */
    assert(list->_tail == hook);
    assert(list->_tail->_prev);
    hook->_prev->_next = NULL;
    list->_tail = list->_tail->_prev;
  } else {
    /* Remove from middle */
    assert(list->_size > 2);
    assert(hook != list->_head);
    assert(hook != list->_tail);
    assert(hook->_prev);
    assert(hook->_next);
    hook->_prev->_next = hook->_next;
    hook->_next->_prev = hook->_prev;
  }

#if DFK_LIST_CONSTANT_TIME_SIZE
  list->_size--;
#endif

  /* release the hook */
#if DFK_DEBUG
  hook->_next = DFK_PDEADBEEF;
  hook->_prev = DFK_PDEADBEEF;
  hook->_list = DFK_PDEADBEEF;
#endif

  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_rerase(dfk_list_t* list, dfk_list_rit* rit)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_RIT_CHECK_INVARIANTS(rit);
  /* Convert reverse iterator to direct and call dfk_list_erase */
  dfk_list_it it = {.value = rit->value};
  DFK_IF_DEBUG(it._list = rit->_list);
  dfk_list_erase(list, &it);
  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_pop_front(dfk_list_t* list)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  assert(dfk_list_size(list));
  dfk_list_it begin;
  dfk_list_begin(list, &begin);
  dfk_list_erase(list, &begin);
  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_pop_back(dfk_list_t* list)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  assert(dfk_list_size(list));
  dfk_list_rit rbegin;
  dfk_list_rbegin(list, &rbegin);
  dfk_list_rerase(list, &rbegin);
  DFK_LIST_CHECK_INVARIANTS(list);
}


size_t dfk_list_size(dfk_list_t* list)
{
  DFK_LIST_CHECK_INVARIANTS(list);
#if DFK_LIST_CONSTANT_TIME_SIZE
  return list->_size;
#else
  dfk_list_it it, end;
  dfk_list_begin(list, &it);
  dfk_list_end(list, &end);
  size_t size = 0;
  while (!dfk_list_it_equal(&it, &end)) {
    ++size;
    dfk_list_it_next(&it);
  }
  return size;
#endif
}


int dfk_list_empty(dfk_list_t* list)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  return !list->_head;
}


size_t dfk_list_sizeof(void)
{
  return sizeof(dfk_list_t);
}


void dfk_list_begin(dfk_list_t* list, dfk_list_it* it)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  assert(it);
  it->value = list->_head;
  DFK_IF_DEBUG(it->_list = list);
  DFK_LIST_IT_CHECK_INVARIANTS(it);
}


void dfk_list_end(dfk_list_t* list, dfk_list_it* it)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  assert(it);
  it->value = NULL;
  DFK_IF_DEBUG(it->_list = list);
  DFK_LIST_IT_CHECK_INVARIANTS(it);
}


void dfk_list_it_next(dfk_list_it* it)
{
  DFK_LIST_IT_CHECK_INVARIANTS(it);
  it->value = it->value->_next;
  DFK_LIST_IT_CHECK_INVARIANTS(it);
}


int dfk_list_it_equal(dfk_list_it* lhs, dfk_list_it* rhs)
{
  DFK_LIST_IT_CHECK_INVARIANTS(lhs);
  DFK_LIST_IT_CHECK_INVARIANTS(rhs);
  DFK_IF_DEBUG(assert(lhs->_list == rhs->_list));
  return lhs->value == rhs->value;
}


size_t dfk_list_it_sizeof(void)
{
  return sizeof(dfk_list_it);
}


void dfk_list_rbegin(dfk_list_t* list, dfk_list_rit* rit)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  assert(rit);
  rit->value = list->_tail;
  DFK_IF_DEBUG(rit->_list = list);
  DFK_LIST_RIT_CHECK_INVARIANTS(rit);
}


void dfk_list_rend(dfk_list_t* list, dfk_list_rit* rit)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  assert(rit);
  rit->value = NULL;
  DFK_IF_DEBUG(rit->_list = list);
  DFK_LIST_RIT_CHECK_INVARIANTS(rit);
}


void dfk_list_rit_next(dfk_list_rit* rit)
{
  DFK_LIST_RIT_CHECK_INVARIANTS(rit);
  rit->value = rit->value->_prev;
  DFK_LIST_RIT_CHECK_INVARIANTS(rit);
}


int dfk_list_rit_equal(dfk_list_rit* lhs, dfk_list_rit* rhs)
{
  DFK_LIST_RIT_CHECK_INVARIANTS(lhs);
  DFK_LIST_RIT_CHECK_INVARIANTS(rhs);
  DFK_IF_DEBUG(assert(lhs->_list == rhs->_list));
  return lhs->value == rhs->value;
}


size_t dfk_list_rit_sizeof(void)
{
  return sizeof(dfk_list_rit);
}

