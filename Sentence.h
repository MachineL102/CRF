#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <sstream>
#include "node.h"
#include "path.h"

//	2043ms	2059ms
void split(const std::string& s, std::vector<std::string>& sv, const char flag = ' ') {
	sv.clear();
	std::istringstream iss(s);
	std::string temp;

	auto begin = s.begin();
	auto end = s.end();
	
	while (true) {
		auto pre = begin;
		begin = std::find(begin, end, flag);
		sv.emplace_back(std::string(pre, begin));	//	耗时
		if (begin == end) break;
		++begin;
	}
	return;
}

// 642ms	726ms
std::string split(const std::string& s, size_t i, const char flag = ' ') {
	std::istringstream iss(s);
	std::string temp;

	auto begin = s.begin();
	auto end = s.end();

	size_t cmp = 0;
	while (true) {
		auto pre = begin;
		begin = std::find(begin, end, flag);
		if (cmp == i) return std::string(pre, begin);
		++cmp;
	}
	return "";
}

class Sentence {
private:
	int x_size = 0;
	double Z_ = 0.0;
	std::vector<std::string> lines;
	std::vector<std::vector<int>>  feature_index;	// 每个单词20个特征索引，即在几万个特征中满足20个特征
	std::vector<std::vector<Node*>> node_;
	std::vector<std::vector<Path*>> path_;
	std::vector<int> result_;
	double cost_; //  viterbi最优序列的损失
public:
	std::vector<int> answer_;
	static int index;
	static int y_size;
	static std::map <std::string, std::pair<int, unsigned int> > dict_;
	static int total_feature;

	int eval() {
		int err = 0;
		for (int i = 0; i < x_size; ++i) {
			if (answer_[i] != result_[i]) {
				++err;
			}
		}
		return err;
	}

	void viterbi() {
		for (int i = 0; i < x_size; ++i) {
			for (int j = 0; j < y_size; ++j) {
				double bestc = -1e37;
				Node* best = 0;
				auto lpath = node_[i][j]->left_path;
				if (lpath.empty()) { double cost = node_[i][j]->cost; best = NULL; bestc = cost; }
				for (auto it = lpath.begin(); it != lpath.end(); ++it) {
					double cost = (*it)->left->bestCost + (*it)->cost +
						node_[i][j]->cost;
					if (cost > bestc) {
						bestc = cost;
						best = (*it)->left;
					}
				}
				node_[i][j]->prev = best;
				node_[i][j]->bestCost = best ? bestc : node_[i][j]->cost;
			}
		}
		double bestc = -1e37;
		Node* best = 0;
		int s = x_size - 1;
		for (int j = 0; j < y_size; ++j) {
			if (bestc < node_[s][j]->bestCost) {
				best = node_[s][j];
				bestc = node_[s][j]->bestCost;
			}
		}

		result_.resize(x_size);
		for (Node* n = best; n; n = n->prev) {
			result_[n->x] = n->stata;
		}

		cost_ = -node_[x_size - 1][result_[x_size - 1]]->bestCost;
	}

	double calcGradient(std::vector<double>& expected, std::vector<double>& weights) {
		double s = 0;
		for (int i = 0; i < x_size; ++i) { //遍历每一个节点的，遍历计算每个节点和每条边上的特征函数，计算每个特征函数的期望
			for (int j = 0; j < y_size; ++j) {
				node_[i][j]->calcExpectation(expected, Z_, y_size);
			}
		}

		// 遍历走过的train_data节点
		for (int i = 0; i < x_size; ++i) { //遍历每一个位置（词）
			for (auto f_index: node_[i][answer_[i]]->fvector) { //answer_[i]代表当前样本的label，遍历每个词当前样本label的特征，进行减1操作，遍历所有节点减1就相当于公式中fj(y,x)
				--expected[f_index + answer_[i]]; //梯度：状态特征函数期望减去真实的状态特征函数值
			}
			s += node_[i][answer_[i]]->cost;  // UNIGRAM cost
		}

		// 遍历走过的train_data路径
		for (int path_index = 0; path_index < x_size - 1; ++path_index) {
			//将(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2), (2, 0), (2, 1), (2, 2)转化为0 - 8的index
			int tmp = answer_[path_index] * 3 + answer_[path_index + 1];
			for (auto f_index : path_[path_index][tmp]->fvector) { //answer_[i]代表当前样本的label，遍历每个词当前样本label的特征，进行减1操作，遍历所有节点减1就相当于公式中fj(y,x)
				--expected[f_index]; //梯度：状态特征函数期望减去真实的状态特征函数值
			}
			s += path_[path_index][tmp]->cost;
		}
		return Z_ - s;
	}

	void buildGraph() {
		for (int i = 0; i < x_size; i++) {
			node_.push_back(std::vector<Node*>());
			for (int j = 0; j < y_size; j++) {
				Node* cur_node = new Node(i, j);
				node_.back().push_back(cur_node);	// 这里传入node类的初始化参数(可以有多个)就可以跳过拷贝构造函数或移动构造函数，从而原地构造
				cur_node->fvector = feature_index[i];
			}
		}
		for (int i = 1; i < x_size; i++) {
			path_.push_back(std::vector<Path*>());
			for (int j = 0; j < y_size; j++) {
				for (int k = 0; k < y_size; k++) {
					Path* cur_path = new Path(j, k);
					path_.back().push_back(cur_path);	// 这里传入node类的初始化参数(可以有多个)就可以跳过拷贝构造函数或移动构造函数，从而原地构造
					cur_path->left = node_[i-1][j];
					cur_path->right = node_[i][k];
					cur_path->fvector = feature_index[i+x_size];
					node_[i][k]->left_path.push_back(cur_path);
					node_[i-1][j]->right_path.push_back(cur_path);
				}
			}
		}
	};

	void dp(std::vector<double>& weights) {
		for (auto v_path : path_) {
			for (auto path : v_path) {
				path->calccost(weights, y_size);
			}
		}
		for (int i = 0; i < x_size; i++) {
			for (int j = 0; j < y_size; j++) {
				node_[i][j]->calccost(weights);
				node_[i][j]->calcAlpha();
			}
		}
		for (int i = x_size - 1; i >= 0; i--) {
			for (int j = 0; j < y_size; j++) {
				node_[i][j]->calcBeta();
			}
		}
		Z_ = 0.0; // 仍然为对数空间中的数据(取了log之后的)
		for (size_t j = 0; j < y_size; ++j) {
			// log(exp(Z_) + exp(beta))     : 因为beta代表的是 log(beta)，所以先exp
			// 而ψ(∗,s1) = 1，所以将 exp(beta)相加即可
			// Z_：ψ(x)=β(s0)=∑s1ψ(∗,s1)β(s1)
			Z_ = logsumexp(Z_, node_[0][j]->beta, j == 0);
		}
	}

	// 是否真的没有调用复制构造，修改源码
	// std::string&& line  +  emplace_back 两次移动可能更好
	void push_back(std::string line) { // 这里line不能是&或*，因为line可能是不断变化的临时变量，所以这里必须复制副本
		lines.push_back(line);	// 直接在data申请好的空间中直接构造line这个变量，push_back：构造line后，移动到data的空间中
		x_size++;
	}

	// input：shared
	void buildFeature(std::vector<std::string>& unigram_templs, std::vector<std::string>& bigram_templs) {
		std::string seq = ",";
		std::vector<int> feature;
		for (size_t i = 0; i < lines.size(); i++) {
			for (std::vector<std::string>::const_iterator it = unigram_templs.begin(); it != unigram_templs.end(); ++it) {

				// 在U05:%x[-1,0]/%x[0,0] 中找 ","
				std::string::const_iterator seq_it = std::find_first_of((*it).begin(), (*it).end(), seq.begin(), seq.end());

				// key = U05:like
				int x = *(seq_it - 1) - 48;
				int y = *(seq_it + 1) - 48;
				if (*(seq_it - 2) == '-') x = -x;
				std::string key = templsToKey(i, x, y, *it); 

				// 寻找有没有第二个，如果有，扩充key为 U05:like U05:come
				seq_it = std::find_first_of(seq_it+1, (*it).end(), seq.begin(), seq.end());
				if (seq_it != (*it).end()) {
					x = *(seq_it - 1) - 48;
					y = *(seq_it + 1) - 48;
					if (*(seq_it - 2) == '-') x = -x;
					// std::string key = templsToKey(i, x, y, *it);
					key += ' ';
					key += templsToKey(i, x, y, *it);
				}
				
				// shared
				std::map <std::string, std::pair<int, unsigned int> >::iterator dic_it = dict_.find(key);
				if (dic_it == dict_.end()) {
					dict_[key] = {index, 1};
					feature.push_back(index);
					index += (key[0] == 'U' ? y_size : y_size * y_size);;
				}
				else {
					dict_[key].second++;
					feature.push_back(dict_[key].first);
				}
			}
			feature_index.push_back(feature);
			feature.clear();
		}
		
		for (size_t i = 0; i < lines.size(); i++) {
			for (std::vector<std::string>::const_iterator it = bigram_templs.begin(); it != bigram_templs.end(); ++it) {
				std::string::const_iterator seq_it = std::find_first_of((*it).begin(), (*it).end(), seq.begin(), seq.end());
				if (seq_it == (*it).end()) continue;
				// key = U05:like
				int x = *(seq_it - 1) - 48;
				int y = *(seq_it + 1) - 48;
				if (*(seq_it - 2) == '-') x = -x;
				std::string key = templsToKey(i, x, y, *it);

				std::map <std::string, std::pair<int, unsigned int> >::iterator dic_it = dict_.find(key);
				if (dic_it == dict_.end()) {
					dict_[key] = { index, 1 };
					feature.push_back(index);
					index += (key[0] == 'U' ? y_size : y_size * y_size);;
				}
				else {
					dict_[key].second++;
					feature.push_back(dict_[key].first);
				}
			}
			feature_index.push_back(feature);
			feature.clear();
		}
		
		total_feature = index;
	}

	std::string templsToKey(int i, int x, int y, std::string templs_string) {
		std::vector<std::string> line;
		if ( i + x < 0) { 
			return "START"; 
		}
		else if (size_t(i + x) >= lines.size()) {
			return "END";
		}
		else {
			return templs_string.substr(0, 4) + split(lines[i + x], y);		// U01:like
		}
	}

	int getFeatureTotal() {
		return total_feature;
	}

	int& getY_size() { return y_size; }
	int& getX_size() { return x_size; }
};

int Sentence::index = 0;
int Sentence::y_size;
int Sentence::total_feature;
std::map <std::string, std::pair<int, unsigned int> > Sentence::dict_;