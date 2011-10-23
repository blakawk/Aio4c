/*
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
/**
 * @file aio4c/list.h
 * @brief Double-linked list implementation.
 */
#ifndef __AIO4C_LIST_H__
#define __AIO4C_LIST_H__

#include <aio4c/types.h>

/**
 * @struct s_Node
 * @brief Represents a List's Node.
 */
/**
 * @def __AIO4C_NODE_DEFINED__
 * @brief Defined when Node's type was defined.
 */
#ifndef __AIO4C_NODE_DEFINED__
#define __AIO4C_NODE_DEFINED__
typedef struct s_Node Node;
struct s_Node  {
   void*          data; /**< Pointer to the data held by the Node. */
   struct s_Node* next; /**< Pointer to this Node's next Node. */
   struct s_Node* prev; /**< Pointer to this Node's previous Node. */
};
#endif /* __AIO4C_NODE_DEFINED__ */

/**
 * @struct s_List
 * @brief Represents a double-linked List.
 */
/**
 * @def __AIO4C_LIST_DEFINED__
 * @brief Defined when List's type was defined.
 */
#ifndef __AIO4C_LIST_DEFINED__
#define __AIO4C_LIST_DEFINED__
typedef struct s_List List;
struct s_List {
    Node* first; /**< This List's first Node. */
    Node* last;  /**< This List's last Node. */
};
#endif /* __AIO4C_LIST_dEFINED__ */

/**
 * @def AIO4C_LIST_INITIALIZER(list)
 * @brief Initializes a List.
 *
 * @param list
 *   Pointer to the list to initialize.
 */
#define AIO4C_LIST_INITIALIZER(list) { \
    (list)->first = NULL;              \
    (list)->last  = NULL;              \
}

/**
 * @fn Node* NewNode(void*)
 * @brief Creates a new List's Node.
 *
 * @param data
 *   Pointer to the data to be held by the new Node.
 * @return
 *   Pointer to the new Node.
 */
extern AIO4C_API Node* NewNode(void* data);

/**
 * @fn void FreeNode(Node**)
 * @brief Frees a Node.
 *
 * Once the memory used by the Node has been freed correctly, the referenced pointer
 * will be set to NULL to disallow any furthur use.
 *
 * @param pNode
 *   A pointer to a pointer referencing a Node.
 */
extern AIO4C_API void FreeNode(Node** pNode);

/**
 * @fn void ListAddAfter(List*,Node*,Node*)
 * @brief Adds a Node in a List.
 *
 * Parameters should be allocated before calling this function. The newNode will be added
 * after the referenced node in the referenced list. If the node does not have any newt
 * Node, then the newNode will be the last of the List.
 *
 * @param list
 *   A pointer to the List to add the Node to.
 * @param node
 *   A pointer to the Node belonging to the referenced list to add the newNode after.
 * @param newNode
 *   The Node to add to the List.
 */
extern AIO4C_API void ListAddAfter(List* list, Node* node, Node* newNode);

/**
 * @fn void ListAddBefore(List*,Node*,Node*)
 * @brief Adds a Node in a List.
 *
 * Parameters should be allocated before calling this function. The newNode will be added
 * before the referenced node in the referenced list. If the node does not have any previous
 * Node, then the newNode will be the first of the List.
 *
 * @param list
 *   A pointer to the List to add the Node to.
 * @param node
 *   A pointer to the Node belonging to the referenced list to add the newNode before.
 * @param newNode
 *   A pointer to the Node to add to the List.
 */
extern AIO4C_API void ListAddBefore(List* list, Node* node, Node* newNode);

/**
 * @fn void ListAddFirst(List*,Node*)
 * @brief Add a Node in the beginning of the List.
 *
 * If the List is empty, then the newNode will be the first and the last one of the List.
 * Else, the Node will be added before the first Node of the List using ListAddBefore.
 * Consistency checks are performed, and SIGABRT will be called if:
 * <ul>
 *   <li>the newNode is NULL,</li>
 *   <li>the newNode's data is NULL,</li>
 *   <li>after the add operation, the first Node is not the one added,</li>
 *   <li>or if there is a previous Node to the one added.</li>
 * </ul>
 *
 * @param list
 *   Pointer to the List to add the Node on.
 * @param newNode
 *   Pointer to the Node to add to the List.
 */
extern AIO4C_API void ListAddFirst(List* list, Node* newNode);

/**
 * @fn void ListAddLast(List*,Node*)
 * @brief Add a Node in the end of the List.
 *
 * If the List is empty, then the newNode will be the first and the last one of the List.
 * Else, the Node will be added after the last Node of the List using ListAddAfter.
 * Consistency checks are performed, and SIGABRT will be called if:
 * <ul>
 *   <li>the newNode is NULL,</li>
 *   <li>the newNode's data is NULL,</li>
 *   <li>after the add operation, the last List's Node is not the one added,</li>
 *   <li>or the next Node of the one added is not NULL.</li>
 * </ul>
 *
 * @param list
 *   Pointer to the List ot add the Node on.
 * @param newNode
 *   Pointer to the Node to add to the List.
 */
extern AIO4C_API void ListAddLast(List* list, Node* newNode);

/**
 * @fn Node* ListPop(List*)
 * @brief Removes the List's first Node.
 *
 * If the List is empty, NULL is returned. If the Node's previous is not NULL, then
 * SIGABRT is raised. After thoses consistency checks, the Node is removed from the
 * List using ListRemove. Once it is done, some other checks are performed, and SIGABRT
 * is raised if they fail:
 * <ul>
 *   <li>if the List first Node did not changed,</li>
 *   <li>if the Node's previous is not NULL,</li>
 *   <li>if the Node's next is not NULL,</li>
 *   <li>if the List's last Node is the one removed.</li>
 * </ul>.
 *
 * @param list
 *   Pointer to the List from which a Node should be removed.
 * @return
 *   Pointer to the removed Node, or NULL if the List is empty.
 */
extern AIO4C_API Node* ListPop(List* list);

/**
 * @fn aio4c_bool_t ListEmpty(List*)
 * @brief Determines if a List is empty.
 *
 * A List is determined empty if there is neither a first Node nor a las Node.
 *
 * @param list
 *   Pointer to the List.
 * @return
 *   <code>true</code> if the List is empty, else <code>false</code>.
 */
extern AIO4C_API aio4c_bool_t ListEmpty(List* list);

/**
 * @fn void ListRemove(List*,Node*)
 * @brief Remove a Node from a List.
 *
 * The node to remove must belong to the list. Consistency checks are performed: if the Node
 * does not have a previous, then it should be the first of the List. In the same way, if the
 * Node does not have a next, then it should be the last of the List. If thoses checks fail,
 * SIGABRT is raised.
 *
 * @param list
 *   Pointer to the List to remove the Node from.
 * @param node
 *   Pointer to the Node to remove from the List.
 */
extern AIO4C_API void ListRemove(List* list, Node* node);

#endif /* __AIO4C_LIST_H__ */
