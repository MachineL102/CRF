#include "path.h"

void Path::calcExpectation(std::vector<double>& expected, double Z, int size) const {
	const double c = exp(left->alpha + cost + right->beta - Z);
	for (int f_index : fvector) {
		expected[f_index + left->stata * size + right->stata] += c; //这里把所有边上相同的转移特征函数对应的概率相加
	}
}

void Path::calccost(std::vector<double>& weights, int y_size) {
	cost = 0.0;
	for (auto index : fvector) {
		cost += weights[index + left_stata * y_size + right_stata];
	}
}