### 第 30 章 条件变量 - 总结

在并发编程中，锁的作用是为了确保临界区的代码在多线程环境下能够正确执行。然而，仅仅依赖锁并不能解决所有的并发问题。在线程之间的协作中，常常需要一种机制，让线程能够在某个条件满足之前进入休眠状态，而不是无休止地自旋等待。这种机制就是条件变量。

**条件变量的基本概念**

条件变量允许一个线程等待特定的条件成立（如某个共享变量的值发生变化），并且能够高效地将线程置于等待状态。通过使用条件变量，线程可以避免使用 CPU 时间来反复检查条件是否满足，显著提升程序的效率。

**示例代码说明**

在这一章的开头，举了一个简单的例子来说明问题。父线程创建了一个子线程，并希望等待子线程执行完毕再继续运行。代码如下：

```
c复制代码void *child(void *arg) { 
    printf("child\n"); 
    // XXX how to indicate we are done? 
    return NULL; 
}

int main(int argc, char *argv[]) { 
    printf("parent: begin\n"); 
    pthread_t c; 
    Pthread_create(&c, NULL, child, NULL); // create child 
    // XXX how to wait for child? 
    printf("parent: end\n"); 
    return 0; 
}
```

理想情况下，输出应该是：

```
vbnet复制代码parent: begin 
child 
parent: end 
```

然而，如果我们尝试使用简单的共享变量来实现同步，如下所示：

```
c复制代码volatile int done = 0; 

void *child(void *arg) { 
    printf("child\n"); 
    done = 1; 
    return NULL; 
}

int main(int argc, char *argv[]) { 
    printf("parent: begin\n"); 
    pthread_t c; 
    Pthread_create(&c, NULL, child, NULL); // create child 
    while (done == 0) 
        ; // spin 
    printf("parent: end\n"); 
    return 0; 
}
```

这个方案虽然能够工作，但它的效率非常低，因为父线程会不断自旋等待子线程的完成，这种方式浪费了大量的 CPU 时间。理想的解决方案应该是让父线程在条件未满足时进入休眠状态，直到子线程完成工作后再唤醒父线程继续执行。

**关键问题**

这里提出了一个关键问题：如何让线程高效地等待一个条件的满足？简单的自旋方案不仅低效，在某些情况下甚至可能导致更严重的性能问题。因此，我们需要更优雅的解决方案，即条件变量。条件变量通过 `pthread_cond_wait()` 和 `pthread_cond_signal()` 等机制，实现了线程在等待条件满足时的高效同步，避免了不必要的 CPU 开销。

在接下来的内容中，将会详细介绍条件变量的使用方法及其在各种并发场景中的应用，以帮助开发者更好地理解并应用这种重要的同步原语。

### 第 30 章 条件变量 - 第 30.1 节 定义和程序 总结

条件变量（condition variable）是一种用于线程同步的机制，允许线程在特定条件不满足时进入等待状态，直到该条件变为真。条件变量通常与互斥锁（mutex）配合使用，以确保线程在等待和发信号的过程中不会发生竞态条件。

#### 条件变量的定义和基本操作

在 POSIX 线程库中，条件变量通过 `pthread_cond_t` 类型声明，并通过 `pthread_cond_wait()` 和 `pthread_cond_signal()` 两个主要函数进行操作。

- `pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m);`
  - 等待条件变量 `c` 满足。在调用此函数时，线程首先需要持有与之关联的互斥锁 `m`，并且在进入等待状态前会自动释放该锁。当线程被唤醒时，它将重新获取锁并继续执行。这种设计是为了避免在线程进入等待状态时可能产生的竞态条件。
- `pthread_cond_signal(pthread_cond_t *c);`
  - 唤醒一个正在等待条件变量 `c` 的线程。如果没有线程在等待，该信号将被忽略。因此，信号的发出需要在适当的条件下进行控制。

#### 通过条件变量实现线程同步

条件变量的一个典型应用场景是线程之间的同步，例如父线程等待子线程完成（也称为 `join` 操作）。在实现中，通过条件变量的 `wait()` 和 `signal()` 操作，父线程可以高效地等待子线程的完成，而不需要自旋等待，节省了 CPU 资源。

##### 示例代码分析

以下是一个简单的例子，展示了如何使用条件变量让父线程等待子线程完成：

```
c复制代码int done = 0; 
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t c = PTHREAD_COND_INITIALIZER; 

void thr_exit() { 
    Pthread_mutex_lock(&m); 
    done = 1; 
    Pthread_cond_signal(&c); 
    Pthread_mutex_unlock(&m); 
} 

void *child(void *arg) { 
    printf("child\n"); 
    thr_exit(); 
    return NULL; 
}

void thr_join() { 
    Pthread_mutex_lock(&m); 
    while (done == 0) 
        Pthread_cond_wait(&c, &m); 
    Pthread_mutex_unlock(&m); 
} 

int main(int argc, char *argv[]) { 
    printf("parent: begin\n"); 
    pthread_t p; 
    Pthread_create(&p, NULL, child, NULL); 
    thr_join(); 
    printf("parent: end\n"); 
    return 0; 
}
```

**工作原理：**

1. 父线程创建子线程并调用 `thr_join()` 进入等待状态。
2. 子线程执行完毕后调用 `thr_exit()`，设置 `done` 为 1，并通过 `pthread_cond_signal()` 发出信号唤醒父线程。
3. 父线程在收到信号后，从 `pthread_cond_wait()` 返回，重新获得互斥锁，并检查 `done` 的状态，确定子线程已完成，进而继续执行。

#### 注意事项

1. **变量状态的重要性**：在该例子中，变量 `done` 记录了子线程是否完成。如果没有这个变量，可能导致父线程在信号发出前进入等待，从而导致死锁。
2. **锁与信号的使用**：在发出信号和进入等待时，必须持有互斥锁。这不仅是条件变量工作的前提，也是为了防止竞态条件的发生。
3. **使用循环检查条件**：即使在逻辑上看似不必要，使用 `while` 而不是 `if` 来检查条件是一个良好的习惯，因为 `pthread_cond_wait()` 可能会意外返回，循环检查确保了条件的再次验证。

通过这个示例，我们看到了条件变量在线程同步中的基本使用方法和注意事项。在接下来的内容中，将进一步探讨条件变量在更复杂的并发场景中的应用。

### 第 30.2 节 生产者/消费者（有界缓冲区）问题总结

生产者/消费者问题，也被称为有界缓冲区问题，是经典的多线程同步问题。它涉及多个生产者线程将数据放入缓冲区，和多个消费者线程从缓冲区中取出数据。此问题的关键在于如何在生产者和消费者之间进行正确的同步，确保不会出现缓冲区溢出或空取数据的错误。

#### 基本缓冲区实现

我们首先定义了一个简单的缓冲区及其相关的 `put()` 和 `get()` 函数。这些函数的作用分别是将数据存入缓冲区和从缓冲区中取出数据。最初的实现假设缓冲区仅能存储一个整数，并且通过一个 `count` 变量来指示缓冲区是空还是满。

```
c复制代码int buffer;
int count = 0; // initially, empty

void put(int value) {
    assert(count == 0);
    count = 1;
    buffer = value;
}

int get() {
    assert(count == 1);
    count = 0;
    return buffer;
}
```

#### 初始的生产者和消费者线程

简单的生产者和消费者线程分别负责将整数放入缓冲区和从缓冲区取出数据。在最初的实现中，生产者和消费者没有进行同步，这样的设计在并发环境下是有问题的。

```
c复制代码void *producer(void *arg) {
    int i;
    int loops = (int) arg;
    for (i = 0; i < loops; i++) {
        put(i);
    }
}

void *consumer(void *arg) {
    int i;
    while (1) {
        int tmp = get();
        printf("%d\n", tmp);
    }
}
```

#### 问题与初步解决方案

直接使用上述代码会出现竞态条件，尤其是在多个生产者或消费者线程同时操作时。因此，引入了条件变量和互斥锁来控制对缓冲区的访问，确保同步。

```
c复制代码cond_t cond;
mutex_t mutex;

void *producer(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
        Pthread_mutex_lock(&mutex);
        if (count == 1)
            Pthread_cond_wait(&cond, &mutex);
        put(i);
        Pthread_cond_signal(&cond);
        Pthread_mutex_unlock(&mutex);
    }
}

void *consumer(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
        Pthread_mutex_lock(&mutex);
        if (count == 0)
            Pthread_cond_wait(&cond, &mutex);
        int tmp = get();
        Pthread_cond_signal(&cond);
        Pthread_mutex_unlock(&mutex);
        printf("%d\n", tmp);
    }
}
```

#### 关键问题与改进

上述初步解决方案存在两个主要问题：

1. **if 语句的使用**：如果使用 `if` 语句来检查条件，可能导致在条件变化后多个线程争抢资源，导致资源状态不一致。因此，应该将 `if` 替换为 `while` 循环，以确保条件在被唤醒后重新检查。
2. **单个条件变量的问题**：使用单个条件变量可能导致错误的线程被唤醒，从而导致系统进入死锁状态。为了解决这个问题，使用了两个条件变量，`empty` 和 `fill`，分别用于表示缓冲区的空和满状态。

改进后的代码如下：

```
c复制代码cond_t empty, fill;
mutex_t mutex;

void *producer(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
        Pthread_mutex_lock(&mutex);
        while (count == 1)
            Pthread_cond_wait(&empty, &mutex);
        put(i);
        Pthread_cond_signal(&fill);
        Pthread_mutex_unlock(&mutex);
    }
}

void *consumer(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
        Pthread_mutex_lock(&mutex);
        while (count == 0)
            Pthread_cond_wait(&fill, &mutex);
        int tmp = get();
        Pthread_cond_signal(&empty);
        Pthread_mutex_unlock(&mutex);
        printf("%d\n", tmp);
    }
}
```

#### 最终有效方案

为提高并发和效率，最后的解决方案引入了一个大小为 `MAX` 的缓冲区，使得生产者和消费者可以在一个更大的范围内并发操作。这不仅减少了上下文切换的频率，还增加了系统的整体并发性能。

```
c复制代码int buffer[MAX];
int fill = 0;
int use = 0;
int count = 0;

void put(int value) {
    buffer[fill] = value;
    fill = (fill + 1) % MAX;
    count++;
}

int get() {
    int tmp = buffer[use];
    use = (use + 1) % MAX;
    count--;
    return tmp;
}
```

通过以上改进，我们成功地解决了生产者/消费者问题，达到了更高的并发性和效率。

**提示**：在处理条件变量时，使用 `while` 循环来检查条件是一个良好的编程实践，可以避免假唤醒和其他竞态条件带来的问题。

### 第 30.3 节 覆盖条件总结

在第 30.3 节中，介绍了条件变量的一个实际应用场景：多线程内存分配库。这个场景展示了如何处理多个线程在分配内存时可能遇到的条件竞争问题。

在这段代码中，`bytesLeft` 变量用于跟踪堆中剩余的可用内存字节数。`allocate()` 函数在分配内存时，如果内存不足，会使调用线程进入等待状态，直到有足够的内存可用。相应地，当内存被释放时，`free()` 函数会发信号通知等待的线程。

```
c复制代码int bytesLeft = MAX_HEAP_SIZE;
cond_t c;
mutex_t m;

void *allocate(int size) {
    Pthread_mutex_lock(&m);
    while (bytesLeft < size)
        Pthread_cond_wait(&c, &m);
    void *ptr = ...; // get mem from heap
    bytesLeft -= size;
    Pthread_mutex_unlock(&m);
    return ptr;
}

void free(void *ptr, int size) {
    Pthread_mutex_lock(&m);
    bytesLeft += size;
    Pthread_cond_signal(&c); // whom to signal??
    Pthread_mutex_unlock(&m);
}
```

#### 问题描述

代码中的关键问题在于，当有多个线程在等待内存时，释放内存的线程应该唤醒哪个等待线程。例如，一个线程 `Ta` 申请 100 字节的内存，另一个线程 `Tb` 申请 10 字节的内存，而堆中只有 50 字节可用。此时，如果 `free()` 函数仅唤醒 `Ta`，那么由于内存不足，它将继续等待，而 `Tb` 可以立即分配 10 字节的内存。这种情况下，`Tb` 没有被唤醒是一个问题。

#### 覆盖条件的解决方案

为了解决这个问题，Lampson 和 Redell 提出了使用 `pthread_cond_broadcast()` 来代替 `pthread_cond_signal()`。这种方法会唤醒所有等待线程，而不仅仅是一个线程。这样做确保了每个可能需要唤醒的线程都能够被唤醒并重新检查条件。

虽然这种方法可能会引发性能问题，因为可能会唤醒不必要的线程，但在某些情况下，这是确保正确性的最直接方法。这种情况被称为覆盖条件（covering condition），因为它通过广播唤醒所有线程来覆盖所有可能的情况。

### 第 30.4 节 小结

在第 30.4 节中，总结了条件变量的重要性及其在并发编程中的应用。条件变量是继锁之后的另一个关键同步原语，它允许线程在某些条件不满足时进入等待状态。当条件满足时，线程被唤醒并继续执行，这为解决复杂的同步问题提供了有效的工具。

特别是在解决经典的生产者/消费者问题和覆盖条件的情境中，条件变量展示了其强大的功能。通过使用条件变量，程序可以避免忙等待，从而提高效率和资源利用率。此外，正确理解和使用条件变量中的 `signal` 和 `broadcast` 语义也是确保程序正确性的重要部分。

在实际开发中，如果遇到必须使用广播信号才能解决的问题，开发者应当警惕潜在的设计缺陷，除非这是最直接且有效的解决方案。在某些场景下，广播信号确实是唯一的、最简单的方案。