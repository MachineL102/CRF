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
    // ֱ��ʹ�ó�ʼ���б�ķ�ʽЧ�ʱ� this->C = 10 �ĸ�ֵ����Ч��Ҫ��
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
    std::set<std::string> labels; // set�����еı�ǩ���ϡ�B�� I�� O��

    // thread_local
    std::vector<std::string> x_;
    std::vector<std::string> y_;
    
    // shared
    int y_size = 0;

    // thread_local
    std::vector<std::string> v;
    Sentence* cur_sentence = new Sentence();  // ָ���Ǳ����ʼ���ģ������Խ�����

    while (std::getline(ifs, line)) {
        // �޸�Ϊ��ǰ����һ�飬�ָ�ÿ���߳�һ������
        if (line[0] == '\0' || line[0] == ' ' || line[0] == '\t') {
            if (cur_sentence) x.push_back(cur_sentence);
            cur_sentence = new Sentence();
            continue;
        }
        cur_sentence->push_back(line);
    }

    // y_size��shared
    
    ifs.close();
    return true;
}


int crf_learn(int argc, char** argv) {

    Param param;
    const int         freq = param.freq; // ����������
    const int         maxiter = param.maxiter;  // ����������
    const double         C = param.C;   // ��������
    const double         eta = param.eta;    // ��������
    const bool           textmodel = param.textmodel;
    const unsigned short thread = param.num_thread;    // �߳���
    std::string salgo = param.algorithm;
    const char* template_file = argv[3];
    const char* train_file = argv[4];


    std::string* templs = new std::string;
    // x:shared    Sentence:thread_local
    std::vector<Sentence*> x;   // x��Ϊ�Զ���������ջ�У����е�Ԫ��Sentence*���ڶ���

    // shared
    std::vector<std::string> unigram_templs;
    std::vector<std::string> bigram_templs;

    // shared
    // openTemplate������С
    // openTrain��Ҫ����
    if(!openTemplate(template_file, unigram_templs, bigram_templs, templs)) std::cout << template_file << "read error" << std::endl;
    if(!openTrain(train_file, x))  std::cout << template_file << "read error" << std::endl;

    std::set<std::string> labels; // set�����еı�ǩ���ϡ�B�� I�� O��
    
    std::threadpool tp(8);
    std::vector<std::future<bool>> future_bool;
    for (auto s_ptr : x) {
        // ���� answer��lines  Ȼ������feature_index
        future_bool.emplace_back(tp.commit(open_triandata_task, s_ptr, &labels, unigram_templs, bigram_templs));
    }
    
    // �˴���Ҫ���е� x ��ִ���� ����ſ��� getFeatureTotal�� ���̳߳��п����߳����� == �����߳�����
    for (int i = 0; i < x.size(); i++) {
        future_bool[i].get();
    }// ���̻߳�������ֱ����������ִ�����
    
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
        // ���� answer��lines  Ȼ������feature_index
        future_bool.emplace_back(tp.commit(std::bind(&Sentence::buildGraph, s_ptr)));
    }
    for (int i = 0; i < x.size(); i++) {
        future_bool[i].get();
    }// ���̻߳�������ֱ����������ִ�����
    future_bool.clear();

    std::vector<std::future<double>> future_double;
    while (i++ < 10000) {
        for (Sentence* s_ptr : x) {
            // weights������ֻ���� expected��������ÿ���̴߳��츱��
            future_bool.emplace_back(tp.commit(std::bind(&Sentence::dp_and_calcGradient_and_viterbi, 
                s_ptr, weights, shared_expected, std::ref(shared_expected), lr)));
            // int error_num = s_ptr->eval(); �����첽���㲢û����ɣ��������ﲻ��eval()
        }

        // �ȴ������첽������ɽ����������ݶȻ���
        for (int i = 0; i < x.size(); i++) {
            future_bool[i].get();
        }// ���̻߳�������ֱ����������ִ�����
        future_bool.clear();

        // ����
        for (Sentence* s_ptr : x) {
            int error_num = s_ptr->eval();
            num_example += s_ptr->getX_size();
            num_err += error_num;
        }

        // �ݶȻ���
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
