#pragma once
#include "node.h"
struct Node;
struct Path {
	// 没有记录时间y_t，这个由path_的size记录
	double cost = 0.0;	// tk(yi−1,yi,x,i), st(yi,x,i)  这两种特征函数的权重和
	int left_stata;
	int right_stata;;
	Node* left;
	Node* right;
	std::vector<int> fvector;	// 边特征index

	Path(int i, int j) : left_stata(i), right_stata(j), left(NULL), right(NULL) {}
	void calcExpectation(std::vector<double>& expected, double Z, int size) const;

	void calccost(std::vector<double>& weights, int size);
};