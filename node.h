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
		// ��� vmax ��̫�࣬��ô���� exp(vmax) + exp(vmin) ���ԣ����߿��Ժ���
		return vmax;
	}
	else {
		return vmax + log(exp(vmin - vmax) + 1.0);
		// x + log(exp(y-x) + 1) = log(exp(y)+exp(x))
	}
}
struct Path;
struct Node {	// ytʱ�̵�״̬Ϊstata�Ľڵ�
	int x;
	int stata;
	double alpha;
	double beta;
	double cost;
	double bestCost;
	Node* prev;	// ����˽ڵ�����·������һ���ڵ�

	Node(int t, int y) : x(t), stata(y) {}
	std::vector<int> fvector;	// �ýڵ㣨tʱ�̵�y��״̬�����ֵ�19��������������һ��ʼ��ʱ�����нڵ������������ͬ
	std::vector<Path*> left_path;	// �ýڵ������Ȩ�غ� + ��ýڵ����ӱߵ�����Ȩ�غ�(vector����Ӧ����ߺü�����)
	std::vector<Path*> right_path;	// �ýڵ������Ȩ�غ� + ��ýڵ����ӱߵ�����Ȩ�غ�(vector����Ӧ���ұߺü�����)

	void calcAlpha();

	void calcBeta();

	void calccost(std::vector<double>& weights);

	void calcExpectation(std::vector<double>& expected, double Z, size_t size) const;
};