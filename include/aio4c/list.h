/**
 * Copyright © 2011 blakawk <blakawk@gentooist.com>
 * All rights reserved.  Released under GPLv3 License.
 *
 * This program is free software: you can redistribute
 * it  and/or  modify  it under  the  terms of the GNU.
 * General  Public  License  as  published by the Free
 * Software Foundation, version 3 of the License.
 *
 * This  program  is  distributed  in the hope that it
 * will be useful, but  WITHOUT  ANY WARRANTY; without
 * even  the  implied  warranty  of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See   the  GNU  General  Public  License  for  more
 * details. You should have received a copy of the GNU
 * General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 **/
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

extern Node* NewNode(void* data);

extern void FreeNode(Node** pNode);

extern void ListAddAfter(List* list, Node* node, Node* newNode);

extern void ListAddBefore(List* list, Node* node, Node* newNode);

extern void ListAddFirst(List* list, Node* newNode);

extern void ListAddLast(List* list, Node* newNode);

extern Node* ListPop(List* list);

extern aio4c_bool_t ListEmpty(List* list);

extern void ListRemove(List* list, Node* node);

#endif