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
#include <aio4c/list.h>

#include <aio4c/alloc.h>
#include <aio4c/error.h>
#include <aio4c/log.h>
#include <aio4c/types.h>

#ifndef AIO4C_WIN32
#include <errno.h>
#endif /* AIO4C_WIN32 */

Node* NewNode(void* data) {
    Node* node = NULL;
    ErrorCode code = AIO4C_ERROR_CODE_INITIALIZER;

    if ((node = aio4c_malloc(sizeof(Node))) == NULL) {
#ifndef AIO4C_WIN32
        code.error = errno;
#else /* AIO4C_WIN32 */
        code.source = AIO4C_ERRNO_SOURCE_SYS;
#endif /* AIO4C_WIN32 */
        code.size = sizeof(Node);
        code.type = "Node";
        Raise(AIO4C_LOG_LEVEL_ERROR, AIO4C_ALLOC_ERROR_TYPE, AIO4C_ALLOC_ERROR, &code);
        return NULL;
    }

    node->next = NULL;
    node->prev = NULL;
    node->data = data;

    return node;
}

void FreeNode(Node** pNode) {
    Node* node = NULL;

    if (pNode != NULL && (node = *pNode) != NULL) {
        aio4c_free(node);
        *pNode = NULL;
    }
}

void ListAddAfter(List* list, Node* node, Node* newNode) {
    newNode->prev = node;
    newNode->next = node->next;

    if (node->next == NULL) {
        list->last = newNode;
    } else {
        node->next->prev = newNode;
    }

    node->next = newNode;
}

void ListAddBefore(List* list, Node* node, Node* newNode) {
    newNode->prev = node->prev;
    newNode->next = node;

    if (node->prev == NULL) {
        list->first = newNode;
    } else {
        node->prev->next = newNode;
    }

    node->prev = newNode;
}

void ListAddFirst(List* list, Node* newNode) {
    if (list->first == NULL) {
        list->first = newNode;
        list->last = newNode;
        newNode->prev = NULL;
        newNode->next = NULL;
    } else {
        ListAddBefore(list, list->first, newNode);
    }
}

void ListAddLast(List* list, Node* newNode) {
    if (list->last == NULL) {
        ListAddFirst(list, newNode);
    } else {
        ListAddAfter(list, list->last, newNode);
    }
}

Node* ListPop(List* list) {
    Node* node = list->first;
    ListRemove(list, list->first);
    return node;
}

aio4c_bool_t ListEmpty(List* list) {
    return (list->first == NULL);
}

void ListRemove(List* list, Node* node) {
    if (node->prev == NULL) {
        list->first = node->next;
    } else {
        node->prev->next = node->next;
    }

    if (node->next == NULL) {
        list->last = node->prev;
    } else {
        node->next->prev = node->prev;
    }
}
