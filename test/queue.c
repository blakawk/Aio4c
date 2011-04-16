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
#include <aio4c.h>
#include <aio4c/alloc.h>
#include <aio4c/list.h>
#include <aio4c/queue.h>
#include <aio4c/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define dprintf(a,...) \
    fprintf(stderr,a,__VA_ARGS__)

typedef struct s_Data {
    int a;
    int b;
} Data;

static int removed = 0;
static int COUNT = 1000;

aio4c_bool_t aio4c_remove(QueueItem* item, Data* data) {
    Data* d = item->content.data;

    dprintf("\t\ttesting %d\n", d->a);
    if (d->a > data->a && d->a <= data->b) {
        removed++;
        dprintf("\t\tremoving %d\n", d->a);
        aio4c_free(d);
        return true;
    }

    return false;
}

void action1(Queue* queue, int* pCount) {
    int count = *pCount;
    Data* c = aio4c_malloc(sizeof(Data));

    if (count == 0) {
        aio4c_free(c);
        return;
    }

    c->a = rand() % count;

    if (count - c->a > 0) {
        c->b = rand() % (count - c->a) + c->a;
    } else {
        c->b = c->a;
    }

    dprintf("\tremoving items between %d and %d\n", c->a, c->b);

    removed = 0;

    RemoveAll(queue, aio4c_remove_callback(aio4c_remove), aio4c_remove_discriminant(c));

    dprintf("\tremoved %d items\n", removed);

    count -= removed;

    aio4c_free(c);

    *pCount = count;
}

void action2(Queue* queue, int* pCount) {
    int i = 0;
    int count = *pCount;
    int a = rand() % COUNT;
    Data* c = NULL;

    dprintf("\tadding %d items\n", a);

    for (i = 0; i < a; i++) {
        c = aio4c_malloc(sizeof(Data));
        c->a = count + i;
        c->b = a;
        EnqueueDataItem(queue, c);
        dprintf("\t\tadded %d\n", c->a);
    }

    count += a;

    *pCount = count;
}

void action3(Queue* queue, int* pCount) {
    int i = 0;
    int count = *pCount;
    if (count == 0) {
        return;
    }
    int a = rand() % count;
    QueueItem item;

    dprintf("\tremoving %d items\n", a);

    for (i = 0; i < a; i++) {
        if (Dequeue(queue, &item, false)) {
            dprintf("\t\tremoved %d\n", ((Data*)item.content.data)->a);
            aio4c_free(item.content.data);
            count--;
        }
    }

    *pCount = count;
}

void action4(Queue* queue, int* pCount) {
    int count = *pCount;
    QueueItem item;

    dprintf("\tremoving all %d items\n", count);

    while (Dequeue(queue, &item, false)) {
        dprintf("\t\tremoved %d\n", ((Data*)item.content.data)->a);
        aio4c_free(item.content.data);
        count--;
    }

    *pCount = count;
}

int main(int argc, char* argv[]) {
    QueueItem item;
    int count = 0, j = 0, action = 0;
    void (*actions[4])(Queue*,int*) = {
        action1,
        action2,
        action3,
        action4
    };
    double percent = 0.0;
    int i = 0;

    Aio4cInit(argc, argv);

    if (argc > 2) {
        fprintf(stderr, "usage: %s [loop count]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc == 2) {
        COUNT = atoi(argv[1]);
        if (COUNT <= 0) {
            fprintf(stderr, "error: loop count must be positive\n");
            fprintf(stderr, "usage: %s [loop count]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    srand(getpid());

    Queue* queue = NewQueue();

    for (j = 1; j <= COUNT; j++) {
        action = rand() % 4;
        dprintf("before action %d: %d\n", action + 1, count);
        actions[action](queue,&count);
        dprintf("after action %d: %d\n", action + 1, count);

        percent = (double)(j * 100.0) / ((double)(COUNT));

        printf("\r[");
        for (i = 1; i <= 100; i++) {
            if (percent >= i) {
                printf("=");
            } else if (percent >= i - 1 && percent <= i) {
                printf(">");
            } else {
                printf(".");
            }
        }
        printf("] %06.2lf %%", percent);
        fflush(stdout);
    }

    while (Dequeue(queue, &item, false)) {
        dprintf("\tremoved %d\n", ((Data*)item.content.data)->a);
        aio4c_free(item.content.data);
        count--;
    }

    if (count == 0) {
        printf(" OK\n");
    } else {
        printf(" KO %d\n", count);
    }

    FreeQueue(&queue);

    Aio4cEnd();

    return count;
}
