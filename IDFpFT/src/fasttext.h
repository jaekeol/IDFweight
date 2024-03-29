/**
 * Copyright (c) 2016-present, Facebook, Inc.
 *               2018-present, Ben Athiwaratkun
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef FASTTEXT_FASTTEXT_H
#define FASTTEXT_FASTTEXT_H

#define FASTTEXT_VERSION 11 /* Version 1a */
#define FASTTEXT_FILEFORMAT_MAGIC_INT32 793712314
#define CHARONLY 0
#define WORDONLY 1
#define COMBINE 2

#include <time.h>

#include <atomic>
#include <memory>
#include <set>
#include <map>

#include "args.h"
#include "dictionary.h"
#include "matrix.h"
#include "qmatrix.h"
#include "model.h"
#include "real.h"
#include "utils.h"
#include "vector.h"
//#include "cnpy.h"

namespace fasttext {

class FastText {
  private:
    std::shared_ptr<Args> args_;
    std::shared_ptr<Dictionary> dict_;

    std::shared_ptr<Matrix> input_;
    std::shared_ptr<Matrix> output_;

    // BenA: for multi-prototype
    std::shared_ptr<Matrix> input2_;
    std::shared_ptr<Matrix> output2_;

    std::shared_ptr<Matrix> inputvar_;
    std::shared_ptr<Matrix> outputvar_;
    std::shared_ptr<Matrix> input2var_;
    std::shared_ptr<Matrix> output2var_;


    std::shared_ptr<QMatrix> qinput_;
    std::shared_ptr<QMatrix> qoutput_;

    std::shared_ptr<Model> model_;

    std::atomic<int64_t> tokenCount;
    clock_t start;
    void signModel(std::ostream&);
    bool checkModel(std::istream&);

    bool quant_;

    std::map<int32_t, float> idf_;	// idf memory

  public:
    FastText();

    void getVector(Vector&, const std::string&);
    void getVector_w(Vector&, const std::string&);	// using idf value
    void getVector_w2(Vector&, const std::string&);	// using idf value
    void getVector(Vector&, const std::string&, int, float);
    void saveVectors();
    void saveOutput();
    void saveModel();
    void loadModel(std::istream&);
    void loadModel(const std::string&);
    void loadModel(const std::string&, bool);
    void printInfo(real, real);

    void supervised(Model&, real, const std::vector<int32_t>&,
                    const std::vector<int32_t>&);
    void cbow(Model&, real, const std::vector<int32_t>&);
    void skipgram(Model&, real, const std::vector<int32_t>&);
    std::vector<int32_t> selectEmbeddings(int32_t) const;
    void quantize(std::shared_ptr<Args>);
    void test(std::istream&, int32_t);
    void predict(std::istream&, int32_t, bool);
    void predict(
        std::istream&,
        int32_t,
        std::vector<std::pair<real, std::string>>&) const;
    void wordVectors();
    void wordVectors(int, float);
    void sentenceVectors();
    void ngramVectors(std::string);
    void textVectors();
    void printWordVectors();
    void printWordVectors(int, float);
    void printSentenceVectors();
    void precomputeWordVectors(Matrix&);
    void findNN(const Matrix&, const Vector&, int32_t,
                const std::set<std::string>&);
    void nn(int32_t);
    void analogies(int32_t);
    void trainThread(int32_t);
    void train(std::shared_ptr<Args>);

	void readIdf(const std::string& filename);
    void getWeight(const std::vector<int32_t>& ngrams, std::vector<float>& weights);

    void loadVectors(std::string);
    void saveNgramVectors(std::string);
};

}
#endif
