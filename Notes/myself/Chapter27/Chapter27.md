## 第27章 插叙：线程API

​		本章介绍了主要的线程 API。后续章节也会进一步介绍如何使用 API。更多的细节可以参考其他书籍和在线资源[B89，B97，B+96，K+96]。随后的章节会慢慢介绍锁和条件变量的概念，因此本章可以作为参考。

#### **关键问题：如何创建和控制线程？**

**操作系统应该提供哪些创建和控制线程的接口？这些接口如何设计得易用和实用？**

### 27.1 线程创建

编写多线程程序的第一步就是创建新线程，因此必须存在某种线程创建接口。在 POSIX 中，创建线程的方式非常简单：

```
#include <pthread.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
```

这个函数声明看起来可能有点复杂（特别是如果你不熟悉 C 中的函数指针），但实际使用时并不难。该函数有四个参数：`thread`、`attr`、`start_routine` 和 `arg`。

- **`thread`**: 这是一个指向 `pthread_t` 结构类型的指针。这个结构将用于标识线程，因此需要将它传入 `pthread_create()` 以便初始化。
- **`attr`**: 用于指定线程的属性，例如栈大小或调度优先级等信息。可以通过调用 `pthread_attr_init()` 来初始化属性。在大多数情况下，传入 `NULL` 即可使用默认属性。
- **`start_routine`**: 这是一个函数指针，指向线程将要执行的函数。这个函数必须接受一个 `void *` 类型的参数，并返回一个 `void *` 类型的值。
- **`arg`**: 这是要传递给线程执行函数的参数，可以是任何类型的数据。为了通用性，`arg` 的类型为 `void *`，允许传递任意类型的数据。

#### 示例

来看一个创建线程的简单例子：

```
#include <pthread.h>
#include <stdio.h>

typedef struct myarg_t {
    int a;
    int b;
} myarg_t;

void *mythread(void *arg) {
    myarg_t *m = (myarg_t *) arg;
    printf("Thread arguments: %d %d\n", m->a, m->b);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p;
    int rc;

    myarg_t args;
    args.a = 10;
    args.b = 20;

    rc = pthread_create(&p, NULL, mythread, &args);
    if (rc != 0) {
        printf("Error creating thread\n");
        return 1;
    }

    pthread_join(p, NULL);  // 等待线程完成
    return 0;
}
```

在这个例子中，我们定义了一个结构体 `myarg_t`，其中包含两个整数成员 `a` 和 `b`。然后，我们定义了一个线程函数 `mythread()`，它接受一个 `void *` 参数，并将其转换为 `myarg_t *` 类型，从而可以访问传入的参数。线程函数打印参数的值并返回。

主程序创建一个线程，并传递 `myarg_t` 结构的地址作为参数。线程创建后，主程序调用 `pthread_join()` 等待线程完成。

### 线程的生命周期

线程一旦创建，就会作为一个独立的执行实体存在。它有自己的调用栈，并与同一进程中的其他线程共享相同的地址空间。线程的生命周期包括创建、执行和终止三个阶段。线程可以通过返回函数或调用 `pthread_exit()` 来终止。当线程终止时，系统会回收它使用的资源，除非它处于“分离”状态。分离状态的线程在终止后，其资源会自动被回收，无需显式等待它完成。

希望这些信息能够补全第 27.1 节的内容。如果你有更多问题或需要进一步的解释，请随时告诉我！

### 27.2 线程完成

上面的例子展示了如何创建一个线程。但是，如果你想等待线程完成，会发生什么情况？为了确保线程执行完毕，你需要使用 `pthread_join()` 函数。该函数有两个参数：

- 第一个参数是 `pthread_t` 类型，用于指定要等待的线程。这个变量是在创建线程时由 `pthread_create()` 初始化的。你需要保存它，以便稍后使用它来等待线程终止。
- 第二个参数是一个指针，指向你希望得到的线程返回值。因为线程函数可以返回任何类型的值，因此该指针定义为 `void *` 类型。由于 `pthread_join()` 会修改传入的参数值，因此你需要传入一个指向该值的指针，而不仅仅是该值本身。

来看一个示例代码（见图 27.2）。在这个例子中，我们再次创建了一个线程，并通过 `myarg_t` 结构传递一些参数。线程函数的返回值使用 `myret_t` 类型。当线程完成时，主线程通过 `pthread_join()` 等待线程，并获取线程的返回值，即 `myret_t` 结构中的内容。

```
#include <stdio.h> 
#include <pthread.h> 
#include <assert.h> 
#include <stdlib.h> 

typedef struct myarg_t { 
    int a; 
    int b; 
} myarg_t; 

typedef struct myret_t { 
    int x; 
    int y; 
} myret_t; 

void *mythread(void *arg) { 
    myarg_t *m = (myarg_t *) arg; 
    printf("%d %d\n", m->a, m->b); 
    myret_t *r = (myret_t *) malloc(sizeof(myret_t)); 
    r->x = 1; 
    r->y = 2; 
    return (void *) r; 
} 

int main(int argc, char *argv[]) { 
    int rc; 
    pthread_t p; 
    myret_t *m; 

    myarg_t args; 
    args.a = 10; 
    args.b = 20; 
    rc = pthread_create(&p, NULL, mythread, &args); 
    assert(rc == 0); 
    rc = pthread_join(p, (void **) &m); 
    assert(rc == 0); 
    printf("returned %d %d\n", m->x, m->y); 
    free(m);  // 不要忘记释放分配的内存
    return 0; 
}
```

### 重要说明

1. **内存分配**: 在示例中，返回值是通过 `malloc()` 动态分配的。这意味着线程返回的指针指向的是堆上的内存，而不是栈上的内存。因此，主线程在使用完返回值后需要调用 `free()` 释放内存。
2. **避免返回栈上的指针**: 如果你在线程函数中分配了栈内存并返回它的地址，如下例所示，那么线程终止时栈内存将被释放，返回的指针将指向无效的内存，导致程序出现不确定行为：

```
void *mythread(void *arg) { 
    myarg_t *m = (myarg_t *) arg; 
    printf("%d %d\n", m->a, m->b); 
    myret_t r;  // 分配在栈上：不安全！
    r.x = 1; 
    r.y = 2; 
    return (void *) &r;  // 返回栈指针：危险！
}
```

1. **简单情况下的传参与返回**: 如果线程函数只需传入或返回一个简单的值（如 `int`），那么你可以不必创建结构，直接传递参数，如图 27.3 所示：

```
void *mythread(void *arg) { 
    int m = (int) arg; 
    printf("%d\n", m); 
    return (void *) (m + 1); 
}

int main(int argc, char *argv[]) { 
    pthread_t p; 
    int rc, m; 
    rc = pthread_create(&p, NULL, mythread, (void *) 100); 
    assert(rc == 0); 
    rc = pthread_join(p, (void **) &m); 
    assert(rc == 0); 
    printf("returned %d\n", m); 
    return 0; 
}
```

### 线程的使用场景

在多线程编程中，`pthread_create()` 和 `pthread_join()` 是关键的 API，它们允许你创建并控制线程。通常情况下，`pthread_join()` 用于确保线程在主线程结束前执行完毕。对于长期运行的程序，如多线程 Web 服务器，可能不会使用 `pthread_join()`，而是让线程独立地完成各自的任务。

通过这些例子和注意事项，你可以更好地理解如何创建和管理线程，确保你的多线程程序正确运行。



#### 原文：

​		上面的例子展示了如何创建一个线程。但是，如果你想等待线程完成，会发生什么情况？你需要做一些特别的事情来等待完成。具体来说，你必须调用函数 pthread_join()。

​		该函数有两个参数。第一个是 pthread_t 类型，用于指定要等待的线程。这个变量是由线程创建函数初始化的（当你将一个指针作为参数传递给 pthread_create()时）。如果你保留了它，就可以用它来等待该线程终止。

​		第二个参数是一个指针，指向你希望得到的返回值。因为函数可以返回任何东西，所以它被定义为返回一个指向 void 的指针。因为 pthread_join()函数改变了传入参数的值，所以你需要传入一个指向该值的指针，而不只是该值本身。

​		我们来看另一个例子（见图 27.2）。在代码中，再次创建单个线程，并通过 myarg_t 结构传递一些参数。对于返回值，使用 myret_t 型。当线程完成运行时，主线程已经在pthread_join()函数内等待了①。然后会返回，我们可以访问线程返回的值，即在 myret_t 中的内容。

​		有几点需要说明。首先，我们常常不需要这样痛苦地打包、解包参数。如果我们不需要参数，创建线程时传入 NULL 即可。类似的，如果不需要返回值，那么 pthread_join()调用也可以传入 NULL。

```
1 #include <stdio.h> 
2 #include <pthread.h> 
3 #include <assert.h> 
4 #include <stdlib.h> 
5 
6 typedef struct myarg_t { 
7 int a; 
8 int b; 
9 } myarg_t; 
10 
11 typedef struct myret_t { 
12 int x; 
13 int y; 
14 } myret_t; 
15 
16 void *mythread(void *arg) { 
17 myarg_t *m = (myarg_t *) arg; 
18 printf("%d %d\n", m->a, m->b); 
19 myret_t *r = Malloc(sizeof(myret_t)); 
20 r->x = 1; 
21 r->y = 2; 
22 return (void *) r; 
23 } 
24 
25 int 
26 main(int argc, char *argv[]) { 
27 int rc; 
28 pthread_t p; 
29 myret_t *m;
30 
31 myarg_t args; 
32 args.a = 10; 
33 args.b = 20; 
34 Pthread_create(&p, NULL, mythread, &args); 
35 Pthread_join(p, (void **) &m); 
36 printf("returned %d %d\n", m->x, m->y); 
37 return 0; 
38 }
图 27.2 等待线程完成
```

其次，如果我们只传入一个值（例如，一个 int），也不必将它打包为一个参数。图 27.3 展示了一个例子。在这种情况下，更简单一些，因为我们不必在结构中打包参数和返回值。

```
void *mythread(void *arg) { 
 int m = (int) arg; 
 printf("%d\n", m); 
 return (void *) (arg + 1); 
} 
int main(int argc, char *argv[]) { 
 pthread_t p; 
 int rc, m; 
 Pthread_create(&p, NULL, mythread, (void *) 100); 
 Pthread_join(p, (void **) &m); 
 printf("returned %d\n", m); 
 return 0; 
}
图 27.3 较简单的向线程传递参数示例
```

​		再次，我们应该注注，必须非常小心如何从线程返回值。特别是，永远不要返回一个指针，并让它指向线程调用栈上分配的东西。如果这样做，你认为会发生什么？（想一想！）下面是一段危险的代码示例，对图 27.2 中的示例做了修改。

```
1 void *mythread(void *arg) { 
2 myarg_t *m = (myarg_t *) arg; 
3 printf("%d %d\n", m->a, m->b); 
4 myret_t r; // ALLOCATED ON STACK: BAD! 
5 r.x = 1; 
6 r.y = 2; 
7 return (void *) &r; 
8 }
```

​		在这个例子中，变量 *r* 被分配在 mythread 的栈上。但是，当它返回时，该值会自动释放（这就是栈使用起来很简单的原因！），因此，将指针传回现在已释放的变量将导致各种不好的结果。当然，当你打印出你以为的返回值时，你可能会感到惊讶（但不一定！）。试试看，自己找出真相①！

​		最后，你可能会注注到，使用 pthread_create()创建线程，然后立即调用 pthread_join()，这是创建线程的一种非常奇怪的方式。事实上，有一个更简单的方法来完成这个任务，它被称为过程调用（procedure call）。显然，我们通常会创建不止一个线程并等待它完成，否则根本没有太多的用途。

​		我们应该注注，并非所有多线程代码都使用 join 函数。例如，多线程 Web 服务器可能会创建大量工作线程，然后使用主线程接受请求，并将其无限期地传递给工作线程。因此这样的长期程序可能不需要 join。然而，创建线程来（并行）执行特定任务的并行程序，很可能会使用 join 来确保在退出或进入下一阶段计算之前完成所有这些工作。



### 27.3 锁

在多线程编程中，使用锁来提供互斥访问临界区是非常重要的。这意味着在任何给定的时间，只有一个线程可以进入临界区，从而避免竞争条件。POSIX 线程库提供了两种最基本的锁操作函数：

```
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
```

这些函数的使用非常直接。如果你发现某段代码属于临界区，就需要通过锁来保护它，以确保它按预期运行。代码大致如下：

```
pthread_mutex_t lock;
pthread_mutex_lock(&lock);
// 临界区代码，例如：
x = x + 1;
pthread_mutex_unlock(&lock);
```

**基本原理**：

- `pthread_mutex_lock()` 尝试获取锁。如果没有其他线程持有锁，线程将成功获取锁并进入临界区。
- 如果锁已被另一个线程持有，那么调用 `pthread_mutex_lock()` 的线程将阻塞，直到锁被释放。
- `pthread_mutex_unlock()` 用于释放锁，允许其他阻塞的线程获取该锁。

**常见问题**：

1. **锁的初始化**：

   所有锁在使用前必须正确初始化。POSIX 线程库提供了两种方法来初始化锁：

   - **静态初始化**：使用 `PTHREAD_MUTEX_INITIALIZER` 进行静态初始化。

     ```
     pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
     ```
     
   - **动态初始化**：调用 `pthread_mutex_init()` 动态初始化。

     ```
  int rc = pthread_mutex_init(&lock, NULL);
     assert(rc == 0);  // 确保初始化成功
     ```
   
   初始化时可以指定锁的属性，通常传入 `NULL` 以使用默认属性。

2. **错误检查**：

   在获取锁和释放锁时，检查返回值是非常重要的。如果不检查错误代码，可能会导致多个线程意外进入临界区。你可以使用如下的封装函数来确保代码整洁并处理错误：

   ```
   void Pthread_mutex_lock(pthread_mutex_t *mutex) {
       int rc = pthread_mutex_lock(mutex);
       assert(rc == 0);
   }
   
   void Pthread_mutex_unlock(pthread_mutex_t *mutex) {
       int rc = pthread_mutex_unlock(mutex);
       assert(rc == 0);
   }
   ```

3. **其他锁操作**：

   POSIX 线程库还提供了以下两个函数，用于更灵活地获取锁：

   - `pthread_mutex_trylock(pthread_mutex_t *mutex)`: 尝试获取锁，如果锁已被占用，则立即返回失败。
   - `pthread_mutex_timedlock(pthread_mutex_t *mutex, struct timespec *abs_timeout)`: 尝试在指定的超时时间内获取锁。如果在超时前获取成功，则返回成功；否则返回超时失败。

   这些函数在某些特殊情况下非常有用，例如在处理死锁时。通过 `pthread_mutex_trylock()` 和 `pthread_mutex_timedlock()`，程序可以避免在尝试获取锁时无限期地阻塞。

**总结**：

通过正确地初始化、使用和管理锁，你可以确保多线程程序中的临界区得到安全的访问，避免竞态条件。锁是多线程编程的基础之一，理解并熟练使用这些工具，对于编写健壮的并发程序至关重要。

### 27.4 条件变量

条件变量（condition variable）是线程库中的一个关键组件，用于在线程之间传递信号，特别是在一个线程需要等待另一个线程完成某些操作时。POSIX 线程库为条件变量提供了两个主要函数：

```
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_signal(pthread_cond_t *cond);
```

**使用条件变量的步骤**：

1. **条件变量的初始化**：

   条件变量通常与锁配合使用。在初始化相关的锁和条件变量之后，程序可以使用它们来控制线程的同步。如下是典型的初始化代码：

   ```
   pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
   pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
   ```

   或者使用动态初始化：

   ```
   pthread_mutex_init(&lock, NULL);
   pthread_cond_init(&cond, NULL);
   ```

2. **等待条件满足**：

   `pthread_cond_wait()` 函数让调用线程进入休眠状态，等待其他线程发出信号。通常，当某些程序状态发生变化时，需要唤醒该线程。典型的用法如下：

   ```
   Pthread_mutex_lock(&lock);
   while (ready == 0)
       Pthread_cond_wait(&cond, &lock);
   Pthread_mutex_unlock(&lock);
   ```

   - 线程会在 `while` 循环中检查条件 `ready`，如果条件不满足，调用 `pthread_cond_wait()` 进入休眠状态。
   - `pthread_cond_wait()` 函数不仅让线程进入休眠，还会在休眠时释放锁，以便其他线程可以获得锁并改变条件。
   - 当条件变量收到信号并唤醒线程后，`pthread_cond_wait()` 会重新获取锁，并继续执行等待线程后面的代码。

3. **发出信号**：

   另一个线程可以通过 `pthread_cond_signal()` 函数发出信号，通知等待线程条件已经改变。例如：

   ```
   Pthread_mutex_lock(&lock);
   ready = 1;
   Pthread_cond_signal(&cond);
   Pthread_mutex_unlock(&lock);
   ```

   - 在发出信号时，必须确保持有锁。这样可以避免在信号发出和条件检查之间发生竞态条件。

4. **注意事项**：

   - **使用 `while` 循环而不是 `if`**：等待线程在 `while` 循环中重新检查条件比使用 `if` 更加安全，因为有些实现可能会错误地唤醒线程。如果条件没有满足，线程会再次进入休眠状态。

   - **避免自旋等待**：不要使用忙等待（busy-waiting）来轮询条件的变化，如下面的错误示例：

     ```
     while (ready == 0)
         ; // 自旋等待
     ```

     这种方式会浪费 CPU 资源，并且容易出错。正确的做法是使用条件变量来控制线程间的同步。

### 27.5 编译和运行

编译多线程代码时，需要包含 `pthread.h` 头文件，并在链接时指定 `-pthread` 标记。例如：

```
gcc -o main main.c -Wall -pthread
```

这将编译一个使用 pthread 库的多线程程序。

### 27.6 小结

本章介绍了 pthread 库的基本功能，包括线程创建、使用锁进行互斥访问，以及通过条件变量进行线程间的同步信号传递。虽然这些 API 并不复杂，但编写健壮的多线程程序需要小心和细致。

编写并发程序的真正难点在于构建正确的并发逻辑，而不仅仅是调用 API。继续学习并掌握这些工具，将帮助你开发高效且可靠的多线程程序。



![image-20240828012418193](image/image-20240828012418193.png)