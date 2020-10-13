# CRF
条件随机场的C++并行实现
在深入阅读CRF++和大量学习资料的基础上，修改了CRF++源码，并附有详细注释
* 改进了CRF++的不足之处
  * Graph的多次不必要的初始化
  * 多次创建和销毁线程占用了大量的实践
  * 训练算法是单线程实现
* 该版本的不足之处
  * 对优化算法lbfgs的实现不熟悉（这里欢迎熟悉该算法的同学一起来完成CRF的实现）

## 阅读源码可以学到什么
* 结合代码层面更好的学习条件随机场
* 如何更好的设计并行程序
* 进一步熟悉C++语言

## 关于为什么要实现算法
Quora上对于Yoshua Bengio的最新访谈，关于一个问题：

对于进入机器学习领域的年轻研究人员，你有什么建议？（What advise do you have for young researchers entering the field of Machine Learning?）他回答到：Make sure you have a strong training in math and computer science (including the practical part, i.e., programming). Read books and (lots of) papers but that is not enough: you need to develop your intuitions by (a) programming a bunch of learning algorithms yourself, e.g., trying to reproduce existing papers, and (b) by learning to tune hyper-parameters and explore variants (in architecture, objective function, etc.), e.g., by participating in competitions or trying to improve on published results once you have been able to reproduce them. Then, find collaborators with whom you can brainstorm about ideas and share the workload involved in exploring and testing new ideas.  Working with an existing group is ideal, of course, or recruit your own students to work on this with you, if you are a faculty.
