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
#include <aio4c/alloc.h>
#include <aio4c/list.h>
#include <aio4c/queue.h>
#include <aio4c/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct s_Data {
    int a;
    int b;
} Data;

aio4c_bool_t aio4c_remove(QueueItem* item, Data* data) {
    Data* d = item->content.data;

    if (d->a > data->a && d->a <= data->b) {
        aio4c_free(d);
        return true;
    }

    return false;
}

#define COUNT 10000

int main(void) {
    QueueItem item;
    Data* a = NULL;
    Node* node = NULL;
    int i = 0, count = 0, j = 0, b = 0, d = 0;

    srand(getpid());

    Data* c = aio4c_malloc(sizeof(Data));
    Queue* queue = NewQueue();

    for (j = 1; j <= 100; j++) {
        b = (rand() % COUNT) + 1;
        c->a = rand() % b;
        c->b = c->a + rand() % (b - c->a);
        d = rand();

        for (i = 0; i < b; i++) {
            count++;
            a = aio4c_malloc(sizeof(Data));
            a->a = i;
            a->b = d;
            EnqueueDataItem(queue, a);
        }

        node = queue->busy.first;
        i = 0;
        while (node != NULL) {
            if (((Data*)(((QueueItem*)node->data)->content.data))->a != i) {
                printf("!");
                fflush(stdout);
            }
            i++;
            node = node->next;
        }

        RemoveAll(queue, aio4c_remove_callback(aio4c_remove), aio4c_remove_discriminant(c));

        count -= c->b - c->a;

        for (i = b - c->b + c->a; i < COUNT; i++) {
            count++;
            a = aio4c_malloc(sizeof(Data));
            a->a = i;
            a->b = d;
            EnqueueDataItem(queue, a);
        }

        while (Dequeue(queue, &item, false)) {
            a = item.content.data;
            if (a->b != d) {
                printf("!");
                fflush(stdout);
            }
            aio4c_free(a);
            count--;
        }

        if (count == 0) {
            printf(".");
            fflush(stdout);
        } else {
            printf("!");
            fflush(stdout);
        }
    }

    printf("\n");
    FreeQueue(&queue);
    aio4c_free(c);

    return 0;
}
