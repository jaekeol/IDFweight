/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "model.h"
#include "utils.h"

#include <assert.h>
#include <algorithm>
#include <stdexcept>

namespace fasttext {

constexpr int64_t SIGMOID_TABLE_SIZE = 512;
constexpr int64_t MAX_SIGMOID = 8;
constexpr int64_t LOG_TABLE_SIZE = 512;

Model::Model(
    std::shared_ptr<Matrix> wi,
    std::shared_ptr<Matrix> wo,
    std::shared_ptr<Args> args,
    int32_t seed)
    : hidden_(args->dim),
      output_(wo->size(0)),
      grad_(args->dim),
      rng(seed),
      quant_(false) {
  wi_ = wi;
  wo_ = wo;
  args_ = args;
  osz_ = wo->size(0);
  hsz_ = args->dim;
  negpos = 0;
  loss_ = 0.0;
  nexamples_ = 1;
  t_sigmoid_.reserve(SIGMOID_TABLE_SIZE + 1);
  t_log_.reserve(LOG_TABLE_SIZE + 1);
  initSigmoid();
  initLog();
}

void Model::setQuantizePointer(
    std::shared_ptr<QMatrix> qwi,
    std::shared_ptr<QMatrix> qwo,
    bool qout) {
  qwi_ = qwi;
  qwo_ = qwo;
  if (qout) {
    osz_ = qwo_->getM();
  }
}

real Model::binaryLogistic(int32_t target, bool label, real lr) {
  real score = sigmoid(wo_->dotRow(hidden_, target));
  real alpha = lr * (real(label) - score);
  grad_.addRow(*wo_, target, alpha);
  wo_->addRow(hidden_, target, alpha);
  if (label) {
    return -log(score);
  } else {
    return -log(1.0 - score);
  }
}

real Model::negativeSampling(int32_t target, real lr) {
  real loss = 0.0;
  grad_.zero();
  for (int32_t n = 0; n <= args_->neg; n++) {
    if (n == 0) {
      loss += binaryLogistic(target, true, lr);
    } else {
      loss += binaryLogistic(getNegative(target), false, lr);
    }
  }
  return loss;
}

real Model::hierarchicalSoftmax(int32_t target, real lr) {
  real loss = 0.0;
  grad_.zero();
  const std::vector<bool>& binaryCode = codes[target];
  const std::vector<int32_t>& pathToRoot = paths[target];
  for (int32_t i = 0; i < pathToRoot.size(); i++) {
    loss += binaryLogistic(pathToRoot[i], binaryCode[i], lr);
  }
  return loss;
}

void Model::computeOutput(Vector& hidden, Vector& output) const {
  if (quant_ && args_->qout) {
    output.mul(*qwo_, hidden);
  } else {
    output.mul(*wo_, hidden);
  }
}

void Model::computeOutputSigmoid(Vector& hidden, Vector& output) const {
  computeOutput(hidden, output);
  for (int32_t i = 0; i < osz_; i++) {
    output[i] = sigmoid(output[i]);
  }
}

void Model::computeOutputSoftmax(Vector& hidden, Vector& output) const {
  computeOutput(hidden, output);
  real max = output[0], z = 0.0;
  for (int32_t i = 0; i < osz_; i++) {
    max = std::max(output[i], max);
  }
  for (int32_t i = 0; i < osz_; i++) {
    output[i] = exp(output[i] - max);
    z += output[i];
  }
  for (int32_t i = 0; i < osz_; i++) {
    output[i] /= z;
  }
}

void Model::computeOutputSoftmax() {
  computeOutputSoftmax(hidden_, output_);
}

real Model::softmax(int32_t target, real lr) {
  grad_.zero();
  computeOutputSoftmax();
  for (int32_t i = 0; i < osz_; i++) {
    real label = (i == target) ? 1.0 : 0.0;
    real alpha = lr * (label - output_[i]);
    grad_.addRow(*wo_, i, alpha);
    wo_->addRow(hidden_, i, alpha);
  }
  return -log(output_[target]);
}

real Model::oneVsAll(const std::vector<int32_t>& targets, real lr) {
  real loss = 0.0;
  for (int32_t i = 0; i < osz_; i++) {
    bool isMatch = utils::contains(targets, i);
    loss += binaryLogistic(i, isMatch, lr);
  }

  return loss;
}

void Model::computeHidden(const std::vector<int32_t>& input, Vector& hidden)
    const {
  assert(hidden.size() == hsz_);
  hidden.zero();	// hidden 초기화
  for (auto it = input.cbegin(); it != input.cend(); ++it) {
    if (quant_) {
      hidden.addRow(*qwi_, *it);
    } else {
      hidden.addRow(*wi_, *it);	// ngram을 찾아서 더한다.
    }
  }
  hidden.mul(1.0 / input.size());	// normalize
}

void Model::computeHidden(const std::vector<int32_t>& input, const std::vector<float>& weights, Vector& hidden)
    const {
  assert(hidden.size() == hsz_);
  hidden.zero();	// hidden 초기화
  //for (auto it = input.cbegin(); it != input.cend(); ++it) {
  float weight_sum = 0;
  for (int i = 0; i<input.size(); i++){
    if (quant_) {
      hidden.addRow(*qwi_, input[i]);
    } else {
      hidden.addRow(*wi_, input[i], weights[i]);	// ngram을 찾아서 더한다.
    }
	weight_sum += weights[i];
  }
  //hidden.mul(1.0 / input.size());	// normalize
  if(weight_sum == 0) weight_sum = 1;
  hidden.mul(1.0 / weight_sum );
}



bool Model::comparePairs(
    const std::pair<real, int32_t>& l,
    const std::pair<real, int32_t>& r) {
  return l.first > r.first;
}

void Model::predict(
    const std::vector<int32_t>& input,
    int32_t k,
    real threshold,
    std::vector<std::pair<real, int32_t>>& heap,
    Vector& hidden,
    Vector& output) const {
  if (k == Model::kUnlimitedPredictions) {
    k = osz_;
  } else if (k <= 0) {
    throw std::invalid_argument("k needs to be 1 or higher!");
  }
  if (args_->model != model_name::sup) {
    throw std::invalid_argument("Model needs to be supervised for prediction!");
  }
  heap.reserve(k + 1);
  computeHidden(input, hidden);
  if (args_->loss == loss_name::hs) {
    dfs(k, threshold, 2 * osz_ - 2, 0.0, heap, hidden);
  } else {
    findKBest(k, threshold, heap, hidden, output);
  }
  std::sort_heap(heap.begin(), heap.end(), comparePairs);
}

void Model::predict(
    const std::vector<int32_t>& input,
    int32_t k,
    real threshold,
    std::vector<std::pair<real, int32_t>>& heap) {
  predict(input, k, threshold, heap, hidden_, output_);
}

void Model::findKBest(
    int32_t k,
    real threshold,
    std::vector<std::pair<real, int32_t>>& heap,
    Vector& hidden,
    Vector& output) const {
  if (args_->loss == loss_name::ova) {
    computeOutputSigmoid(hidden, output);
  } else {
    computeOutputSoftmax(hidden, output);
  }
  for (int32_t i = 0; i < osz_; i++) {
    if (output[i] < threshold) {
      continue;
    }
    if (heap.size() == k && std_log(output[i]) < heap.front().first) {
      continue;
    }
    heap.push_back(std::make_pair(std_log(output[i]), i));
    std::push_heap(heap.begin(), heap.end(), comparePairs);
    if (heap.size() > k) {
      std::pop_heap(heap.begin(), heap.end(), comparePairs);
      heap.pop_back();
    }
  }
}

void Model::dfs(
    int32_t k,
    real threshold,
    int32_t node,
    real score,
    std::vector<std::pair<real, int32_t>>& heap,
    Vector& hidden) const {
  if (score < std_log(threshold)) {
    return;
  }
  if (heap.size() == k && score < heap.front().first) {
    return;
  }

  if (tree[node].left == -1 && tree[node].right == -1) {
    heap.push_back(std::make_pair(score, node));
    std::push_heap(heap.begin(), heap.end(), comparePairs);
    if (heap.size() > k) {
      std::pop_heap(heap.begin(), heap.end(), comparePairs);
      heap.pop_back();
    }
    return;
  }

  real f;
  if (quant_ && args_->qout) {
    f = qwo_->dotRow(hidden, node - osz_);
  } else {
    f = wo_->dotRow(hidden, node - osz_);
  }
  f = 1. / (1 + std::exp(-f));

  dfs(k, threshold, tree[node].left, score + std_log(1.0 - f), heap, hidden);
  dfs(k, threshold, tree[node].right, score + std_log(f), heap, hidden);
}

real Model::computeLoss(
    const std::vector<int32_t>& targets,
    int32_t targetIndex,
    real lr) {
  real loss = 0.0;

  if (args_->loss == loss_name::ns) {
    loss = negativeSampling(targets[targetIndex], lr);
  } else if (args_->loss == loss_name::hs) {
    loss = hierarchicalSoftmax(targets[targetIndex], lr);
  } else if (args_->loss == loss_name::softmax) {
    loss = softmax(targets[targetIndex], lr);
  } else if (args_->loss == loss_name::ova) {
    loss = oneVsAll(targets, lr);
  } else {
    throw std::invalid_argument("Unhandled loss function for this model.");
  }

  return loss;
}

void Model::update(
    const std::vector<int32_t>& input,
    const std::vector<int32_t>& targets,
    int32_t targetIndex,
    real lr) {
  if (input.size() == 0) {
    return;
  }
  computeHidden(input, hidden_);	// hidden에다가 ngaram vector를 죽더한다.

  if (targetIndex == kAllLabelsAsTarget) {
    loss_ += computeLoss(targets, -1, lr);
  } else {
    assert(targetIndex >= 0);
    assert(targetIndex < osz_);
    loss_ += computeLoss(targets, targetIndex, lr);	//loss계산하면서 grad.. 
  }

  nexamples_ += 1;

  if (args_->model == model_name::sup) {
    grad_.mul(1.0 / input.size());
  }
  for (auto it = input.cbegin(); it != input.cend(); ++it) {
    wi_->addRow(grad_, *it, 1.0);
  }
}

void Model::update(
    const std::vector<int32_t>& input,
	const std::vector<float>& weights,
    const std::vector<int32_t>& targets,
    int32_t targetIndex,
    real lr,
	std::map<int32_t, float>& idf) {
  if (input.size() == 0) {
    return;
  }
  computeHidden(input, weights, hidden_);	// hidden에다가 ngaram vector를 죽더한다.

  if (targetIndex == kAllLabelsAsTarget) {
    loss_ += computeLoss(targets, -1, lr);
  } else {
    assert(targetIndex >= 0);
    assert(targetIndex < osz_);
    loss_ += computeLoss(targets, targetIndex, lr);	//loss계산하면서 grad.. 
  }

  nexamples_ += 1;

  if (args_->model == model_name::sup) {
    grad_.mul(1.0 / input.size());
  }

  real grad_scalar=0;
  int32_t ngram;
  for (int i=0; i<input.size(); i++){
//	wi_->addRow(grad_, input[i], weights[i]);	// weight에 따라 update한다.
	grad_scalar = wi_->addRow_r(grad_, input[i], weights[i]);
	grad_scalar = grad_scalar / (float)args_->dim;
	ngram = input[i];
	if(idf.find(ngram) != idf.end()){
		if(idf[ngram] < 1 && idf[ngram] >= 0){
			idf[ngram] = idf[ngram] + grad_scalar;	 // idf를 update 한다.
		}
	}
  }
}


void Model::setTargetCounts(const std::vector<int64_t>& counts) {
  assert(counts.size() == osz_);
  if (args_->loss == loss_name::ns) {
    initTableNegatives(counts);
  }
  if (args_->loss == loss_name::hs) {
    buildTree(counts);
  }
}

void Model::initTableNegatives(const std::vector<int64_t>& counts) {
  real z = 0.0;
  for (size_t i = 0; i < counts.size(); i++) {
    z += pow(counts[i], 0.5);
  }
  for (size_t i = 0; i < counts.size(); i++) {
    real c = pow(counts[i], 0.5);
    for (size_t j = 0; j < c * NEGATIVE_TABLE_SIZE / z; j++) {
      negatives_.push_back(i);
    }
  }
  std::shuffle(negatives_.begin(), negatives_.end(), rng);
}

int32_t Model::getNegative(int32_t target) {
  int32_t negative;
  do {
    negative = negatives_[negpos];
    negpos = (negpos + 1) % negatives_.size();
  } while (target == negative);
  return negative;
}

void Model::buildTree(const std::vector<int64_t>& counts) {
  tree.resize(2 * osz_ - 1);
  for (int32_t i = 0; i < 2 * osz_ - 1; i++) {
    tree[i].parent = -1;
    tree[i].left = -1;
    tree[i].right = -1;
    tree[i].count = 1e15;
    tree[i].binary = false;
  }
  for (int32_t i = 0; i < osz_; i++) {
    tree[i].count = counts[i];
  }
  int32_t leaf = osz_ - 1;
  int32_t node = osz_;
  for (int32_t i = osz_; i < 2 * osz_ - 1; i++) {
    int32_t mini[2];
    for (int32_t j = 0; j < 2; j++) {
      if (leaf >= 0 && tree[leaf].count < tree[node].count) {
        mini[j] = leaf--;
      } else {
        mini[j] = node++;
      }
    }
    tree[i].left = mini[0];
    tree[i].right = mini[1];
    tree[i].count = tree[mini[0]].count + tree[mini[1]].count;
    tree[mini[0]].parent = i;
    tree[mini[1]].parent = i;
    tree[mini[1]].binary = true;
  }
  for (int32_t i = 0; i < osz_; i++) {
    std::vector<int32_t> path;
    std::vector<bool> code;
    int32_t j = i;
    while (tree[j].parent != -1) {
      path.push_back(tree[j].parent - osz_);
      code.push_back(tree[j].binary);
      j = tree[j].parent;
    }
    paths.push_back(path);
    codes.push_back(code);
  }
}

real Model::getLoss() const {
  return loss_ / nexamples_;
}

void Model::initSigmoid() {
  for (int i = 0; i < SIGMOID_TABLE_SIZE + 1; i++) {
    real x = real(i * 2 * MAX_SIGMOID) / SIGMOID_TABLE_SIZE - MAX_SIGMOID;
    t_sigmoid_.push_back(1.0 / (1.0 + std::exp(-x)));
  }
}

void Model::initLog() {
  for (int i = 0; i < LOG_TABLE_SIZE + 1; i++) {
    real x = (real(i) + 1e-5) / LOG_TABLE_SIZE;
    t_log_.push_back(std::log(x));
  }
}

real Model::log(real x) const {
  if (x > 1.0) {
    return 0.0;
  }
  int64_t i = int64_t(x * LOG_TABLE_SIZE);
  return t_log_[i];
}

real Model::std_log(real x) const {
  return std::log(x + 1e-5);
}

real Model::sigmoid(real x) const {
  if (x < -MAX_SIGMOID) {
    return 0.0;
  } else if (x > MAX_SIGMOID) {
    return 1.0;
  } else {
    int64_t i =
        int64_t((x + MAX_SIGMOID) * SIGMOID_TABLE_SIZE / MAX_SIGMOID / 2);
    return t_sigmoid_[i];
  }
}

} // namespace fasttext
