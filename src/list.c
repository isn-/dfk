/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <string.h>
#include <assert.h>
#include <dfk/internal.h>
#include <dfk/list.h>

#define DFK_PTR_XOR(x, y) ((void*) ((ptrdiff_t) (x) ^ (ptrdiff_t) (y)))
#define DFK_PTR_XOR3(x, y, z) \
  ((void*) ((ptrdiff_t) (x) ^ (ptrdiff_t) (y) ^ (ptrdiff_t) (z)))

#ifdef NDEBUG

#define DFK_LIST_CHECK_INVARIANTS(list) DFK_UNUSED(list)
#define DFK_LIST_HOOK_CHECK_INVARIANTS(hook) DFK_UNUSED(hook)
#define DFK_LIST_IT_CHECK_INVARIANTS(it) DFK_UNUSED(it)
#define DFK_LIST_RIT_CHECK_INVARIANTS(rit) DFK_UNUSED(rit)

#else

#if DFK_LIST_CONSTANT_TIME_SIZE
#define DFK_IF_LIST_CONSTANT_TIME_SIZE(expr) (expr)
#else
#define DFK_IF_LIST_CONSTANT_TIME_SIZE(expr)
#endif

#if DFK_LIST_MEMORY_OPTIMIZED
#define DFK_IF_LIST_MEMORY_OPTIMIZED(expr) (expr)
#else
#define DFK_IF_LIST_MEMORY_OPTIMIZED(expr)
#endif

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
    DFK_IF_LIST_CONSTANT_TIME_SIZE(assert(list->_size == 0));
    return;
  }

  /*
   * List of size 1
   */
  if (list->_head == list->_tail) {
#if DFK_LIST_MEMORY_OPTIMIZED
    assert(list->_head->_p == NULL);
#else
    assert(!list->_head->_next);
    assert(!list->_head->_prev);
#endif
    DFK_IF_LIST_CONSTANT_TIME_SIZE(assert(list->_size == 1));
    DFK_IF_DEBUG(assert(list->_head->_list == list));
    return;
  }

#if !DFK_LIST_MEMORY_OPTIMIZED
  /*
   * For non-empty list check that list._head._prev and
   * list._tail._next are NULL
   */
  assert(!list->_head->_prev);
  assert(!list->_tail->_next);
#endif

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
#if DFK_LIST_MEMORY_OPTIMIZED
    dfk_list_hook_t* pi = NULL;
    dfk_list_hook_t* pj = NULL;
#endif
    /* while (i) will not detect case 3) */
    while (i != list->_tail) {
      assert(i); /* List ended unexpectedly */
#if DFK_LIST_MEMORY_OPTIMIZED
      dfk_list_hook_t* nextpi = i;
      i = DFK_PTR_XOR(pi, i->_p);
      pi = nextpi;
#else
      i = i->_next;
#endif
      for (size_t c = 0; c < 2; ++c) {
        if (j) {
#if DFK_LIST_MEMORY_OPTIMIZED
          dfk_list_hook_t* nextpj = j;
          j = DFK_PTR_XOR(pj, j->_p);
          pj = nextpj;
#else
          j = j->_next;
#endif
        }
      }
      assert(i != j); /* loop detected */
      ++nhops;
      DFK_IF_LIST_CONSTANT_TIME_SIZE(assert(nhops < list->_size));
    }
    assert(i == list->_tail);
    assert(!j);
    DFK_IF_LIST_CONSTANT_TIME_SIZE(assert(nhops + 1 == list->_size));
  }

  {
    /*
     * check tail -> head traversal, using the same technique
     * as for head -> tail
     */
    size_t nhops = 0;
    dfk_list_hook_t* i = list->_tail;
    dfk_list_hook_t* j = list->_tail;
#if DFK_LIST_MEMORY_OPTIMIZED
    dfk_list_hook_t* pi = NULL;
    dfk_list_hook_t* pj = NULL;
#endif
    /* while (i) will not detect case 3) */
    while (i != list->_head) {
      assert(i); /* List ended unexpectedly */
#if DFK_LIST_MEMORY_OPTIMIZED
      dfk_list_hook_t* nextpi = i;
      i = DFK_PTR_XOR(pi, i->_p);
      pi = nextpi;
#else
      i = i->_prev;
#endif
      for (size_t c = 0; c < 2; ++c) {
        if (j) {
#if DFK_LIST_MEMORY_OPTIMIZED
          dfk_list_hook_t* nextpj = j;
          j = DFK_PTR_XOR(pj, j->_p);
          pj = nextpj;
#else
          j = j->_prev;
#endif
        }
      }
      assert(i != j); /* loop detected */
      ++nhops;
      DFK_IF_LIST_CONSTANT_TIME_SIZE(assert(nhops < list->_size));
    }
    assert(i == list->_head);
    assert(!j);
    DFK_IF_LIST_CONSTANT_TIME_SIZE(assert(nhops + 1 == list->_size));
  }

#if DFK_DEBUG
  {
    /*
     * Assert that all list elements have correct .list property.
     * We have checked head -> tail traversal before, so the `while' loop
     * will eventually end.
     */
#if DFK_LIST_MEMORY_OPTIMIZED
    dfk_list_hook_t* p = NULL;
    dfk_list_hook_t* i = list->_head;
    while (i) {
      assert(i->_list == list);
      dfk_list_hook_t* nextp = i;
      i = DFK_PTR_XOR(i->_p, p);
      p = nextp;
    }
#else
    dfk_list_hook_t* i = list->_head;
    while (i) {
      assert(i->_list == list);
      i = i->_next;
    }
#endif /* DFK_LIST_MEMORY_OPTIMIZED */
  }
#endif /* DFK_DEBUG */
}

static void dfk__list_hook_check_invariants(dfk_list_hook_t* hook)
{
  assert(hook);
#if !DFK_LIST_MEMORY_OPTIMIZED
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
#endif /* DFK_DEBUG */
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
#endif /* DFK_DEBUG */
  }
#endif /* !DFK_LIST_MEMORY_OPTIMIZED */

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
  DFK_IF_LIST_CONSTANT_TIME_SIZE(list->_size = 0);
  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_hook_init(dfk_list_hook_t* hook)
{
  assert(hook);
#if DFK_LIST_MEMORY_OPTIMIZED
  hook->_p = NULL;
#else
  hook->_next = NULL;
  hook->_prev = NULL;
#endif
  DFK_IF_DEBUG(hook->_list = NULL);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
}


void dfk_list_append(dfk_list_t* list, dfk_list_hook_t* hook)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
  DFK_IF_DEBUG(assert(!hook->_list));

#if DFK_LIST_MEMORY_OPTIMIZED
  if (list->_tail) {
    hook->_p = list->_tail;
    list->_tail->_p = DFK_PTR_XOR(list->_tail->_p, hook);
  }
#else
  hook->_next = NULL;
  hook->_prev = list->_tail;
  if (list->_tail) {
    list->_tail->_next = hook;
  }
#endif

  if (!list->_head) {
    list->_head = hook;
  }
  list->_tail = hook;
  DFK_IF_DEBUG(hook->_list = list);
  DFK_IF_LIST_CONSTANT_TIME_SIZE(++list->_size);

  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
}


void dfk_list_prepend(dfk_list_t* list, dfk_list_hook_t* hook)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
  DFK_IF_DEBUG(assert(!hook->_list));

#if DFK_LIST_MEMORY_OPTIMIZED
  if (list->_head) {
    hook->_p = list->_head;
    list->_head->_p = DFK_PTR_XOR(list->_head->_p, hook);
  }
#else
  hook->_prev = NULL;
  hook->_next = list->_head;
  if (list->_head) {
    list->_head->_prev = hook;
  }
#endif

  list->_head = hook;
  if (!list->_tail) {
    list->_tail = hook;
  }
  DFK_IF_DEBUG(hook->_list = list);
  DFK_IF_LIST_CONSTANT_TIME_SIZE(++list->_size);

  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
}


void dfk_list_insert(dfk_list_t* list, dfk_list_hook_t* hook,
    dfk_list_it* position)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
  DFK_LIST_IT_CHECK_INVARIANTS(position);

  dfk_list_hook_t* before = NULL; /* Points at the element before `position' */
  if (position->value) {
#if DFK_LIST_MEMORY_OPTIMIZED
    before = position->_prev;
    position->value->_p = DFK_PTR_XOR3(position->value->_p, before, hook);
#else
    before = position->value->_prev;
    position->value->_prev = hook;
#endif
  } else if (list->_tail) {
    /*
     * Iterator `position' points at end, list is not empty,
     * therefore, we append to the end
     */
    before = list->_tail;
  }
  if (before) {
#if DFK_LIST_MEMORY_OPTIMIZED
    before->_p = DFK_PTR_XOR3(before->_p, hook, position->value);
#else
    before->_next = hook;
#endif
  }
#if DFK_LIST_MEMORY_OPTIMIZED
  hook->_p = DFK_PTR_XOR(before, position->value);
#else
  hook->_prev = before;
  hook->_next = position->value;
#endif
  if (!before) {
    list->_head = hook;
  }
  if (!position->value) {
    list->_tail = hook;
  }

  DFK_IF_DEBUG(hook->_list = list);
  DFK_IF_LIST_CONSTANT_TIME_SIZE(list->_size++);

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

  dfk_list_hook_t* before = NULL; /* Points at the element before `position' */
  if (position->value) {
#if DFK_LIST_MEMORY_OPTIMIZED
    before = position->_prev;
    position->value->_p = DFK_PTR_XOR3(position->value->_p, before, hook);
#else
    before = position->value->_next;
    position->value->_next = hook;
#endif
  } else if (list->_tail) {
    /*
     * Iterator `position' points at end, list is not empty,
     * therefore, we append to the end
     */
    before = list->_head;
  }
  if (before) {
#if DFK_LIST_MEMORY_OPTIMIZED
    before->_p = DFK_PTR_XOR3(before->_p, hook, position->value);
#else
    before->_prev = hook;
#endif
  }
#if DFK_LIST_MEMORY_OPTIMIZED
  hook->_p = DFK_PTR_XOR(before, position->value);
#else
  hook->_next = before;
  hook->_prev = position->value;
#endif
  if (!position->value) {
    list->_head = hook;
  }
  if (!before) {
    list->_tail = hook;
  }

  DFK_IF_DEBUG(hook->_list = list);
  DFK_IF_LIST_CONSTANT_TIME_SIZE(list->_size++);

  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_HOOK_CHECK_INVARIANTS(hook);
  DFK_LIST_RIT_CHECK_INVARIANTS(position);
}


void dfk_list_clear(dfk_list_t* list)
{
  DFK_LIST_CHECK_INVARIANTS(list);

#if DFK_DEBUG
  /*
   * Can not use iterator at this point because
   * list invariants are violated during the loop
   */
#if DFK_LIST_MEMORY_OPTIMIZED
  dfk_list_hook_t* prev = NULL;
  dfk_list_hook_t* i = list->_head;
  while (i) {
    dfk_list_hook_t* next = DFK_PTR_XOR(i->_p, prev);
    prev = i;
    i->_list = DFK_PDEADBEEF;
    i->_p = DFK_PDEADBEEF;
    i = next;
  }
#else
  dfk_list_hook_t* i = list->_head;
  while (i) {
    dfk_list_hook_t* next = i->_next;
    i->_list = DFK_PDEADBEEF;
    i->_prev = DFK_PDEADBEEF;
    i->_next = DFK_PDEADBEEF;
    i = next;
  }
#endif /* DFK_LIST_MEMORY_OPTIMIZED */
#endif /* DFK_DEBUG */

  list->_head = NULL;
  list->_tail = NULL;

  DFK_IF_LIST_CONSTANT_TIME_SIZE(list->_size = 0);

  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_erase(dfk_list_t* list, dfk_list_it* it)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_IT_CHECK_INVARIANTS(it);
  DFK_IF_DEBUG(assert(it->value->_list == list));

  assert(dfk_list_size(list));

  dfk_list_hook_t* hook = it->value;

  if (list->_head == list->_tail) {
    /* The list of size 1 */
    assert(list->_head == hook);
    list->_head = NULL;
    list->_tail = NULL;
  } else if (hook == list->_head) {
    /*
     * The list of size > 1
     * Remove from head
     */
#if DFK_LIST_MEMORY_OPTIMIZED
    list->_head = hook->_p;
    list->_head->_p = DFK_PTR_XOR(list->_head->_p, hook);
#else
    assert(hook->_next);
    hook->_next->_prev = NULL;
    list->_head = hook->_next;
#endif
  } else if (hook == list->_tail) {
    /*
     * The list of size > 1
     * Remove from tail
     */
#if DFK_LIST_MEMORY_OPTIMIZED
    list->_tail = hook->_p;
    list->_tail->_p = DFK_PTR_XOR(list->_tail->_p, hook);
#else
    assert(hook->_prev);
    hook->_prev->_next = NULL;
    list->_tail = hook->_prev;
#endif
  } else {
    /*
     * The list of size > 2
     * Remove from middle
     */
    assert(hook != list->_head);
    assert(hook != list->_tail);
#if DFK_LIST_MEMORY_OPTIMIZED
    dfk_list_hook_t* next = DFK_PTR_XOR(hook->_p, it->_prev);
    assert(next);
    assert(it->_prev);
    next->_p = DFK_PTR_XOR3(next->_p, hook, it->_prev);
    it->_prev->_p = DFK_PTR_XOR3(it->_prev->_p, hook, next);
#else
    assert(hook->_prev);
    assert(hook->_next);
    hook->_prev->_next = hook->_next;
    hook->_next->_prev = hook->_prev;
#endif
  }

  DFK_IF_LIST_CONSTANT_TIME_SIZE(list->_size--);

  /* release the hook */
#if DFK_DEBUG
#if DFK_LIST_MEMORY_OPTIMIZED
  hook->_p = DFK_PDEADBEEF;
#else
  hook->_next = DFK_PDEADBEEF;
  hook->_prev = DFK_PDEADBEEF;
#endif
  hook->_list = DFK_PDEADBEEF;
#endif

  DFK_LIST_CHECK_INVARIANTS(list);
}


void dfk_list_rerase(dfk_list_t* list, dfk_list_rit* rit)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  DFK_LIST_RIT_CHECK_INVARIANTS(rit);
  /* Convert reverse iterator to direct and call dfk_list_erase */
  dfk_list_it it = {
#if DFK_LIST_MEMORY_OPTIMIZED
    ._prev = DFK_PTR_XOR(rit->_prev, rit->value),
#endif
    .value = rit->value
  };
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
  DFK_IF_LIST_MEMORY_OPTIMIZED(it->_prev = NULL);
  it->value = list->_head;
  DFK_IF_DEBUG(it->_list = list);
  DFK_LIST_IT_CHECK_INVARIANTS(it);
}


void dfk_list_end(dfk_list_t* list, dfk_list_it* it)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  assert(it);
  DFK_IF_LIST_MEMORY_OPTIMIZED(it->_prev = NULL);
  it->value = NULL;
  DFK_IF_DEBUG(it->_list = list);
  DFK_LIST_IT_CHECK_INVARIANTS(it);
}


void dfk_list_it_next(dfk_list_it* it)
{
  DFK_LIST_IT_CHECK_INVARIANTS(it);
#if DFK_LIST_MEMORY_OPTIMIZED
  dfk_list_hook_t* prev = it->value;
  it->value = DFK_PTR_XOR(it->_prev, it->value->_p);
  it->_prev = prev;
#else
  it->value = it->value->_next;
#endif
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
  DFK_IF_LIST_MEMORY_OPTIMIZED(rit->_prev = NULL);
  rit->value = list->_tail;
  DFK_IF_DEBUG(rit->_list = list);
  DFK_LIST_RIT_CHECK_INVARIANTS(rit);
}


void dfk_list_rend(dfk_list_t* list, dfk_list_rit* rit)
{
  DFK_LIST_CHECK_INVARIANTS(list);
  assert(rit);
  DFK_IF_LIST_MEMORY_OPTIMIZED(rit->_prev = NULL);
  rit->value = NULL;
  DFK_IF_DEBUG(rit->_list = list);
  DFK_LIST_RIT_CHECK_INVARIANTS(rit);
}


void dfk_list_rit_next(dfk_list_rit* rit)
{
  DFK_LIST_RIT_CHECK_INVARIANTS(rit);
#if DFK_LIST_MEMORY_OPTIMIZED
  dfk_list_hook_t* prev = rit->value;
  rit->value = DFK_PTR_XOR(rit->_prev, rit->value->_p);
  rit->_prev = prev;
#else
  rit->value = rit->value->_prev;
#endif
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

