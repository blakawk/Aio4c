/**
 * Copyright (c) 2011 blakawk
 *
 * This file is part of Aio4c <http://aio4c.so>.
 *
 * Aio4c <http://aio4c.so> is free software: you
 * can  redistribute  it  and/or modify it under
 * the  terms  of the GNU General Public License
 * as published by the Free Software Foundation,
 * version 3 of the License.
 *
 * Aio4c <http://aio4c.so> is distributed in the
 * hope  that it will be useful, but WITHOUT ANY
 * WARRANTY;  without  even the implied warranty
 * of   MERCHANTABILITY   or   FITNESS   FOR   A
 * PARTICULAR PURPOSE.
 *
 * See  the  GNU General Public License for more
 * details.  You  should have received a copy of
 * the  GNU  General  Public  License along with
 * Aio4c    <http://aio4c.so>.   If   not,   see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef __AIO4C_LIST_H__
#define __AIO4C_LIST_H__

#include <aio4c/types.h>

typedef struct s_Node {
    void*          data;
    struct s_Node* next;
    struct s_Node* prev;
} Node;

typedef struct s_List {
    Node*        first;
    Node*        last;
} List;

#define AIO4C_LIST_INITIALIZER(list) { \
    (list)->first = NULL;              \
    (list)->last  = NULL;              \
}

extern AIO4C_API Node* NewNode(void* data);

extern AIO4C_API void FreeNode(Node** pNode);

extern AIO4C_API void ListAddAfter(List* list, Node* node, Node* newNode);

extern AIO4C_API void ListAddBefore(List* list, Node* node, Node* newNode);

extern AIO4C_API void ListAddFirst(List* list, Node* newNode);

extern AIO4C_API void ListAddLast(List* list, Node* newNode);

extern AIO4C_API Node* ListPop(List* list);

extern AIO4C_API aio4c_bool_t ListEmpty(List* list);

extern AIO4C_API void ListRemove(List* list, Node* node);

#endif
