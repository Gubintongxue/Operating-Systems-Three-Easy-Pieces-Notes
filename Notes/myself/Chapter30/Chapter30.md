## 第 30 章 条件变量 - 总结

在并发编程中，锁的作用是为了确保临界区的代码在多线程环境下能够正确执行。然而，仅仅依赖锁并不能解决所有的并发问题。在线程之间的协作中，常常需要一种机制，让线程能够在某个条件满足之前进入休眠状态，而不是无休止地自旋等待。这种机制就是条件变量。

**条件变量的基本概念**

条件变量允许一个线程等待特定的条件成立（如某个共享变量的值发生变化），并且能够高效地将线程置于等待状态。通过使用条件变量，线程可以避免使用 CPU 时间来反复检查条件是否满足，显著提升程序的效率。

**示例代码说明**

在这一章的开头，举了一个简单的例子来说明问题。父线程创建了一个子线程，并希望等待子线程执行完毕再继续运行。代码如下：

```c
void *child(void *arg) { 
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

```c
parent: begin 
child 
parent: end 
```

然而，如果我们尝试使用简单的共享变量来实现同步，如下所示：

```c
volatile int done = 0; 

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

#### **关键问题**

**这里提出了一个关键问题：如何让线程高效地等待一个条件的满足？简单的自旋方案不仅低效，在某些情况下甚至可能导致更严重的性能问题。因此，我们需要更优雅的解决方案，即条件变量。条件变量通过 `pthread_cond_wait()` 和 `pthread_cond_signal()` 等机制，实现了线程在等待条件满足时的高效同步，避免了不必要的 CPU 开销。**



在接下来的内容中，将会详细介绍条件变量的使用方法及其在各种并发场景中的应用，以帮助开发者更好地理解并应用这种重要的同步原语。

### 第 30.1 节 定义和程序

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

```C
int done = 0; 
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



#### 原文：

​		线程可以使用条件变量（condition variable），来等待一个条件变成真。条件变量是一个显式队列，当某些执行状态（即条件，condition）不满足时，线程可以把自己加入队列，等待（waiting）该条件。另外某个线程，当它改变了上述状态时，就可以唤醒一个或者多个等待线程（通过在该条件上发信号），让它们继续执行。Dijkstra 最早在“私有信号量”[D01]中提出这种思想。Hoare 后来在关于观察者的工作中，将类似的思想称为条件变量[H74]。

​		要声明这样的条件变量，只要像这样写：pthread_cond_t c;，这里声明 c 是一个条件变量（注意：还需要适当的初始化）。条件变量有两种相关操作：wait()和 signal()。线程要睡眠的时候，调用 wait()。当线程想唤醒等待在某个条件变量上的睡眠线程时，调用 signal()。具体来说，POSIX 调用如图 30.3 所示。

```
pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m); 
pthread_cond_signal(pthread_cond_t *c); 
1 int done = 0; 
2 pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; 
3 pthread_cond_t c = PTHREAD_COND_INITIALIZER; 
4 
5 void thr_exit() { 
6 Pthread_mutex_lock(&m); 
7 done = 1; 
8 Pthread_cond_signal(&c); 
9 Pthread_mutex_unlock(&m); 
10 } 
11 
12 void *child(void *arg) { 
13 printf("child\n"); 
14 thr_exit(); 
15 return NULL; 
16 }
17 
18 void thr_join() { 
19 Pthread_mutex_lock(&m); 
20 while (done == 0) 
21 Pthread_cond_wait(&c, &m); 
22 Pthread_mutex_unlock(&m); 
23 } 
24 
25 int main(int argc, char *argv[]) { 
26 printf("parent: begin\n"); 
27 pthread_t p; 
28 Pthread_create(&p, NULL, child, NULL); 
29 thr_join(); 
30 printf("parent: end\n"); 
31 return 0; 
32 } 
图 30.3 父线程等待子线程：使用条件变量
```

​		我们常简称为 wait()和 signal()。你可能注意到一点，wait()调用有一个参数，它是互斥量。它假定在 wait()调用时，这个互斥量是已上锁状态。wait()的职责是释放锁，并让调用线程休眠（原子地）。当线程被唤醒时（在另外某个线程发信号给它后），它必须重新获取锁，再返回调用者。这样复杂的步骤也是为了避免在线程陷入休眠时，产生一些竞态条件。我们观察一下图 30.3 中 join 问题的解决方法，以加深理解。

​		有两种情况需要考虑。第一种情况是父线程创建出子线程，但自己继续运行（假设只有一个处理器），然后马上调用 thr_join()等待子线程。在这种情况下，它会先获取锁，检查子进程是否完成（还没有完成），然后调用 wait()，让自己休眠。子线程最终得以运行，打印出“child”，并调用 thr_exit()函数唤醒父进程，这段代码会在获得锁后设置状态变量 done，然后向父线程发信号唤醒它。最后，父线程会运行（从 wait()调用返回并持有锁），释放锁，打印出“parent:end”。

​		第二种情况是，子线程在创建后，立刻运行，设置变量 done 为 1，调用 signal 函数唤醒其他线程（这里没有其他线程），然后结束。父线程运行后，调用 thr_join()时，发现 done已经是 1 了，就直接返回。

​		最后一点说明：你可能看到父线程使用了一个 while 循环，而不是 if 语句来判断是否需要等待。虽然从逻辑上来说没有必要使用循环语句，但这样做总是好的（后面我们会加以说明）。

​		为了确保理解 thr_exit()和 thr_join()中每个部分的重要性，我们来看一些其他的实现。首先，你可能会怀疑状态变量 done 是否需要。代码像下面这样如何？正确吗？

```
1 void thr_exit() { 
2 Pthread_mutex_lock(&m); 
3 Pthread_cond_signal(&c); 
4 Pthread_mutex_unlock(&m); 
5 } 
6 
7 void thr_join() { 
8 Pthread_mutex_lock(&m); 
9 Pthread_cond_wait(&c, &m);
10 Pthread_mutex_unlock(&m); 
11 }
```

​		这段代码是有问题的。假设子线程立刻运行，并且调用 thr_exit()。在这种情况下，子线程发送信号，但此时却没有在条件变量上睡眠等待的线程。父线程运行时，就会调用 wait并卡在那里，没有其他线程会唤醒它。通过这个例子，你应该认识到变量 done 的重要性，它记录了线程有兴趣知道的值。睡眠、唤醒和锁都离不开它。下面是另一个糟糕的实现。在这个例子中，我们假设线程在发信号和等待时都不加锁。会发生什么问题？想想看！

```
1 void thr_exit() { 
2 done = 1; 
3 Pthread_cond_signal(&c); 
4 } 
5 
6 void thr_join() { 
7 if (done == 0) 
8 Pthread_cond_wait(&c); 
9 }
```

​		这里的问题是一个微妙的竞态条件。具体来说，如果父进程调用 thr_join()，然后检查完done 的值为 0，然后试图睡眠。但在调用 wait 进入睡眠之前，父进程被中断。子线程修改变量 done 为 1，发出信号，同样没有等待线程。父线程再次运行时，就会长眠不醒，这就惨了。

#### 提示：发信号时总是持有锁

​		**尽管并不是所有情况下都严格需要，但有效且简单的做法，还是在使用条件变量发送信号时持有锁。虽然上面的例子是必须加锁的情况，但也有一些情况可以不加锁，而这可能是你应该避免的。因此，为了简单，请在调用 signal 时持有锁（hold the lock when calling signal）。**

​		**这个提示的反面，即调用 wait 时持有锁，不只是建议，而是 wait 的语义强制要求的。因为 wait 调用总是假设你调用它时已经持有锁、调用者睡眠之前会释放锁以及返回前重新持有锁。因此，这个提示的一般化形式是正确的：调用 signal 和 wait 时要持有锁（hold the lock when calling signal or wait），你会保持身心健康的。**



希望通过这个简单的 join 示例，你可以看到使用条件变量的一些基本要求。为了确保你能理解，我们现在来看一个更复杂的例子：生产者/消费者（producer/consumer）或有界缓冲区（bounded-buffer）问题。



### 第 30.2 节 生产者/消费者（有界缓冲区）问题

生产者/消费者问题，也被称为有界缓冲区问题，是经典的多线程同步问题。它涉及多个生产者线程将数据放入缓冲区，和多个消费者线程从缓冲区中取出数据。此问题的关键在于如何在生产者和消费者之间进行正确的同步，确保不会出现缓冲区溢出或空取数据的错误。

#### 基本缓冲区实现

我们首先定义了一个简单的缓冲区及其相关的 `put()` 和 `get()` 函数。这些函数的作用分别是将数据存入缓冲区和从缓冲区中取出数据。最初的实现假设缓冲区仅能存储一个整数，并且通过一个 `count` 变量来指示缓冲区是空还是满。

```
int buffer;
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
void *producer(void *arg) {
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
cond_t cond;
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
cond_t empty, fill;
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
int buffer[MAX];
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



#### 原文：

​		本章要面对的下一个问题，是生产者/消费者（producer/consumer）问题，也叫作有界缓冲区（bounded buffer）问题。这一问题最早由 Dijkstra 提出[D72]。实际上也正是通过研究这一问题，Dijkstra 和他的同事发明了通用的信号量（它可用作锁或条件变量）[D01]。

​		假设有一个或多个生产者线程和一个或多个消费者线程。生产者把生成的数据项放入缓冲区；消费者从缓冲区取走数据项，以某种方式消费。

​		很多实际的系统中都会有这种场景。例如，在多线程的网络服务器中，一个生产者将HTTP 请求放入工作队列（即有界缓冲区），消费线程从队列中取走请求并处理。

​		我们在使用管道连接不同程序的输出和输入时，也会使用有界缓冲区，例如 grep foo file.txt | wc -l。这个例子并发执行了两个进程，grep 进程从 file.txt 中查找包括“foo”的行，写到标准输出；UNIX shell 把输出重定向到管道（通过 pipe 系统调用创建）。管道的另一端是 wc 进程的标准输入，wc 统计完行数后打印出结果。因此，grep 进程是生产者，wc 是进程是消费者，它们之间是内核中的有界缓冲区，而你在这个例子里只是一个开心的用户。

​		因为有界缓冲区是共享资源，所以我们必须通过同步机制来访问它，以免①产生竞态条件。为了更好地理解这个问题，我们来看一些实际的代码。

​		首先需要一个共享缓冲区，让生产者放入数据，消费者取出数据。简单起见，我们就拿一个整数来做缓冲区（你当然可以想到用一个指向数据结构的指针来代替），两个内部函数将值放入缓冲区，从缓冲区取值。图 30.4 为相关代码。

```
1 int buffer; 
2 int count = 0; // initially, empty 
3 
4 void put(int value) { 
5 assert(count == 0); 
6 count = 1; 
7 buffer = value; 
8 } 
9 
10 int get() { 
11 assert(count == 1); 
12 count = 0; 
13 return buffer; 
14 } 
图 30.4 put 和 get 函数（第 1 版）
```

​		很简单，不是吗？put()函数会假设缓冲区是空的，把一个值存在缓冲区，然后把 count设置为 1 表示缓冲区满了。get()函数刚好相反，把缓冲区清空后（即将 count 设置为 0），并返回该值。不用担心这个共享缓冲区只能存储一条数据，稍后我们会一般化，用队列保存更多数据项，这会比听起来更有趣。

​		现在我们需要编写一些函数，知道何时可以访问缓冲区，以便将数据放入缓冲区或从缓冲区取出数据。条件是显而易见的：仅在 count 为 0 时（即缓冲器为空时），才将数据放入缓冲器中。仅在计数为 1 时（即缓冲器已满时），才从缓冲器获得数据。如果我们编写同步代码，让生产者将数据放入已满的缓冲区，或消费者从空的数据获取数据，就做错了（在这段代码中，断言将触发）。

​		这项工作将由两种类型的线程完成，其中一类我们称之为生产者（producer）线程，另一类我们称之为消费者（consumer）线程。图 30.5 展示了一个生产者的代码，它将一个整数放入共享缓冲区 loops 次，以及一个消费者，它从该共享缓冲区中获取数据（永远不停），每次打印出从共享缓冲区中提取的数据项。

```
1 void *producer(void *arg) { 
2 int i; 
3 int loops = (int) arg; 
4 for (i = 0; i < loops; i++) { 
5 put(i); 
6 } 
7 } 
8 
9 void *consumer(void *arg) { 
10 int i; 
11 while (1) { 
12 int tmp = get(); 
13 printf("%d\n", tmp); 
14 } 
15 } 
图 30.5 生产者/消费者线程（第 1 版）
```

有问题的方案 

​		假设只有一个生产者和一个消费者。显然，put()和 get()函数之中会有临界区，因为 put()更新缓冲区，get()读取缓冲区。但是，给代码加锁没有用，我们还需别的东西。不奇怪，别的东西就是某些条件变量。在这个（有问题的）首次尝试中（见图 30.6），我们用了条件变量 cond 和相关的锁 mutex。

```
1 cond_t cond; 
2 mutex_t mutex; 
3 
4 void *producer(void *arg) { 
5 int i; 
6 for (i = 0; i < loops; i++) { 
7 Pthread_mutex_lock(&mutex); // p1 
8 if (count == 1) // p2 
9 Pthread_cond_wait(&cond, &mutex); // p3 
10 put(i); // p4 
11 Pthread_cond_signal(&cond); // p5 
12 Pthread_mutex_unlock(&mutex); // p6 
13 } 
14 } 
15 
16 void *consumer(void *arg) { 
17 int i; 
18 for (i = 0; i < loops; i++) { 
19 Pthread_mutex_lock(&mutex); // c1 
20 if (count == 0) // c2 
21 Pthread_cond_wait(&cond, &mutex); // c3 
22 int tmp = get(); // c4 
23 Pthread_cond_signal(&cond); // c5 
24 Pthread_mutex_unlock(&mutex); // c6
25 printf("%d\n", tmp); 
26 } 
27 }
图 30.6 生产者/消费者：一个条件变量和 if 语句
```

​		来看看生产者和消费者之间的信号逻辑。当生产者想要填充缓冲区时，它等待缓冲区变空（p1～p3）。消费者具有完全相同的逻辑，但等待不同的条件——变满（c1～c3）。

​		当只有一个生产者和一个消费者时，图 30.6 中的代码能够正常运行。但如果有超过一个线程（例如两个消费者），这个方案会有两个严重的问题。哪两个问题？

​		……（暂停思考一下）……

​		我们来理解第一个问题，它与等待之前的 if 语句有关。假设有两个消费者（*T*c1 和 *T*c2），一个生产者（*T*p）。首先，一个消费者（*T*c1）先开始执行，它获得锁（c1），检查缓冲区是否可以消费（c2），然后等待（c3）（这会释放锁）。

​		接着生产者（*T*p）运行。它获取锁（p1），检查缓冲区是否满（p2），发现没满就给缓冲区加入一个数字（p4）。然后生产者发出信号，说缓冲区已满（p5）。关键的是，这让第一个消费者（*T*c1）不再睡在条件变量上，进入就绪队列。*T*c1 现在可以运行（但还未运行）。生产者继续执行，直到发现缓冲区满后睡眠（p6,p1-p3）。

​		这时问题发生了：另一个消费者（*T*c2）抢先执行，消费了缓冲区中的值（c1,c2,c4,c5,c6，跳过了 c3 的等待，因为缓冲区是满的）。现在假设 *T*c1 运行，在从 wait 返回之前，它获取了锁，然后返回。然后它调用了 get() (p4)，但缓冲区已无法消费！断言触发，代码不能像预期那样工作。显然，我们应该设法阻止 *T*c1 去消费，因为 *T*c2 插进来，消费了缓冲区中之前生产的一个值。表 30.1 展示了每个线程的动作，以及它的调度程序状态（就绪、运行、睡眠）随时间的变化。

![image-20240829135148024](image/image-20240829135148024.png)

![image-20240829135157332](image/image-20240829135157332.png)

​		问题产生的原因很简单：在 *T*c1 被生产者唤醒后，但在它运行之前，缓冲区的状态改变了（由于 *T*c2）。发信号给线程只是唤醒它们，暗示状态发生了变化（在这个例子中，就是值已被放入缓冲区），但并不会保证在它运行之前状态一直是期望的情况。信号的这种释义常称为 Mesa 语义（Mesa semantic），为了纪念以这种方式建立条件变量的首次研究[LR80]。另一种释义是 Hoare 语义（Hoare semantic），虽然实现难度大，但是会保证被唤醒线程立刻执行[H74]。实际上，几乎所有系统都采用了 Mesa 语义。

##### 较好但仍有问题的方案：使用 While 语句替代 If 

​		幸运的是，修复这个问题很简单（见图 30.7）：把 if 语句改为 while。当消费者 *T*c1 被唤醒后，立刻再次检查共享变量（c2）。如果缓冲区此时为空，消费者就会回去继续睡眠（c3）。生产者中相应的 if 也改为 while（p2）。

```
1 cond_t cond; 
2 mutex_t mutex; 
3 
4 void *producer(void *arg) { 
5 int i; 
6 for (i = 0; i < loops; i++) { 
7 Pthread_mutex_lock(&mutex); // p1 
8 while (count == 1) // p2 
9 Pthread_cond_wait(&cond, &mutex); // p3 
10 put(i); // p4 
11 Pthread_cond_signal(&cond); // p5 
12 Pthread_mutex_unlock(&mutex); // p6 
13 } 
14 } 
15 
16 void *consumer(void *arg) { 
17 int i; 
18 for (i = 0; i < loops; i++) { 
19 Pthread_mutex_lock(&mutex); // c1 
20 while (count == 0) // c2 
21 Pthread_cond_wait(&cond, &mutex); // c3 
22 int tmp = get(); // c4 
23 Pthread_cond_signal(&cond); // c5 
24 Pthread_mutex_unlock(&mutex); // c6 
25 printf("%d\n", tmp); 
26 } 
27 }
图 30.7 生产者/消费者：一个条件变量和 while 语句
```

​		由于 Mesa 语义，我们要记住一条关于条件变量的简单规则：总是使用 while 循环（always use while loop）。虽然有时候不需要重新检查条件，但这样做总是安全的，做了就开心了。

​		但是，这段代码仍然有一个问题，也是上文提到的两个问题之一。你能想到吗？它和我们只用了一个条件变量有关。尝试弄清楚这个问题是什么，再继续阅读。想一下！

​		……（暂停想一想，或者闭一下眼）……

​		我们来确认一下你想得对不对。假设两个消费者（*T*c1 和 *T*c2）先运行，都睡眠了（c3）。生产者开始运行，在缓冲区放入一个值，唤醒了一个消费者（假定是 *T*c1），并开始睡眠。现在是一个消费者马上要运行（*T*c1），两个线程（*T*c2 和 *T*p）都等待在同一个条件变量上。问题马上就要出现了：让人感到兴奋！

​		消费者 *T*c1 醒过来并从 wait()调用返回（c3），重新检查条件（c2），发现缓冲区是满的，消费了这个值（c4）。这个消费者然后在该条件上发信号（c5），唤醒一个在睡眠的线程。但是，应该唤醒哪个线程呢？

​		因为消费者已经清空了缓冲区，很显然，应该唤醒生产者。但是，如果它唤醒了 *T*c2（这绝对是可能的，取决于等待队列是如何管理的），问题就出现了。具体来说，消费者 *T*c2 会醒过来，发现队列为空（c2），又继续回去睡眠（c3）。生产者 *T*p 刚才在缓冲区中放了一个值，现在在睡眠。另一个消费者线程 *T*c1 也回去睡眠了。3 个线程都在睡眠，显然是一个缺陷。由表 30.2 可以看到这个可怕灾难的步骤。

![image-20240829135940734](image/image-20240829135940734.png)

![image-20240829135950681](image/image-20240829135950681.png)

​		信号显然需要，但必须更有指向性。消费者不应该唤醒消费者，而应该只唤醒生产者，反之亦然。

#### 单值缓冲区的生产者/消费者方案 

​		解决方案也很简单：使用两个条件变量，而不是一个，以便正确地发出信号，在系统状态改变时，哪类线程应该唤醒。图 30.8 展示了最终的代码。

```
1 cond_t empty, fill; 
2 mutex_t mutex; 
3 
4 void *producer(void *arg) { 
5 int i; 
6 for (i = 0; i < loops; i++) { 
7 Pthread_mutex_lock(&mutex); 
8 while (count == 1) 
9 Pthread_cond_wait(&empty, &mutex); 
10 put(i); 
11 Pthread_cond_signal(&fill); 
12 Pthread_mutex_unlock(&mutex); 
13 } 
14 } 
15 
16 void *consumer(void *arg) { 
17 int i; 
18 for (i = 0; i < loops; i++) { 
19 Pthread_mutex_lock(&mutex); 
20 while (count == 0) 
21 Pthread_cond_wait(&fill, &mutex); 
22 int tmp = get(); 
23 Pthread_cond_signal(&empty); 
24 Pthread_mutex_unlock(&mutex); 
25 printf("%d\n", tmp); 
26 } 
27 } 
图 30.8 生产者/消费者：两个条件变量和 while 语句
```

在上述代码中，生产者线程等待条件变量 empty，发信号给变量 fill。相应地，消费者线程等待 fill，发信号给 empty。这样做，从设计上避免了上述第二个问题：消费者再也不会唤醒消费者，生产者也不会唤醒生产者。



最终的生产者/消费者方案 

​		我们现在有了可用的生产者/消费者方案，但不太通用。我们最后的修改是提高并发和效率。具体来说，增加更多缓冲区槽位，这样在睡眠之前，可以生产多个值。同样，睡眠之前可以消费多个值。单个生产者和消费者时，这种方案因为上下文切换少，提高了效率。多个生产者和消费者时，它甚至支持并发生产和消费，从而提高了并发。幸运的是，和现有方案相比，改动也很小。

​		第一处修改是缓冲区结构本身，以及对应的 put()和 get()方法（见图 30.9）。我们还稍稍修改了生产者和消费者的检查条件，以便决定是否要睡眠。图 30.10 展示了最终的等待和信号逻辑。生产者只有在缓冲区满了的时候才会睡眠（p2），消费者也只有在队列为空的时候睡眠（c2）。至此，我们解决了生产者/消费者问题。

```
1 int buffer[MAX]; 
2 int fill = 0; 
3 int use = 0; 
4 int count = 0; 
5 
6 void put(int value) { 
7 buffer[fill] = value; 
8 fill = (fill + 1) % MAX; 
9 count++; 
10 } 
11 
12 int get() { 
13 int tmp = buffer[use]; 
14 use = (use + 1) % MAX; 
15 count--; 
16 return tmp; 
17 } 
图 30.9 最终的 put()和 get()方法

```



```
1 cond_t empty, fill; 
2 mutex_t mutex; 
3 
4 void *producer(void *arg) { 
5 int i; 
6 for (i = 0; i < loops; i++) { 
7 Pthread_mutex_lock(&mutex); // p1 
8 while (count == MAX) // p2 
9 Pthread_cond_wait(&empty, &mutex); // p3 
10 put(i); // p4 
11 Pthread_cond_signal(&fill); // p5 
12 Pthread_mutex_unlock(&mutex); // p6 
13 } 
14 } 
15 
16 void *consumer(void *arg) { 
17 int i; 
18 for (i = 0; i < loops; i++) {
19 Pthread_mutex_lock(&mutex); // c1 
20 while (count == 0) // c2 
21 Pthread_cond_wait(&fill, &mutex); // c3 
22 int tmp = get(); // c4 
23 Pthread_cond_signal(&empty); // c5 
24 Pthread_mutex_unlock(&mutex); // c6 
25 printf("%d\n", tmp); 
26 } 
27 } 
图 30.10 最终有效方案
```

#### 提示：对条件变量使用 **while**（不是 **if**）

​		多线程程序在检查条件变量时，使用 while 循环总是对的。if 语句可能会对，这取决于发信号的语义。因此，总是使用 while，代码就会符合预期。 

​		对条件变量使用 while 循环，这也解决了假唤醒（spurious wakeup）的情况。某些线程库中，由于实现的细节，有可能出现一个信号唤醒两个线程的情况[L11]。再次检查线程的等待条件，假唤醒是另一个原因。





### 第 30.3 节 覆盖条件

在第 30.3 节中，介绍了条件变量的一个实际应用场景：多线程内存分配库。这个场景展示了如何处理多个线程在分配内存时可能遇到的条件竞争问题。

在这段代码中，`bytesLeft` 变量用于跟踪堆中剩余的可用内存字节数。`allocate()` 函数在分配内存时，如果内存不足，会使调用线程进入等待状态，直到有足够的内存可用。相应地，当内存被释放时，`free()` 函数会发信号通知等待的线程。

```
int bytesLeft = MAX_HEAP_SIZE;
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