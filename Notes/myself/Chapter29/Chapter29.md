## 第29章 基于锁的并发数据结构

​		在结束锁的讨论之前，我们先讨论如何在常见数据结构中使用锁。通过锁可以使数据结构线程安全（thread safe）。当然，具体如何加锁决定了该数据结构的正确性和效率？因此，我们的挑战是：

#### 关键问题：如何给数据结构加锁？

​		**对于特定数据结构，如何加锁才能让该结构功能正确？进一步，如何对该数据结构加锁，能够保证高性能，让许多线程同时访问该结构，即并发访问（concurrently）？**



​		当然，我们很难介绍所有的数据结构，或实现并发的所有方法，因为这是一个研究多年的议题，已经发表了数以千计的相关论文。因此，我们希望能够提供这类思考方式的足够介绍，同时提供一些好的资料，供你自己进一步研究。我们发现，Moir 和 Shavit 的调查[MS04]就是很好的资料。

### 29.1 并发计数器

计数器是最简单的一种数据结构，使用广泛而且接口简单。图 29.1 中定义了一个非并发的计数器。

```
typedef struct counter_t { 
    int value; 
} counter_t; 

void init(counter_t *c) { 
    c->value = 0; 
} 

void increment(counter_t *c) { 
    c->value++; 
} 

void decrement(counter_t *c) { 
    c->value--; 
} 

int get(counter_t *c) { 
    return c->value; 
} 
```

*图 29.1 无锁的计数器*

这个非并发的计数器虽然简单，但在多线程环境中，不能保证线程安全。当多个线程同时访问或更新计数器时，可能会导致数据竞争问题。

### 简单但无法扩展的加锁计数器

为了解决并发问题，我们可以通过添加锁来实现线程安全的计数器，如图 29.2 所示。

```
typedef struct counter_t { 
    int value; 
    pthread_mutex_t lock; 
} counter_t; 

void init(counter_t *c) { 
    c->value = 0; 
    pthread_mutex_init(&c->lock, NULL); 
} 

void increment(counter_t *c) { 
    pthread_mutex_lock(&c->lock); 
    c->value++; 
    pthread_mutex_unlock(&c->lock); 
} 

void decrement(counter_t *c) { 
    pthread_mutex_lock(&c->lock); 
    c->value--; 
    pthread_mutex_unlock(&c->lock); 
} 

int get(counter_t *c) { 
    pthread_mutex_lock(&c->lock); 
    int rc = c->value; 
    pthread_mutex_unlock(&c->lock); 
    return rc; 
} 
```

*图 29.2 有锁的计数器*

这个有锁的计数器可以正确地在多线程环境中工作，但它存在扩展性问题。当多个线程同时访问计数器时，锁的争用会导致性能显著下降。

### 可扩展的懒惰计数器

为了提高计数器的扩展性，可以使用一种称为懒惰计数器的技术。这种技术使用多个局部计数器和一个全局计数器，其中每个 CPU 核心有一个局部计数器。局部计数器减少了线程之间的锁争用，从而提高了并发性能。每当局部计数器达到一定阈值时，它们的值会被合并到全局计数器中。

懒惰计数器的实现如下：

```
typedef struct counter_t { 
    int global; // global count 
    pthread_mutex_t glock; // global lock 
    int local[NUMCPUS]; // local count (per CPU) 
    pthread_mutex_t llock[NUMCPUS]; // ... and locks 
    int threshold; // update frequency 
} counter_t; 

void init(counter_t *c, int threshold) { 
    c->threshold = threshold; 
    c->global = 0; 
    pthread_mutex_init(&c->glock, NULL); 
    for (int i = 0; i < NUMCPUS; i++) { 
        c->local[i] = 0; 
        pthread_mutex_init(&c->llock[i], NULL); 
    } 
} 

void update(counter_t *c, int threadID, int amt) { 
    pthread_mutex_lock(&c->llock[threadID]); 
    c->local[threadID] += amt; // assumes amt > 0 
    if (c->local[threadID] >= c->threshold) { // transfer to global 
        pthread_mutex_lock(&c->glock); 
        c->global += c->local[threadID]; 
        pthread_mutex_unlock(&c->glock); 
        c->local[threadID] = 0; 
    } 
    pthread_mutex_unlock(&c->llock[threadID]); 
} 

int get(counter_t *c) { 
    pthread_mutex_lock(&c->glock); 
    int val = c->global; 
    pthread_mutex_unlock(&c->glock); 
    return val; // only approximate! 
} 
```

*图 29.5 懒惰计数器的实现*

懒惰计数器通过使用局部计数器和全局计数器的组合，使得计数操作在多个线程中更具扩展性。阈值 `S` 决定了局部计数器与全局计数器的更新频率：`S` 值越大，扩展性越好，但全局计数器与实际值的偏差越大。



### 小结

我们通过简单的锁来实现了线程安全的计数器，并进一步探讨了如何优化以提高其并发性能。懒惰计数器通过局部计数器的引入，显著减少了锁的争用，从而在多线程环境中实现了更好的性能扩展性。这种方法在实际应用中非常重要，尤其是在高并发系统中。



### 29.2 并发链表

在这一节中，我们将讨论如何通过锁机制使链表数据结构变得线程安全，并探讨如何进一步优化该并发链表。

#### 基础实现

我们首先从一个基础实现开始，只关注链表的插入操作。以下是基本的链表结构和插入、查找操作的实现：

```
/ basic node structure 
typedef struct node_t { 
    int key; 
    struct node_t *next; 
} node_t; 

// basic list structure (one used per list) 
typedef struct list_t { 
    node_t *head; 
    pthread_mutex_t lock; 
} list_t; 

void List_Init(list_t *L) { 
    L->head = NULL; 
    pthread_mutex_init(&L->lock, NULL); 
} 

int List_Insert(list_t *L, int key) { 
    pthread_mutex_lock(&L->lock); 
    node_t *new = malloc(sizeof(node_t)); 
    if (new == NULL) { 
        perror("malloc"); 
        pthread_mutex_unlock(&L->lock); 
        return -1; // fail 
    } 
    new->key = key; 
    new->next = L->head; 
    L->head = new; 
    pthread_mutex_unlock(&L->lock); 
    return 0; // success 
} 

int List_Lookup(list_t *L, int key) { 
    pthread_mutex_lock(&L->lock); 
    node_t *curr = L->head; 
    while (curr) { 
        if (curr->key == key) { 
            pthread_mutex_unlock(&L->lock); 
            return 0; // success 
        } 
        curr = curr->next; 
    } 
    pthread_mutex_unlock(&L->lock); 
    return -1; // failure 
} 
```

*图 29.6 并发链表*

在这个实现中，`List_Insert` 和 `List_Lookup` 函数入口处获取锁，结束时释放锁，以确保链表操作的线程安全性。然而，这种方式存在一些小问题，特别是在 `malloc` 失败时需要在插入失败之前释放锁，容易引入错误。

#### 优化并发链表

为了改进上述实现，我们可以调整代码结构，使得获取和释放锁的操作仅围绕临界区进行。具体来说，我们可以将锁定范围缩小到更新链表的操作，从而减少锁的持有时间并降低复杂度。以下是优化后的代码：

```
void List_Init(list_t *L) { 
    L->head = NULL; 
    pthread_mutex_init(&L->lock, NULL); 
} 

void List_Insert(list_t *L, int key) { 
    // synchronization not needed 
    node_t *new = malloc(sizeof(node_t)); 
    if (new == NULL) { 
        perror("malloc"); 
        return; 
    } 
    new->key = key; 

    // just lock critical section 
    pthread_mutex_lock(&L->lock); 
    new->next = L->head; 
    L->head = new; 
    pthread_mutex_unlock(&L->lock); 
} 

int List_Lookup(list_t *L, int key) { 
    int rv = -1; 
    pthread_mutex_lock(&L->lock); 
    node_t *curr = L->head; 
    while (curr) { 
        if (curr->key == key) { 
            rv = 0; 
            break; 
        } 
        curr = curr->next; 
    } 
    pthread_mutex_unlock(&L->lock); 
    return rv; // now both success and failure 
} 
```

*图 29.7 重写并发链表*

这种方式使得 `List_Insert` 函数中的 `malloc` 不再需要锁的保护，只在真正操作链表时获取锁。这样做减少了锁的持有时间，减少了潜在的竞争条件。

#### 扩展链表的并发性

尽管改进了并发链表的实现，但仍然存在扩展性问题。当有多个线程同时访问链表时，性能可能受到锁争用的影响。为了解决这个问题，可以采用 **过手锁**（hand-over-hand locking） 技术。

过手锁的基本思想是为链表中的每个节点都设置一个锁。在遍历链表时，线程首先获取下一个节点的锁，然后释放当前节点的锁。这种方法可以增加链表操作的并发性，但也会增加锁的获取和释放的开销，实际效果并不总是理想的。

### 小结

通过锁机制，我们可以使链表数据结构在多线程环境中保持线程安全。然而，简单的锁机制可能会影响性能，尤其是在高并发场景下。优化锁的使用，减少锁的持有时间，并考虑更复杂的锁策略（如过手锁），可以进一步提高并发数据结构的性能。在实际应用中，应根据具体需求和系统环境选择合适的并发控制策略。

### 29.3 并发队列

在这一节中，我们探讨了如何实现一个高效的并发队列。虽然我们可以通过加锁来实现线程安全的队列，但我们将跳过这种简单方法，转而关注更复杂的并发队列实现。

Michael 和 Scott [MS98] 设计的并发队列通过引入双锁机制，实现了高效的并发操作。以下是他们设计的队列的数据结构和实现代码：

```
typedef struct node_t { 
    int value; 
    struct node_t *next; 
} node_t; 

typedef struct queue_t { 
    node_t *head; 
    node_t *tail; 
    pthread_mutex_t headLock; 
    pthread_mutex_t tailLock; 
} queue_t; 

void Queue_Init(queue_t *q) { 
    node_t *tmp = malloc(sizeof(node_t)); 
    tmp->next = NULL; 
    q->head = q->tail = tmp; 
    pthread_mutex_init(&q->headLock, NULL); 
    pthread_mutex_init(&q->tailLock, NULL); 
} 

void Queue_Enqueue(queue_t *q, int value) { 
    node_t *tmp = malloc(sizeof(node_t)); 
    assert(tmp != NULL); 
    tmp->value = value; 
    tmp->next = NULL; 

    pthread_mutex_lock(&q->tailLock); 
    q->tail->next = tmp; 
    q->tail = tmp; 
    pthread_mutex_unlock(&q->tailLock); 
} 

int Queue_Dequeue(queue_t *q, int *value) { 
    pthread_mutex_lock(&q->headLock); 
    node_t *tmp = q->head; 
    node_t *newHead = tmp->next; 
    if (newHead == NULL) { 
        pthread_mutex_unlock(&q->headLock); 
        return -1; // queue was empty 
    } 
    *value = newHead->value; 
    q->head = newHead; 
    pthread_mutex_unlock(&q->headLock); 
    free(tmp); 
    return 0; 
} 
```

*图 29.8 Michael 和 Scott 的并发队列*

在这段代码中，两个锁分别负责队列的头和尾部操作。通过这种方式，入队和出队操作可以并发执行，因为入队只涉及 `tailLock`，而出队只涉及 `headLock`。

Michael 和 Scott 使用了一个技巧，即在初始化队列时创建一个假节点，将队列的头和尾分开，这样可以使得队列操作更加高效。这个并发队列的设计在多线程程序中广泛应用。

### 29.4 并发散列表

并发散列表是另一个常用的数据结构，它在多线程环境下可以提高查找和插入操作的效率。以下是一个简单的并发散列表实现：

```
#define BUCKETS (101) 

typedef struct hash_t { 
    list_t lists[BUCKETS]; 
} hash_t; 

void Hash_Init(hash_t *H) { 
    int i; 
    for (i = 0; i < BUCKETS; i++) { 
        List_Init(&H->lists[i]); 
    } 
} 

int Hash_Insert(hash_t *H, int key) { 
    int bucket = key % BUCKETS; 
    return List_Insert(&H->lists[bucket], key); 
} 

int Hash_Lookup(hash_t *H, int key) { 
    int bucket = key % BUCKETS; 
    return List_Lookup(&H->lists[bucket], key); 
} 
```

*图 29.9 并发散列表*

这个散列表的实现使用了之前实现的并发链表，每个散列桶（bucket）都有一个链表和一个对应的锁。这种设计允许多个线程同时操作不同的桶，从而提高并发性能。

在多个线程同时进行插入操作时，这个简单的并发散列表能够表现出极好的扩展性。图 29.10 展示了该并发散列表在不同线程数下的性能表现，与使用单锁的链表相比，并发散列表的扩展性显著提高。

![image-20240829121908564](image/image-20240829121908564.png)

### 小结

在这一章中，我们讨论了如何使用锁来实现并发队列和并发散列表。通过合理地使用锁，可以有效地提高数据结构的并发性能，减少锁争用带来的性能瓶颈。与此同时，我们也强调了在设计并发数据结构时，避免不成熟的优化，以确保代码的正确性和效率。



#### 建议：避免不成熟的优化（**Knuth** 定律）

​		实现并发数据结构时，先从最简单的方案开始，也就是加一把大锁来同步。这样做，你很可能构建了正确的锁。如果发现性能问题，那么就改进方法，只要优化到满足需要即可。正如 Knuth 的著名说法“不成熟的优化是所有坏事的根源。” 

​		许多操作系统，在最初过渡到多处理器时都是用一把大锁，包括 Sun 和 Linux。在 Linux 中，这个锁甚至有个名字，叫作 BKL（大内核锁，big kernel lock）。这个方案在很多年里都很有效，直到多 CPU系统普及，内核只允许一个线程活动成为性能瓶颈。终于到了为这些系统优化并发性能的时候了。Linux采用了简单的方案，把一个锁换成多个。Sun 则更为激进，实现了一个最开始就能并发的新系统，Solaris。读者可以通过 Linux 和 Solaris 的内核资料了解更多信息[BC05，MM00]。





### 29.5 小节

在第 29 章的最后一节，我们总结了讨论的并发数据结构，从最简单的计数器到更复杂的链表、队列以及散列表。这些数据结构在多线程环境下的实现展示了如何通过锁来保证线程安全，同时在某些情况下，还能提高并发性能。

在小结中，我们强调了以下几个关键点：

1. **控制流变化时注意锁的管理**：在设计并发数据结构时，需要特别注意控制流的变化，确保在函数返回或发生错误时正确地释放锁。否则，可能会引发难以调试的并发问题。
2. **并发性与性能**：增加并发性并不总能提高性能。在某些情况下，过多的锁反而会导致性能下降。因此，在实现并发数据结构时，必须权衡并发性和性能之间的关系。
3. **避免不成熟的优化**：我们特别提醒开发者，优化应当基于实际的性能问题，而不是盲目地追求更高的并发性。过早的优化往往会带来复杂性，导致代码难以维护，并且未必能带来实际的性能提升。

通过这一章的学习，你应该掌握了如何为常见的数据结构添加并发控制，并且理解了在多线程环境下实现高效数据结构所面临的挑战。如果你对这些话题感兴趣，可以进一步研究更高级的并发数据结构，如 B 树，或深入了解非阻塞数据结构的技术，这在常见并发问题的章节中会稍作提及。对于那些想要深入理解和应用这些概念的读者，数据库课程和相关的文献会是很好的补充资源。

总的来说，第 29 章为你提供了关于并发数据结构的基础知识和设计思路，帮助你在多线程编程中更好地处理并发性和性能之间的关系。