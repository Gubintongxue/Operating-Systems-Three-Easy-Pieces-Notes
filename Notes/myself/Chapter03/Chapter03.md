## 第三章 关于虚拟化的对话

```
教授：现在我们开始讲操作系统 3 个部分的第 1 部分——虚拟化。
学生：尊敬的教授，什么是虚拟化？
教授：想象我们有一个桃子。
学生：桃子？（不可思议）
教授：是的，一个桃子，我们称之为物理（physical）桃子。但有很多想吃这个桃子的
人，我们希望向每个想吃的人提供一个属于他的桃子，这样才能皆大欢喜。我们把给每个
人的桃子称为虚拟（virtual）桃子。我们通过某种方式，从这个物理桃子创造出许多虚拟桃
子。重要的是，在这种假象中，每个人看起来都有一个物理桃子，但实际上不是。
学生：所以每个人都不知道他在和别人一起分享一个桃子吗？
教授：是的。
学生：但不管怎么样，实际情况就是只有一个桃子啊。
教授：是的，所以呢？
学生：所以，如果我和别人分享同一个桃子，我一定会发现这个问题。
教授：是的！你说得没错。但吃的人多才有这样的问题。多数时间他们都在打盹或者
做其他事情，所以，你可以在他们打盹的时候把他手中的桃子拿过来分给其他人，这样我
们就创造了有许多虚拟桃子的假象，每人一个桃子！
学生：这听起来就像糟糕的竞选口号。教授，您是在跟我讲计算机知识吗？
教授：年轻人，看来需要给你一个更具体的例子。以最基本的计算机资源 CPU 为例，
假设一个计算机只有一个 CPU（尽管现代计算机一般拥有 2 个、4 个或者更多 CPU），虚拟
化要做的就是将这个 CPU 虚拟成多个虚拟 CPU 并分给每一个进程使用，因此，每个应用都
以为自己在独占 CPU，但实际上只有一个 CPU。这样操作系统就创造了美丽的假象——它
虚拟化了 CPU。
学生：听起来好神奇，能再多讲一些吗？它是如何工作的？
教授：问得很及时，听上去你已经做好开始学习的准备了。
学生：是的，不过我还真有点担心您又要讲桃子的事情了。
教授：不用担心，毕竟我也不喜欢吃桃子。那我们开始学习吧……
```

