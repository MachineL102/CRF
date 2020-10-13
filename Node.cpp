#include "node.h"
#include <iostream>

void Node::calcAlpha() {
	/*
		当this=node_[1][1]时，下面是
		alpha(s2=y[2])--第二个时刻等于第二个状态 = Σ(s1)  psi(s1,s2)+alpha(s1)
		psi(s1,s2) = exp(Σ(k)  wk * phi(s1,s2,x) )
		psi(s1,s2) = (*it)->cost，由于在对数空间，所以只计算wk的和
		alpha(s1)
	*/
	alpha = 0.0;	// 这里是对数空间的0，相当于初始化为1（针对没有left_path的首节点而言的，因为--看21行）
	for (auto it = left_path.begin(); it != left_path.end(); ++it) {
		// a = log(exp(a) + exp(cost+a'))
		// 该alpha是取对数之后的
		// 以下公式含义为：log(alpha) = logsumexp(alpha, cost'+log(alpha')) 
		// = log(exp(alpha) + exp(cost'+alpha')) = log(exp(alpha) + exp(cost+alpha'))
		// = log(exp(alpha+exp(psi)*exp(alpha')))         cost = Σ wi
		alpha = logsumexp(alpha,
			(*it)->cost + (*it)->left->alpha,

			(it == left_path.begin()));	//	只要有left_path，alpha的初始化就是(*it)->cost + (*it)->left->alpha，相当于M(exp(cost))*alpha(exp(alpha))
	}
	alpha += cost;    // cost：该节点对应权重之和，此处不需要
	//std::cout << alpha << std::endl;
}

void Node::calcBeta() {
	beta = 0.0;
	for (auto it = right_path.begin(); it != right_path.end(); ++it) {
		beta = logsumexp(beta,
			(*it)->cost + (*it)->right->beta,
			(it == right_path.begin()));
	}
	beta += cost;
}

void Node::calccost(std::vector<double>& weights) {
	cost = 0.0;
	for (auto index : fvector) {
		cost += weights[index + stata];
	}
}

void Node::calcExpectation(std::vector<double>& expected, double Z, size_t size) const {
	// 每个节点的特征索引: phi(Sk = Si, X)=1 时候的 Wi

	// 前面alpha，beta各加了一个cost，所以此处减去一个即可 cost = 
	const double c = exp(alpha + beta - cost - Z);   // p(sk|x)：alpha*beta/cost/z 
	
	// p(sk|x) 的真正含义是 p(sk=si|),即对于该输入的状态序列时间k是状态i的概率，所以每个节点一个c
	// 该节点cost = 势函数 ψ(sj−1,sj)，节点cost+边cost（对应的特征函数索引的权重）
	// 每个节点60个特征函数，20个特征函数索引
	for (int f_index : fvector) {
		expected[f_index + stata] += c;           //这里会把所有节点的相同状态特征函数对应的节点概率相加，特征函数值*概率再加和便是期望。由于特征函数值为1，所以直接加概率值
	}
	for (auto it = left_path.begin(); it != left_path.end(); ++it) { //转移特征的期望
		(*it)->calcExpectation(expected, Z, size);
	}
}