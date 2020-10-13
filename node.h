#pragma once
#include <vector>
#include <numeric>
#include "path.h"
#include <algorithm>

#define MINUS_LOG_EPSILON  50

inline double logsumexp(double x, double y, bool flg) {
	if (flg) return y;  // init mode
	const double vmin = std::min(x, y);
	const double vmax = std::max(x, y);
	if (vmax > vmin + MINUS_LOG_EPSILON) {
		// 如果 vmax 大太多，那么对于 exp(vmax) + exp(vmin) 而言，后者可以忽略
		return vmax;
	}
	else {
		return vmax + log(exp(vmin - vmax) + 1.0);
		// x + log(exp(y-x) + 1) = log(exp(y)+exp(x))
	}
}
struct Path;
struct Node {	// yt时刻的状态为stata的节点
	int x;
	int stata;
	double alpha;
	double beta;
	double cost;
	double bestCost;
	Node* prev;	// 到达此节点的最短路径的上一个节点

	Node(int t, int y) : x(t), stata(y) {}
	std::vector<int> fvector;	// 该节点（t时刻第y个状态）出现的19个特征的索引，一开始该时刻所有节点的特征索引相同
	std::vector<Path*> left_path;	// 该节点的特征权重和 + 与该节点连接边的特征权重和(vector：对应了左边好几条边)
	std::vector<Path*> right_path;	// 该节点的特征权重和 + 与该节点连接边的特征权重和(vector：对应了右边好几条边)

	void calcAlpha();

	void calcBeta();

	void calccost(std::vector<double>& weights);

	void calcExpectation(std::vector<double>& expected, double Z, size_t size) const;
};