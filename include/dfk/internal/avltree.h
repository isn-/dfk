/**
 * @file dfk/internal/avltree.h
 * Balanced tree data structure
 *
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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <dfk/config.h>

typedef struct dfk_avltree_hook_t {
  struct dfk_avltree_hook_t* left;
  struct dfk_avltree_hook_t* right;
  struct dfk_avltree_hook_t* parent;
  signed char bal;
#if DFK_DEBUG
  struct dfk_avltree_t* tree;
#endif
} dfk_avltree_hook_t;

typedef int (*dfk_avltree_cmp)(dfk_avltree_hook_t*, dfk_avltree_hook_t*);
typedef int (*dfk_avltree_lookup_cmp)(dfk_avltree_hook_t*, void*);

typedef struct dfk_avltree_t {
  dfk_avltree_hook_t* root;
  dfk_avltree_cmp cmp;
} dfk_avltree_t;


typedef struct dfk_avltree_it_t {
  dfk_avltree_hook_t* value;
} dfk_avltree_it_t;

void dfk_avltree_init(dfk_avltree_t* tree, dfk_avltree_cmp cmp);
void dfk_avltree_free(dfk_avltree_t* tree);

void dfk_avltree_hook_init(dfk_avltree_hook_t* h);
void dfk_avltree_hook_free(dfk_avltree_hook_t* h);

dfk_avltree_hook_t* dfk_avltree_insert(dfk_avltree_t* tree, dfk_avltree_hook_t* e);
void dfk_avltree_erase(dfk_avltree_t* tree, dfk_avltree_hook_t* e);
dfk_avltree_hook_t* dfk_avltree_lookup(dfk_avltree_t* tree, void* e, dfk_avltree_lookup_cmp cmp);

void dfk_avltree_it_init(dfk_avltree_t* tree, dfk_avltree_it_t* it);
void dfk_avltree_it_free(dfk_avltree_it_t* it);
void dfk_avltree_it_next(dfk_avltree_it_t* it);
int dfk_avltree_it_valid(dfk_avltree_it_t* it);

