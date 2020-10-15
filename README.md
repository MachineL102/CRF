# CRF完美并行  -- （核心部分已实现，crf_learn可以执行）
条件随机场的C++thread并行实现（主要使用线程池技术）
在深入阅读CRF理论和大量学习资料的基础上，分析了可能存在的数据竞争问题和更好的并行策略，参考了CRF++源码，实现了充分利用计算机资源的更优的并行算法，并在实现代码中附有详细注释<br>

CRF++源码：https://github.com/taku910/crfpp
CRF++源码解读：https://blog.csdn.net/aws3217150/article/details/69212445

* 改进了CRF++的不足之处
  * Graph的多次不必要的初始化
  * 多次创建和销毁线程占用了大量的时间
  * 训练算法是单线程实现
* 该版本的不足之处
  * 结构紊乱，异常处理，LBFGS并行算法（如果这一点完成，速度应该可以超过CRF++）

## 阅读该源码可以学到什么
* 结合代码层面更好的学习条件随机场
* 如何更好的设计并行程序
  * 锁
  * 条件变量
  * 线程池的实现
  * 线程安全的数据结构
  * 多个向量相加的并行实现
* 进一步熟悉C++语言
* 如何使用C++扩展python

## 关于为什么要实现算法
* 目前好用的machine learning库均来源于国外(sklearn, tensorflow, pytorch)，自主研发警告。
* 引用一段话（知乎发现的）
Quora上对于Yoshua Bengio的最新访谈，关于一个问题（看巨佬是怎么说的）：
对于进入机器学习领域的年轻研究人员，你有什么建议？（What advise do you have for young researchers entering the field of Machine Learning?）他回答到：Make sure you have a strong training in math and computer science (including the practical part, i.e., programming). Read books and (lots of) papers but that is not enough: you need to develop your intuitions by (a) programming a bunch of learning algorithms yourself, e.g., trying to reproduce existing papers, and (b) by learning to tune hyper-parameters and explore variants (in architecture, objective function, etc.), e.g., by participating in competitions or trying to improve on published results once you have been able to reproduce them. Then, find collaborators with whom you can brainstorm about ideas and share the workload involved in exploring and testing new ideas.  Working with an existing group is ideal, of course, or recruit your own students to work on this with you, if you are a faculty.
<br>划重点---扎实的数学和计算机能力（包括实践---编程）

## 结语
* 丰富的注释使得该项目是一个很好的学习资料
* 和真正投入生产的距离主要是（主要包括-更好的优化算法，更好的异常处理，更好的设计模式）
* 同时也是一名机器学习算法（尤其是概率图方向）和C++程序设计的学习者，大家有什么问题或意见都可以提出，一一回复。
* 邮箱：995220358@qq.com（一名应统研究生，机器学习和计算机理论爱好者）
