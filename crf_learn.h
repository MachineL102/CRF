#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include "Sentence.h"
#include "node.h"
#include "path.h"

class Param {
public:
    int freq;
    int maxiter;
    double C;
    double eta;
    bool textmodel;
    unsigned short num_thread;
    std::string algorithm;

public:
    // 直接使用初始化列表的方式效率比 this->C = 10 的赋值操作效率要高
    Param(): freq(1), maxiter(100000), C(10.0), eta(0.000001), textmodel(false), num_thread(8){
        algorithm = "crf-l2";
        //std::cout << "Parm init" << std::endl;

    }
};

bool openTemplate(const char* filename, std::vector<std::string>& unigram_templs_, std::vector<std::string>& bigram_templs_, std::string* templs) {
    std::ifstream ifs(filename);  // 
    if (!ifs) std::cout << filename << "read error" << std::endl;

    std::string line;
    while (std::getline(ifs, line)) {
        if (!line[0] || line[0] == '#') {
            continue;
        }
        if (line[0] == 'U') {
            unigram_templs_.push_back(line);
        }
        else if (line[0] == 'B') {
            bigram_templs_.push_back(line);
        }
        else {
            std::cout << "unknown type: " << line << "of" << filename << std::endl; return false;
        }
    }
    for (size_t i = 0; i < unigram_templs_.size(); ++i) {
        templs->append(unigram_templs_[i]);
        templs->append("\n");
    }
    for (size_t i = 0; i < bigram_templs_.size(); ++i) {
        templs->append(bigram_templs_[i]);
        templs->append("\n");
    }
    return true;
}

bool openTrain(const char* filename, std::vector<Sentence*>& x) {
    std::ifstream ifs(filename);
    std::string line;
    if (!ifs) std::cout << "no such file or directory: " << filename << std::endl;
    std::set<std::string> labels; // set放所有的标签集合【B， I， O】

    // thread_local
    std::vector<std::string> x_;
    std::vector<std::string> y_;
    
    // shared
    int y_size = 0;
    std::string seq = "\t";

    // thread_local
    std::vector<std::string> v;
    Sentence* cur_sentence = new Sentence();  // 指针是必须初始化的，不可以仅声明

    while (std::getline(ifs, line)) {
        // 修改为提前遍历一遍，分给每个线程一个句子
        if (line[0] == '\0' || line[0] == ' ' || line[0] == '\t') {
            if (cur_sentence) x.push_back(cur_sentence);
            cur_sentence = new Sentence();
            continue;
        }
        split(line, v, ' ');
        cur_sentence->push_back(line);
        // labels：shared
        labels.insert(v.back()); 
        int tmp = std::distance(labels.begin(), labels.find(v.back())); //  应该把begin放在前面，++find
        cur_sentence->answer_.push_back(tmp);
        v.clear();
    }
    y_.clear(); 
    for (std::set<std::string>::iterator it = labels.begin();
        it != labels.end(); ++it) {
        y_.push_back(*it);
    }

    // y_size：shared
    cur_sentence -> getY_size() = labels.size();
    ifs.close();
    return true;
}

int crf_learn(int argc, char** argv) {

    Param param;
    const int         freq = param.freq; // 最少特征数
    const int         maxiter = param.maxiter;  // 最大迭代次数
    const double         C = param.C;   // 正则化因子
    const double         eta = param.eta;    // 迭代精度
    const bool           textmodel = param.textmodel;
    const unsigned short thread = param.num_thread;    // 线程数
    std::string salgo = param.algorithm;
    const char* template_file = argv[3];
    const char* train_file = argv[4];


    std::string* templs = new std::string;
    // thread_local
    std::vector<Sentence*> x;   // x作为自动变量是在栈中，其中的元素Sentence*是在堆中

    // shared
    std::vector<std::string> unigram_templs;
    std::vector<std::string> bigram_templs;

    // shared
    // openTemplate任务量小
    // openTrain需要并行
    if(!openTemplate(template_file, unigram_templs, bigram_templs, templs)) std::cout << template_file << "read error" << std::endl;
    if(!openTrain(train_file, x))  std::cout << template_file << "read error" << std::endl;

    // thread_local
    for (size_t cur_sentence = 0; cur_sentence < x.size(); ++cur_sentence) {
        std::vector<int> feature;
        x[cur_sentence]->buildFeature(unigram_templs, bigram_templs);
    }

    int feature_total = Sentence().getFeatureTotal();

    // shared
    std::vector<double> weights(feature_total);
    std::vector<double> expected(feature_total);

    int num_err = 0;
    double obj = 0.0;
    double lr = 1;
    bool orthant = false;
    int num_example = 0;
    int i = 0;

    // thread_local
    for (Sentence* s : x) {
        s->buildGraph();    // 设置每个Node的数据
    }
    while (i++ < 1000) {
        for (Sentence* s : x) {
            // 只读shared
            s->dp(weights);    // 计算M，alpha，beta，Z_全局归一化常数
            // 只读shared
            obj += s->calcGradient(expected, weights);
            s->viterbi();
            int error_num = s->eval();
            num_example += s->getX_size();
            num_err += error_num;
        }
        for (size_t i = 0; i < feature_total; i++) {
            // 可写shared
            weights[i] -= lr * expected[i];
            expected[i] = 0;
        }
        double need_del = (double)num_err / num_example;
        std::cout << need_del << std::endl;
    }

    std::cout << "expected" << expected[0] << expected[1000] << std::endl;

    return 0;
}
