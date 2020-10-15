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
#include "threadpool.h"

std::mutex labels_mutex;

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

bool open_triandata_task(Sentence* sentence, std::set<std::string>* labels, 
    std::vector<std::string>& unigram_templs, std::vector<std::string>& bigram_templs) {
    std::string tmp;
    for (auto line : sentence->lines) {
        auto index = line.find_last_of(" ");
        tmp = line.substr(index + 1);
        {
            std::lock_guard<std::mutex> guard(labels_mutex);
            labels->insert(tmp);
        }
        sentence->answer_.emplace_back(std::distance(labels->begin(), labels->find(tmp)));
    }
    sentence->buildFeature(unigram_templs, bigram_templs);
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
        cur_sentence->push_back(line);
    }

    // y_size：shared
    
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
    // x:shared    Sentence:thread_local
    std::vector<Sentence*> x;   // x作为自动变量是在栈中，其中的元素Sentence*是在堆中

    // shared
    std::vector<std::string> unigram_templs;
    std::vector<std::string> bigram_templs;

    // shared
    // openTemplate任务量小
    // openTrain需要并行
    if(!openTemplate(template_file, unigram_templs, bigram_templs, templs)) std::cout << template_file << "read error" << std::endl;
    if(!openTrain(train_file, x))  std::cout << template_file << "read error" << std::endl;

    std::set<std::string> labels; // set放所有的标签集合【B， I， O】
    
    std::threadpool tp(8);
    std::vector<std::future<bool>> future_bool;
    for (auto s_ptr : x) {
        // 设置 answer和lines  然后设置feature_index
        future_bool.emplace_back(tp.commit(open_triandata_task, s_ptr, &labels, unigram_templs, bigram_templs));
    }
    
    // 此处需要所有的 x 都执行完 任务才可以 getFeatureTotal， 即线程池中空闲线程数量 == 所有线程数量
    for (int i = 0; i < x.size(); i++) {
        future_bool[i].get();
    }// 主线程会阻塞，直到所有任务执行完毕
    
    // shared
    std::vector<double> weights(Sentence::index);
    std::vector<double> shared_expected(Sentence::index);

    int num_err = 0;
    double obj = 0.0;
    double lr = 0.1;
    bool orthant = false;
    int num_example = 0;
    int i = 0;

    // thread_local
    future_bool.clear();
    for (auto s_ptr : x) {
        // 设置 answer和lines  然后设置feature_index
        future_bool.emplace_back(tp.commit(std::bind(&Sentence::buildGraph, s_ptr)));
    }
    for (int i = 0; i < x.size(); i++) {
        future_bool[i].get();
    }// 主线程会阻塞，直到所有任务执行完毕
    future_bool.clear();

    std::vector<std::future<double>> future_double;
    while (i++ < 10000) {
        for (Sentence* s_ptr : x) {
            // weights：共享只读， expected：不共享，每个线程创造副本
            future_bool.emplace_back(tp.commit(std::bind(&Sentence::dp_and_calcGradient_and_viterbi, 
                s_ptr, weights, shared_expected, std::ref(shared_expected), lr)));
            // int error_num = s_ptr->eval(); 由于异步计算并没有完成，所以这里不能eval()
        }

        // 等带所有异步任务完成进行评估和梯度汇总
        for (int i = 0; i < x.size(); i++) {
            future_bool[i].get();
        }// 主线程会阻塞，直到所有任务执行完毕
        future_bool.clear();

        // 评估
        for (Sentence* s_ptr : x) {
            int error_num = s_ptr->eval();
            num_example += s_ptr->getX_size();
            num_err += error_num;
        }

        // 梯度汇总
        for (size_t i = 0; i < Sentence::index; i++) {
            weights[i] -= lr * shared_expected[i];
            shared_expected[i] = 0;
        }

        double need_del = (double)num_err / num_example;
        std::cout << need_del << std::endl;
    }

    std::cout << "expected" << shared_expected[0] << shared_expected[1000] << std::endl;

    return 0;
}
