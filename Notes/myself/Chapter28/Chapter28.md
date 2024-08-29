## 第28章 锁

​		通过对并发的介绍，我们看到了并发编程的一个最基本问题：我们希望原子式执行一系列指令，但由于单处理器上的中断（或者多个线程在多处理器上并发执行），我们做不到。本章介绍了锁（lock），直接解决这一问题。程序员在源代码中加锁，放在临界区周围，保证临界区能够像单条原子指令一样执行。

### 28.1 锁的基本思想

在并发编程中，锁（lock）是一种机制，用于确保只有一个线程能够访问共享资源或进入临界区。临界区是一段访问共享资源的代码，这段代码必须原子性地执行，以防止数据竞争和不确定性问题。通过加锁，我们可以控制对共享资源的访问，确保数据的一致性和正确性。

举个简单的例子，假设有一个共享变量 `balance`，多个线程需要对它进行更新操作：

```
balance = balance + 1;
```

在多线程环境下，如果不加锁，不同线程的 `balance` 更新操作可能会发生冲突，导致错误的结果。为了避免这种情况，我们可以使用锁来保护这段代码，使其成为一个临界区：

```
lock_t mutex; // 定义一个全局锁变量 'mutex'
...
lock(&mutex); // 获取锁
balance = balance + 1; // 临界区
unlock(&mutex); // 释放锁
```

在这个代码段中，我们首先声明了一个锁变量 `mutex`。在进入临界区之前，线程首先调用 `lock(&mutex)` 获取锁。如果锁是可用的，线程将获得锁并进入临界区执行代码。如果锁已被其他线程占用，线程将阻塞，直到锁变为可用。执行完临界区代码后，线程调用 `unlock(&mutex)` 释放锁，使其他线程可以获取锁并进入临界区。

通过锁的机制，我们可以控制线程的执行顺序，避免多个线程同时进入临界区，确保线程之间的互斥访问。

### 28.2 Pthread 锁

在 POSIX 线程库（Pthreads）中，锁被称为互斥量（mutex）。互斥量用于在线程之间实现互斥，防止多个线程同时进入临界区，从而避免数据竞争。下面是一个使用 Pthreads 实现锁机制的示例：

```
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

Pthread_mutex_lock(&lock); // 获取锁
balance = balance + 1; // 临界区
Pthread_mutex_unlock(&lock); // 释放锁
```

在这个示例中，我们使用了 `pthread_mutex_t` 类型的 `lock` 变量，并通过 `PTHREAD_MUTEX_INITIALIZER` 进行初始化。在进入临界区之前，线程调用 `Pthread_mutex_lock(&lock)` 获取锁。如果锁可用，线程将获得锁并继续执行临界区代码。如果锁已被占用，线程将阻塞，直到锁变为可用。执行完临界区代码后，线程调用 `Pthread_mutex_unlock(&lock)` 释放锁。

POSIX 提供的锁机制允许程序员根据需要使用不同的锁来保护不同的数据结构或变量。这种细粒度的锁策略有助于提高并发性，允许多个线程同时进入不同的临界区，从而提高程序的性能。

### 总结

锁是并发编程中确保多个线程安全访问共享资源的重要工具。通过锁，程序员可以控制线程的执行顺序，避免数据竞争和不确定性问题。Pthreads 提供的互斥量机制使得锁的使用更加灵活和高效，适用于多线程环境中的各种并发控制需求。

### 28.3 实现一个锁

为了实现锁，我们需要考虑以下几个方面：硬件支持、操作系统的支持，以及锁的效率和公平性等因素。

#### 关键问题：怎样实现一个锁？

**构建高效的锁**：我们希望锁能够以低成本提供互斥，同时具备一些特性，如公平性、避免线程饿死等。为了实现这一点，我们需要依赖硬件支持和操作系统的帮助。

在硬件方面，现代计算机体系结构通常提供了一些基本的同步原语，例如原子指令（如 `test-and-set`、`compare-and-swap` 等），这些原语能够在硬件层面上确保某些操作的原子性。通过使用这些原语，我们可以实现简单而有效的锁机制。

操作系统则提供了更高级别的支持，比如线程管理、锁机制的抽象，以及在多处理器环境中处理并发的功能。操作系统通常提供锁的标准库，程序员可以直接调用这些库函数，而不需要关注底层实现的细节。

#### 28.4 评价锁

在实现锁之前，我们需要明确锁的评价标准：

1. **互斥性**：锁的最基本功能是提供互斥，确保在同一时刻只有一个线程可以进入临界区。有效的锁必须能够防止多个线程同时进入临界区。
2. **公平性**：锁的实现应当是公平的，即当多个线程竞争锁时，每个线程应该有公平的机会获得锁。公平性问题也涉及到避免线程饿死（某个线程始终无法获得锁）。
3. **性能**：锁的性能主要考虑在不同场景下的开销：
   - **无竞争**：当只有一个线程尝试获取和释放锁时，锁的开销应该尽可能小。
   - **单处理器上的多线程竞争**：在单处理器上，多个线程竞争锁的情况下，锁的开销如何。
   - **多处理器上的多线程竞争**：在多处理器环境下，多个线程在不同处理器上竞争锁的性能表现。

通过分析这些场景，我们可以更好地理解不同锁实现的优缺点。

### 28.5 控制中断

在单处理器系统中，实现锁的早期方法之一是通过关闭中断来实现互斥。这种方法的代码如下：

```
void lock() { 
    DisableInterrupts(); 
} 
void unlock() { 
    EnableInterrupts(); 
}
```

这个方法的原理是在进入临界区前关闭中断，确保临界区的代码不会被打断，从而保证了代码的原子性。

#### 优点：

- **简单**：关闭中断是一种非常直接的方式，易于理解和实现。

#### 缺点：

- **信任问题**：这种方法要求允许所有调用线程执行特权操作（如关闭中断），这可能导致安全性问题。例如，恶意程序可以通过关闭中断并进入死循环来独占处理器。
- **不支持多处理器**：在多处理器系统中，关闭中断只能影响当前处理器，而不能阻止其他处理器上的线程进入临界区。
- **中断丢失**：关闭中断可能导致系统错过一些重要的中断信号，如硬件设备完成任务的通知，可能导致系统无法正常运行。
- **效率低下**：在现代处理器中，关闭和打开中断的操作相对较慢，影响系统性能。

由于这些缺点，关闭中断的方法通常只在一些特殊情况下使用，例如操作系统内核中需要临时保证原子性的操作。在一般的应用程序中，这种方法并不适合用来实现锁。



#### 补充：DEKKER 算法和 PETERSON 算法

​		**20 世纪 60 年代，Dijkstra 向他的朋友们提出了并发问题，他的数学家朋友 Theodorus Jozef Dekker想出了一个解决方法。不同于我们讨论的需要硬件指令和操作系统支持的方法，Dekker 的算法（Dekker’s algorithm）只使用了 load 和 store（早期的硬件上，它们是原子的）。Peterson 后来改进了 Dekker 的方法[P81]。同样只使用 load 和 store，保证不会有两个线程同时进入临界区。以下是 Peterson 算法（Peterson’s algorithm，针对两个线程），读者可以尝试理解这些代码吗？flag 和 turn 变量是用来做什么的？**

```
int flag[2]; 
int turn; 
void init() { 
 flag[0] = flag[1] = 0; // 1->thread wants to grab lock 
 turn = 0; // whose turn? (thread 0 or 1?) 
} 
void lock() { 
 flag[self] = 1; // self: thread ID of caller 
 turn = 1 - self; // make it other thread's turn 
 while ((flag[1-self] == 1) && (turn == 1 - self)) 
 ; // spin-wait 
} 
void unlock() { 
 flag[self] = 0; // simply undo your intent 
}
```

​		**一段时间以来，出于某种原因，大家都热衷于研究不依赖硬件支持的锁机制。后来这些工作都没有太多意义，因为只需要很少的硬件支持，实现锁就会容易很多（实际在多处理器的早期，就有这些硬件支持）。而且上面提到的方法无法运行在现代硬件（应为松散内存一致性模型），导致它们更加没有用处。更多的相关研究也湮没在历史中……**







### 28.6 测试并设置指令（原子交换）

由于在多处理器环境中关闭中断的方法不能工作，系统设计者为硬件引入了对锁的支持。最早的多处理器系统，如20世纪60年代早期的Burroughs B5000，就已经包含了这种支持。如今，所有系统（甚至单处理器系统）都具备了这样的功能。

最简单的硬件支持形式是**测试并设置指令**（test-and-set instruction），也被称为**原子交换**（atomic exchange）。为了理解 `test-and-set` 指令如何工作，我们先实现一个不依赖它的锁，该锁使用一个简单的变量来标记锁是否被持有。

在初次尝试中，我们可以使用一个变量来表示锁是否被线程占用。下图 28.1 显示了这一方法的代码实现。

```
typedef struct lock_t { 
    int flag; 
} lock_t; 

void init(lock_t *mutex) { 
    // 0 -> lock is available, 1 -> held 
    mutex->flag = 0; 
} 

void lock(lock_t *mutex) { 
    while (mutex->flag == 1) // TEST the flag 
        ; // spin-wait (do nothing) 
    mutex->flag = 1; // now SET it! 
} 

void unlock(lock_t *mutex) { 
    mutex->flag = 0; 
}
```

*图 28.1 第一次尝试：简单标志*

在这个实现中，当第一个线程进入临界区并调用 `lock()` 时，它会检查标志是否为1（如果标志不是1），然后将标志设置为1，表示该线程持有锁。当该线程离开临界区时，会调用 `unlock()`，清除标志，表示锁未被持有。

然而，这个实现有两个显著的问题：**正确性**和**性能**。

#### 正确性问题

考虑表 28.1 中的执行场景，假设 `flag = 0`： *表 28.1 追踪：没有互斥*

| 线程1               | 线程2                      |
| ------------------- | -------------------------- |
| 调用 `lock()`       |                            |
| `while (flag == 1)` |                            |
| 中断：切换到线程2   | 调用 `lock()`              |
|                     | `while (flag == 1)`        |
|                     | `flag = 1;`                |
| 中断：切换回线程1   | `flag = 1; // 再次设置为1` |

在这个场景中，由于适时的中断，两个线程都将标志设置为1，从而都进入了临界区。这种行为显然没有满足最基本的互斥要求。

#### 性能问题

性能问题主要出现在**自旋等待**（spin-waiting）时，即当一个线程在等待另一个线程释放锁时，采用了自旋的方式。这种方式会浪费CPU时间，尤其是在单处理器上，等待的线程甚至无法运行目标线程，因为它被上下文切换阻止了。我们需要一个更成熟的解决方案来避免这种浪费。



#### 原文：

​		因为关闭中断的方法无法工作在多处理器上，所以系统设计者开始让硬件支持锁。最早的多处理器系统，像 20 世纪 60 年代早期的 Burroughs B5000[M82]，已经有这些支持。今天所有的系统都支持，甚至包括单 CPU 的系统。

​		最简单的硬件支持是测试并设置指令（test-and-set instruction），也叫作原子交换（atomic exchange）。为了理解 test-and-set 如何工作，我们首先实现一个不依赖它的锁，用一个变量标记锁是否被持有。

​		在第一次尝试中（见图 28.1），想法很简单：用一个变量来标志锁是否被某些线程占用。第一个线程进入临界区，调用 lock()，检查标志是否为 1（这里不是 1），然后设置标志为 1，表明线程持有该锁。结束临界区时，线程调用 unlock()，清除标志，表示锁未被持有。

```C
1 typedef struct lock_t { int flag; } lock_t; 
2 
3 void init(lock_t *mutex) { 
4 // 0 -> lock is available, 1 -> held 
5 mutex->flag = 0; 
6 } 
7 
8 void lock(lock_t *mutex) { 
9 while (mutex->flag == 1) // TEST the flag 
10 ; // spin-wait (do nothing) 
11 mutex->flag = 1; // now SET it! 
12 } 
13 
14 void unlock(lock_t *mutex) { 
15 mutex->flag = 0; 
16 }
```

​		当第一个线程正处于临界区时，如果另一个线程调用 lock()，它会在 while 循环中自旋等待（spin-wait)，直到第一个线程调用 unlock()清空标志。然后等待的线程会退出 while 循环，设置标志，执行临界区代码。

​		遗憾的是，这段代码有两个问题：正确性和性能。这个正确性问题在并发编程中很常见。假设代码按照表 28.1 执行，开始时 flag=0。

![image-20240829111813174](image/image-20240829111813174.png)

​		从这种交替执行可以看出，通过适时的（不合时宜的？）中断，我们很容易构造出两个线程都将标志设置为 1，都能进入临界区的场景。这种行为就是专家所说的“不好”，我们显然没有满足最基本的要求：互斥。

​		性能问题（稍后会有更多讨论）主要是线程在等待已经被持有的锁时，采用了自旋等待（spin-waiting）的技术，就是不停地检查标志的值。自旋等待在等待其他线程释放锁的时候会浪费时间。尤其是在单处理器上，一个等待线程等待的目标线程甚至无法运行（至少在上下文切换之前）！我们要开发出更成熟的解决方案，也应该考虑避免这种浪费。





### 28.7 实现可用的自旋锁

尽管前面的实现思路很好，但没有硬件支持就无法实现。幸运的是，现代系统提供了这种硬件支持，可以基于这个概念创建简单的锁。这个更强大的指令被称为**测试并设置指令**（test-and-set instruction），也叫作**原子交换**（atomic exchange）。在不同平台上，这个指令有不同的名字：在 SPARC 上叫 `ldstub`（load/store unsigned byte），在 x86 上叫 `xchg`（atomic exchange）。它们基本上实现了相同的功能。

我们用如下的C代码片段定义了测试并设置指令的功能：

```
int TestAndSet(int *old_ptr, int new) { 
    int old = *old_ptr; // fetch old value at old_ptr 
    *old_ptr = new; // store 'new' into old_ptr 
    return old; // return the old value 
}
```

这个函数返回 `old_ptr` 指向的旧值，同时更新为 `new` 值。关键在于这些操作是原子地执行的。基于此，我们可以实现一个简单的自旋锁，如图 28.2 所示。

```
typedef struct lock_t { 
    int flag; 
} lock_t; 

void init(lock_t *lock) { 
    // 0 indicates that lock is available, 1 that it is held 
    lock->flag = 0; 
} 

void lock(lock_t *lock) { 
    while (TestAndSet(&lock->flag, 1) == 1) 
        ; // spin-wait (do nothing) 
} 

void unlock(lock_t *lock) { 
    lock->flag = 0; 
}
```

*图 28.2 利用测试并设置指令的简单自旋锁*

这个锁的工作原理如下：

1. 如果一个线程调用 `lock()` 并且锁是空闲的（`flag = 0`），那么 `TestAndSet()` 会返回 0，线程将获取锁，并将 `flag` 设置为1，表示锁已被持有。
2. 如果另一个线程也尝试获取同一个锁（`flag = 1`），它会在 `TestAndSet()` 中自旋，直到 `flag` 被解锁（即被设置为0）。一旦锁被释放，`TestAndSet()` 会返回0，并将 `flag` 设置为1，从而获取锁。

这种自旋锁利用了硬件的支持，实现了有效的互斥。然而，在单处理器系统中，它可能会引发效率问题。多个线程在竞争锁时，可能导致高的CPU使用率，但无法做有意义的工作。为了解决这个问题，需要采用更复杂的锁机制，例如自旋锁与阻塞锁的结合，或其他高级锁设计。

###  提示：从恶意调度程序的角度思考并发

为了理解并发编程中的问题，试着从恶意调度程序的角度思考。假设调度程序会在最不合时宜的时候中断线程，这有助于发现并解决并发代码中的潜在问题。通过这种思维方式，我们可以更好地理解如何构建可靠的同步原语，并避免竞争条件带来的问题。

这种思考方式对于编写健壮的并发程序至关重要。每个细节都可能导致程序行为的显著变化，必须认真对待并加以处理。



#### 原文：

​		尽管上面例子的想法很好，但没有硬件的支持是无法实现的。幸运的是，一些系统提供了这一指令，支持基于这种概念创建简单的锁。这个更强大的指令有不同的名字：在 SPARC上，这个指令叫 ldstub（load/store unsigned byte，加载/保存无符号字节）；在 x86 上，是 xchg（atomic exchange，原子交换）指令。但它们基本上在不同的平台上做同样的事，通常称为测试并设置指令（test-and-set）。我们用如下的 C 代码片段来定义测试并设置指令做了什么：

```
1 int TestAndSet(int *old_ptr, int new) { 
2 int old = *old_ptr; // fetch old value at old_ptr 
3 *old_ptr = new; // store 'new' into old_ptr 
4 return old; // return the old value 
5 }
```

​		测试并设置指令做了下述事情。它返回 old_ptr 指向的旧值，同时更新为 new 的新值。当然，关键是这些代码是原子地（atomically）执行。因为既可以测试旧值，又可以设置新值，所以我们把这条指令叫作“测试并设置”。这一条指令完全可以实现一个简单的自旋锁（spin lock），如图 28.2 所示。或者你可以先尝试自己实现，这样更好！

​		我们来确保理解为什么这个锁能工作。首先假设一个线程在运行，调用 lock()，没有其他线程持有锁，所以 flag 是 0。当调用 TestAndSet(flag, 1)方法，返回 0，线程会跳出 while循环，获取锁。同时也会原子的设置 flag 为 1，标志锁已经被持有。当线程离开临界区，调用 unlock()将 flag 清理为 0。

```
1 typedef struct lock_t { 
2 int flag; 
3 } lock_t; 
4 
5 void init(lock_t *lock) { 
6 // 0 indicates that lock is available, 1 that it is held 
7 lock->flag = 0; 
8 } 
9 
10 void lock(lock_t *lock) { 
11 while (TestAndSet(&lock->flag, 1) == 1) 
12 ; // spin-wait (do nothing) 
13 } 
14 
15 void unlock(lock_t *lock) { 
16 lock->flag = 0; 
17 }
图 28.2 利用测试并设置的简单自旋锁
```

​		第二种场景是，当某一个线程已经持有锁（即 flag 为 1）。本线程调用 lock()，然后调用TestAndSet(flag, 1)，这一次返回 1。只要另一个线程一直持有锁，TestAndSet()会重复返回 1，本线程会一直自旋。当 flag 终于被改为 0，本线程会调用 TestAndSet()，返回 0 并且原子地设置为 1，从而获得锁，进入临界区。

​		将测试（test 旧的锁值）和设置（set 新的值）合并为一个原子操作之后，我们保证了只有一个线程能获取锁。这就实现了一个有效的互斥原语！你现在可能也理解了为什么这种锁被称为自旋锁（spin lock）。这是最简单的一种锁，一直自旋，利用 CPU 周期，直到锁可用。在单处理器上，需要抢占式的调度器（preemptive scheduler，即不断通过时钟中断一个线程，运行其他线程）。否则，自旋锁在单 CPU 上无法使用，因为一个自旋的线程永远不会放弃 CPU。

#### 提示：从恶意调度程序的角度想想并发

​		通过这个例子，你可能会明白理解并发执行所需的方法。你应该试着假装自己是一个恶意调度程序（malicious scheduler），会最不合时宜地中断线程，从而挫败它们在构建同步原语方面的微弱尝试。你是多么坏的调度程序！虽然中断的确切顺序也许未必会发生，但这是可能的，我们只需要以此证明某种特定的方法不起作用。恶意思考可能会有用！（至少有时候有用。）



### 28.8 评价自旋锁

现在我们可以使用之前设定的标准来评价自旋锁的表现。首先是**正确性**：自旋锁是否能够保证互斥？答案是肯定的。自旋锁一次只允许一个线程进入临界区，因此它是一个正确的锁。

接下来是**公平性**：自旋锁对等待线程的公平性如何？能否保证一个等待线程最终会进入临界区？答案是不确定的。自旋锁不提供任何公平性保证。在某些情况下，等待线程可能会因为竞争而永远自旋，导致**饿死**的情况。

最后一个标准是**性能**。使用自旋锁的成本是多少？要全面分析性能，我们需要考虑几种不同的情况。

首先，在单处理器环境下，自旋锁的性能表现不佳。如果一个线程持有锁并进入临界区，而此时调度器中断了该线程，其他线程将不得不自旋等待。由于这些线程无法在当前线程释放锁之前获得处理器时间，这种等待实际上浪费了CPU周期。

然而，在多处理器环境下，自旋锁的表现要好得多，特别是当线程数与CPU数量大致相等时。如果线程A在CPU1上运行，线程B在CPU2上自旋等待锁，线程B的自旋不会浪费CPU时间，因为临界区的代码通常很短，锁很快就会释放，线程B将能够立即获得锁并继续执行。

### 28.9 比较并交换

在某些系统中，除了测试并设置指令，还提供了**比较并交换**指令。SPARC系统中称为 `compare-and-swap`，x86系统中称为 `compare-and-exchange`。图28.3展示了这条指令的C语言伪代码。

```
int CompareAndSwap(int *ptr, int expected, int new) { 
    int actual = *ptr; 
    if (actual == expected) 
        *ptr = new; 
    return actual; 
}
```

*图 28.3 比较并交换*

**比较并交换**指令的基本思路是：检查指针`ptr`指向的值是否与`expected`相等。如果相等，则将`ptr`的值更新为`new`，否则什么也不做。无论是否成功，函数都会返回指针`ptr`当前的实际值，以便调用者知道操作是否成功。

有了**比较并交换**指令，我们可以像使用**测试并设置**一样实现一个自旋锁。以下是 `lock()` 函数的实现代码：

```
void lock(lock_t *lock) { 
    while (CompareAndSwap(&lock->flag, 0, 1) == 1) 
        ; // spin 
}
```

其余的代码与使用**测试并设置**指令实现的自旋锁完全相同。这段代码的工作方式与之前的锁类似：它检查`flag`是否为0，如果是，则将其原子性地更新为1，从而获取锁。若锁已经被持有，其他竞争的线程将会自旋等待。



如果你想看看如何创建建 C 可调用的 x86 版本的比较并交换，下面的代码段可能有用（来自[S05])：

```
1 char CompareAndSwap(int *ptr, int old, int new) { 
2 unsigned char ret; 
3 
4 // Note that sete sets a 'byte' not the word 
5 __asm__ __volatile__ ( 
6 " lock\n" 
7 " cmpxchgl %2,%1\n" 
8 " sete %0\n" 
9 : "=q" (ret), "=m" (*ptr) 
10 : "r" (new), "m" (*ptr), "a" (old) 
11 : "memory"); 
12 return ret; 
13 }
```

最后，你可能会发现，比较并交换指令比测试并设置更强大。当我们在将来简单探讨无等待同步（wait-free synchronization）[H91]时，会用到这条指令的强大之处。然而，如果只用它实现一个简单的自旋锁，它的行为等价于上面分析的自旋锁。





### 28.10 链接的加载和条件式存储指令

一些平台提供了用于实现临界区的特定指令组合。例如，MIPS架构中的**链接的加载**（load-linked）和**条件式存储**（store-conditional）指令可用于实现锁等并发结构。图28.4展示了这些指令的C语言伪代码。

```
int LoadLinked(int *ptr) { 
    return *ptr; 
}

int StoreConditional(int *ptr, int value) { 
    if (no one has updated *ptr since the LoadLinked to this address) { 
        *ptr = value; 
        return 1; // success! 
    } else { 
        return 0; // failed to update 
    } 
}
```

*图 28.4 链接的加载和条件式存储*

**链接的加载**指令与普通的加载指令类似，都是从内存中取出一个值并存入寄存器中。关键的区别在于**条件式存储**指令：只有在自从执行链接的加载指令后，指针`ptr`指向的内存没有被其他线程修改时，条件存储才会成功，并将`ptr`的值更新为`value`。成功时，`StoreConditional` 返回 1；失败时，返回 0，并且不会更新值。

以下是使用**链接的加载**和**条件式存储**实现一个锁的代码：

```
void lock(lock_t *lock) { 
    while (1) { 
        while (LoadLinked(&lock->flag) == 1) 
            ; // spin until it's zero 
        if (StoreConditional(&lock->flag, 1) == 1) 
            return; // if set-it-to-1 was a success: all done 
        // otherwise: try it all over again 
    } 
}

void unlock(lock_t *lock) { 
    lock->flag = 0; 
}
```

*图 28.5 使用 LL/SC 实现锁*

在这里，`lock()` 函数的工作方式如下：线程首先自旋等待，直到`flag`被设置为0（表示锁没有被持有）。然后，线程尝试通过条件式存储获取锁。如果成功，线程将`flag`设置为1，并进入临界区。否则，线程将继续自旋并重试。

这种方法在现代系统中非常有效，尤其是在支持松散内存一致性模型的多处理器系统中。

总结：通过这一章节的讨论，我们了解了多种锁的实现方法，包括**测试并设置**和**比较并交换**，以及如何使用**链接的加载**和**条件式存储**指令来构建有效的锁。这些锁机制为并发编程提供了关键的基础。



#### 原文：

一些平台提供了实现临界区的一对指令。例如 MIPS 架构[H93]中，链接的加载（load-linked）和条件式存储（store-conditional）可以用来配合使用，实现其他并发结构。图 28.4 是这些指令的 C 语言伪代码。Alpha、PowerPC 和 ARM 都提供类似的指令[W09]。

```
1 int LoadLinked(int *ptr) { 
2 return *ptr; 
3 } 
4 
5 int StoreConditional(int *ptr, int value) { 
6 if (no one has updated *ptr since the LoadLinked to this address) {
7 *ptr = value; 
8 return 1; // success! 
9 } else { 
10 return 0; // failed to update 
11 } 
12 }
图 28.4 链接的加载和条件式存储
```

​		链接的加载指令和典型加载指令类似，都是从内存中取出值存入一个寄存器。关键区别来自条件式存储（store-conditional）指令，只有上一次加载的地址在期间都没有更新时，才会成功，（同时更新刚才链接的加载的地址的值）。成功时，条件存储返回 1，并将 ptr 指的值更新为 value。失败时，返回 0，并且不会更新值。

​		你可以挑战一下自己，使用链接的加载和条件式存储来实现一个锁。完成之后，看看下面代码提供的简单解决方案。试一下！解决方案如图 28.5 所示。

​		lock()代码是唯一有趣的代码。首先，一个线程自旋等待标志被设置为 0（因此表明锁没有被保持）。一旦如此，线程尝试通过条件存储获取锁。如果成功，则线程自动将标志值更改为 1，从而可以进入临界区。

```
1 void lock(lock_t *lock) { 
2 while (1) { 
3 while (LoadLinked(&lock->flag) == 1) 
4 ; // spin until it's zero 
5 if (StoreConditional(&lock->flag, 1) == 1) 
6 return; // if set-it-to-1 was a success: all done 
7 // otherwise: try it all over again 
8 } 
9 } 
10 
11 void unlock(lock_t *lock) { 
12 lock->flag = 0; 
13 }
图 28.5 使用 LL/SC 实现锁
```

#### 提示：代码越少越好（劳尔定律）

​		**程序员倾向于吹嘘自己使用大量的代码实现某功能。这样做本质上是不对的。我们应该吹嘘以很少的代码实现给定的任务。简洁的代码更易懂，缺陷更少。正如 Hugh Lauer 在讨论构建一个飞行员操作系统时说：“如果给同样这些人两倍的时间，他们可以只用一半的代码来实现”[L81]。我们称之为劳尔定律（Lauer’s Law），很值得记住。下次你吹嘘写了多少代码来完成作业时，三思而后行，或者更好的做法是，回去重写，让代码更清晰、精简。**



​		请注意条件式存储失败是如何发生的。一个线程调用 lock()，执行了链接的加载指令，返回 0。在执行条件式存储之前，中断产生了，另一个线程进入 lock 的代码，也执行链接式加载指令，同样返回 0。现在，两个线程都执行了链接式加载指令，将要执行条件存储。重点是只有一个线程能够成功更新标志为 1，从而获得锁；第二个执行条件存储的线程会失败（因为另一个线程已经成功执行了条件更新），必须重新尝试获取锁。

​		在几年前的课上，一位本科生同学 David Capel 给出了一种更为简洁的实现，献给那些喜欢布尔条件短路的人。看看你是否能弄清楚为什么它是等价的。当然它更短！

```
1 void lock(lock_t *lock) { 
2 while (LoadLinked(&lock->flag)||!StoreConditional(&lock->flag, 1)) 
3 ; // spin 
4 }
```





### 28.11 获取并增加

**获取并增加**（fetch-and-add）是一种简单而强大的硬件原语，能够原子地返回特定内存地址的旧值，并将该值自增1。其C语言伪代码如下：

```
int FetchAndAdd(int *ptr) { 
    int old = *ptr; 
    *ptr = old + 1; 
    return old; 
}
```

这个指令可以用来实现各种同步原语，其中之一就是 **ticket 锁**，由 Mellor-Crummey 和 Michael Scott 提出【MS91】。Ticket 锁的基本思路是让每个线程获取一个递增的 ticket 值，并通过一个 turn 值来确保每个线程按照顺序进入临界区。

以下是实现 ticket 锁的代码示例：

```
typedef struct lock_t { 
    int ticket; 
    int turn; 
} lock_t;

void lock_init(lock_t *lock) { 
    lock->ticket = 0; 
    lock->turn = 0; 
}

void lock(lock_t *lock) { 
    int myturn = FetchAndAdd(&lock->ticket); 
    while (lock->turn != myturn) 
        ; // spin
}

void unlock(lock_t *lock) { 
    FetchAndAdd(&lock->turn); 
}
```

*图 28.6 ticket 锁*

与之前的方法不同，ticket 锁使用了两个变量 `ticket` 和 `turn` 来构建锁。其操作流程如下：

1. 当一个线程希望获取锁时，首先对 `ticket` 变量执行原子的 `FetchAndAdd` 操作。这个操作返回线程的“顺位”`myturn`，表示线程排在队列中的位置。
2. 线程接着检查全局的 `turn` 变量，只有当 `myturn` 等于 `turn` 时，线程才能进入临界区。
3. 当线程释放锁时，`turn` 变量递增，这样下一个等待线程可以进入临界区。

Ticket 锁的一个重要特性是它能够保证所有线程最终都能获得锁，只要它们已经获取了一个 `ticket` 值。这避免了自旋锁可能出现的饿死情况。

### 28.12 自旋过多：怎么办

虽然硬件支持的锁实现非常简单且有效，但在某些场景下，它们可能会表现得效率低下。例如，在单处理器系统上，如果一个线程正在持有锁且被中断，另一个线程尝试获取锁时将会进入自旋等待。而这个线程会一直自旋，直到持有锁的线程被调度回运行并释放锁。

这种情况下，自旋等待线程在整个时间片内都会无效地浪费CPU时间。情况会在有多个线程竞争一个锁时变得更加糟糕：所有线程都会在锁被释放前自旋，从而浪费大量的时间片。

因此，接下来的关键问题是：

#### **关键问题：如何避免自旋？**

**仅靠硬件支持是不够的，我们需要操作系统的帮助来避免这种不必要的自旋。我们将在后续内容中讨论如何通过结合操作系统和硬件特性来实现更高效的锁机制，以减少或者消除自旋等待的浪费。**



### 28.13 简单方法：让出来吧，宝贝

在面对自旋锁浪费CPU资源的问题时，有一种简单且友好的改进方法：在自旋等待时主动放弃CPU，以允许其他线程运行。这种方法被称为“让出来吧，宝贝！”（yield）。图28.7展示了这种方法的实现。

```
void init() { 
    flag = 0; 
} 

void lock() { 
    while (TestAndSet(&flag, 1) == 1) 
        yield(); // give up the CPU 
} 

void unlock() { 
    flag = 0; 
}
```

*图 28.7 测试并设置和让出实现的锁*

在这种方法中，我们假设操作系统提供了一个`yield()`的原语，线程调用它可以主动放弃CPU，从而允许其他线程运行。这样，线程在发现锁被占用时可以避免继续自旋，而是让出CPU时间给其他线程。

#### 评价这种方法

在单CPU系统上，这种方法非常有效。如果一个线程在持有锁时被抢占，其他线程发现锁被占用后，通过`yield()`放弃CPU，持有锁的线程就可以尽快完成它的工作并释放锁，从而让等待的线程继续执行。

然而，在多线程环境下，特别是当有大量线程竞争同一个锁时，这种方法仍然可能会导致大量的上下文切换，影响性能。比如，如果有100个线程在竞争一把锁，一个线程持有锁并被抢占，其他99个线程都可能会进入`yield()`循环，从而导致频繁的上下文切换，浪费大量系统资源。

此外，这种方法也存在潜在的饿死问题。如果一个线程不断地放弃CPU，而其他线程在它重新运行之前抢先获得锁，该线程可能会一直得不到锁的机会。

### 28.14 使用队列：休眠替代自旋

为了避免前述方法中的自旋和饿死问题，我们可以使用队列来控制线程的获取顺序，并让线程在锁不可用时进入休眠状态，避免浪费CPU资源。

Solaris操作系统提供了两个系统调用`park()`和`unpark(threadID)`，可以用来实现这种基于队列的锁。`park()`让调用线程进入休眠状态，`unpark(threadID)`则会唤醒指定的线程。图28.8展示了这种锁的实现：

```
typedef struct lock_t { 
    int flag; 
    int guard; 
    queue_t *q; 
} lock_t;

void lock_init(lock_t *m) { 
    m->flag = 0; 
    m->guard = 0; 
    queue_init(m->q); 
}

void lock(lock_t *m) { 
    while (TestAndSet(&m->guard, 1) == 1) 
        ; // acquire guard lock by spinning 
    if (m->flag == 0) { 
        m->flag = 1; // lock is acquired 
        m->guard = 0; 
    } else { 
        queue_add(m->q, gettid()); 
        m->guard = 0; 
        park(); 
    } 
}

void unlock(lock_t *m) { 
    while (TestAndSet(&m->guard, 1) == 1) 
        ; // acquire guard lock by spinning 
    if (queue_empty(m->q)) 
        m->flag = 0; // let go of lock; no one wants it 
    else 
        unpark(queue_remove(m->q)); // hold lock (for next thread!) 
    m->guard = 0; 
}
```

*图 28.8 使用队列，测试并设置、让出和唤醒的锁*

#### 解释这个锁实现

1. **Guard Lock**: `guard`变量起到了自旋锁的作用，围绕对`flag`和队列的操作。虽然这种方法并没有完全避免自旋等待，但由于自旋的时间非常短（只涉及锁的获取和释放操作，而不是用户定义的临界区），它仍然是一个合理的解决方案。
2. **Park and Unpark**: 当线程发现锁被占用且自己无法获得锁时，它会将自己的线程ID加入等待队列，然后调用`park()`进入睡眠状态。锁释放时，持有锁的线程会调用`unpark()`，唤醒下一个等待的线程。
3. **Wakeup-Waiting Race**: 在调用`park()`之前必须先释放`guard`锁，否则可能会出现唤醒/等待竞争条件，导致线程进入永久睡眠状态。Solaris通过提供`setpark()`系统调用解决了这个问题，确保线程在`park()`之前不会错过`unpark()`的信号。

这种基于队列的锁通过将线程组织成队列，并在锁可用时有序唤醒等待线程，避免了资源浪费和饿死问题，同时保证了公平性。

### 28.15 不同操作系统，不同实现

我们已经讨论了如何通过硬件支持和操作系统机制来实现高效的锁。不同的操作系统可能会提供不同的机制和支持来实现锁，尽管它们的基本思想是类似的。

#### Linux的Futex

Linux提供了一种名为futex（Fast Userspace Mutexes）的机制，它与Solaris的机制类似，但提供了更多内核功能。futex锁与一个特定的物理内存位置相关联，并且有一个预先构建好的内核队列。调用者可以通过futex调用来进行睡眠或者唤醒操作。

futex有两个主要的系统调用：

1. `futex_wait(address, expected)`：当`address`处的值等于`expected`时，让调用线程进入睡眠状态。如果不相等，则调用立即返回。
2. `futex_wake(address)`：唤醒等待队列中的一个线程。

以下是一个基于Linux环境下的futex锁实现示例：

```
void mutex_lock (int *mutex) { 
    int v; 
    /* Bit 31 was clear, we got the mutex (this is the fastpath) */ 
    if (atomic_bit_test_set (mutex, 31) == 0) 
        return; 
    atomic_increment (mutex); 
    while (1) { 
        if (atomic_bit_test_set (mutex, 31) == 0) { 
            atomic_decrement (mutex); 
            return; 
        } 
        /* We have to wait now. First make sure the futex value 
        we are monitoring is truly negative (i.e. locked). */ 
        v = *mutex; 
        if (v >= 0) 
            continue; 
        futex_wait (mutex, v); 
    } 
} 

void mutex_unlock (int *mutex) { 
    /* Adding 0x80000000 to the counter results in 0 if and only if 
    there are no other interested threads */ 
    if (atomic_add_zero (mutex, 0x80000000)) 
        return; 

    /* There are other threads waiting for this mutex, 
    wake one of them up. */ 
    futex_wake (mutex); 
}
```

*图 28.9 基于 Linux 的 futex 锁*

这段代码展示了如何利用futex实现锁。在这种实现中，一个整数同时记录了锁是否被持有（通过整数的最高位）以及等待的线程数量（通过整数的其他位）。如果锁是负的，则表示锁已被持有。

### 28.16 两阶段锁

Linux的futex锁实现中引入了两阶段锁（two-phase lock）的概念。这种锁方法意识到自旋在某些情况下是有用的，特别是在锁即将被释放时。因此，两阶段锁首先自旋一段时间，尝试获取锁；如果未能成功获取锁，则进入第二阶段，线程会进入睡眠状态，直到锁可用。

这种方法的基本思路是结合自旋和休眠两种机制，以实现更高效的锁。具体地，Linux的futex锁在初始阶段会进行自旋（有时只自旋一次），如果自旋无法获取锁，则调用futex进入睡眠状态，等待唤醒。

两阶段锁是一种混合策略，结合了自旋和休眠两种方案，旨在应对不同的硬件环境、线程数量和负载条件。它提供了一种在广泛场景中表现良好的通用锁策略。

### 28.17 小结

现代锁的实现通常结合了硬件支持（如更强大的指令）和操作系统支持（如Solaris的`park()`和`unpark()`原语，以及Linux的futex）。尽管细节上有所不同，但这些锁操作的实现通常高度优化，并针对特定系统环境进行了调整。

这些方法展示了如何在真实的系统中实现高效的锁。了解这些实现有助于理解操作系统和硬件如何协同工作，以提供强大且高效的同步机制。可以通过查看Solaris或Linux的代码来获得更多信息，或参考相关文献以更深入地理解现代多处理器锁策略的比较和分析。