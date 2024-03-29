/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <iomanip>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <map>
#include <cmath>
#include "args.h"
#include "fasttext.h"

using namespace fasttext;

void printUsage() {
  std::cerr
      << "usage: fasttext <command> <args>\n\n"
      << "The commands supported by fasttext are:\n\n"
      << "  supervised              train a supervised classifier\n"
      << "  quantize                quantize a model to reduce the memory usage\n"
      << "  test                    evaluate a supervised classifier\n"
      << "  test-label              print labels with precision and recall scores\n"
      << "  predict                 predict most likely labels\n"
      << "  predict-prob            predict most likely labels with probabilities\n"
      << "  skipgram                train a skipgram model\n"
      << "  cbow                    train a cbow model\n"
      << "  print-word-vectors      print word vectors given a trained model\n"
      << "  print-sentence-vectors  print sentence vectors given a trained model\n"
      << "  print-ngrams            print ngrams given a trained model and word\n"
      << "  nn                      query for nearest neighbors\n"
      << "  analogies               query for analogies\n"
      << "  dump                    dump arguments,dictionary,input/output vectors\n"
      << std::endl;
}

void printQuantizeUsage() {
  std::cerr << "usage: fasttext quantize <args>" << std::endl;
}

void printTestUsage() {
  std::cerr
      << "usage: fasttext test <model> <test-data> [<k>] [<th>]\n\n"
      << "  <model>      model filename\n"
      << "  <test-data>  test data filename (if -, read from stdin)\n"
      << "  <k>          (optional; 1 by default) predict top k labels\n"
      << "  <th>         (optional; 0.0 by default) probability threshold\n"
      << std::endl;
}

void printPredictUsage() {
  std::cerr
      << "usage: fasttext predict[-prob] <model> <test-data> [<k>] [<th>]\n\n"
      << "  <model>      model filename\n"
      << "  <test-data>  test data filename (if -, read from stdin)\n"
      << "  <k>          (optional; 1 by default) predict top k labels\n"
      << "  <th>         (optional; 0.0 by default) probability threshold\n"
      << std::endl;
}

void printTestLabelUsage() {
  std::cerr
      << "usage: fasttext test-label <model> <test-data> [<k>] [<th>]\n\n"
      << "  <model>      model filename\n"
      << "  <test-data>  test data filename\n"
      << "  <k>          (optional; 1 by default) predict top k labels\n"
      << "  <th>         (optional; 0.0 by default) probability threshold\n"
      << std::endl;
}

void printPrintWordVectorsUsage() {
  std::cerr << "usage: fasttext print-word-vectors <model>\n\n"
            << "  <model>      model filename\n"
            << std::endl;
}

void printPrintSentenceVectorsUsage() {
  std::cerr << "usage: fasttext print-sentence-vectors <model>\n\n"
            << "  <model>      model filename\n"
            << std::endl;
}

void printPrintNgramsUsage() {
  std::cerr << "usage: fasttext print-ngrams <model> <word>\n\n"
            << "  <model>      model filename\n"
            << "  <word>       word to print\n"
            << std::endl;
}

void quantize(const std::vector<std::string>& args) {
  Args a = Args();
  if (args.size() < 3) {
    printQuantizeUsage();
    a.printHelp();
    exit(EXIT_FAILURE);
  }
  a.parseArgs(args);
  FastText fasttext;
  // parseArgs checks if a->output is given.
  fasttext.loadModel(a.output + ".bin");
  fasttext.quantize(a);
  fasttext.saveModel(a.output + ".ftz");
  exit(0);
}

void printNNUsage() {
  std::cout << "usage: fasttext nn <model> <k>\n\n"
            << "  <model>      model filename\n"
            << "  <k>          (optional; 10 by default) predict top k labels\n"
            << std::endl;
}

void printAnalogiesUsage() {
  std::cout << "usage: fasttext analogies <model> <k>\n\n"
            << "  <model>      model filename\n"
            << "  <k>          (optional; 10 by default) predict top k labels\n"
            << std::endl;
}

void printDumpUsage() {
  std::cout << "usage: fasttext dump <model> <option>\n\n"
            << "  <model>      model filename\n"
            << "  <option>     option from args,dict,input,output" << std::endl;
}

void test(const std::vector<std::string>& args) {
  bool perLabel = args[1] == "test-label";

  if (args.size() < 4 || args.size() > 6) {
    perLabel ? printTestLabelUsage() : printTestUsage();
    exit(EXIT_FAILURE);
  }

  const auto& model = args[2];
  const auto& input = args[3];
  int32_t k = args.size() > 4 ? std::stoi(args[4]) : 1;
  real threshold = args.size() > 5 ? std::stof(args[5]) : 0.0;

  FastText fasttext;
  fasttext.loadModel(model);

  Meter meter;

  if (input == "-") {
    fasttext.test(std::cin, k, threshold, meter);
  } else {
    std::ifstream ifs(input);
    if (!ifs.is_open()) {
      std::cerr << "Test file cannot be opened!" << std::endl;
      exit(EXIT_FAILURE);
    }
    fasttext.test(ifs, k, threshold, meter);
  }

  if (perLabel) {
    std::cout << std::fixed << std::setprecision(6);
    auto writeMetric = [](const std::string& name, double value) {
      std::cout << name << " : ";
      if (std::isfinite(value)) {
        std::cout << value;
      } else {
        std::cout << "--------";
      }
      std::cout << "  ";
    };

    std::shared_ptr<const Dictionary> dict = fasttext.getDictionary();
    for (int32_t labelId = 0; labelId < dict->nlabels(); labelId++) {
      writeMetric("F1-Score", meter.f1Score(labelId));
      writeMetric("Precision", meter.precision(labelId));
      writeMetric("Recall", meter.recall(labelId));
      std::cout << " " << dict->getLabel(labelId) << std::endl;
    }
  }
  meter.writeGeneralMetrics(std::cout, k);

  exit(0);
}

void printPredictions(
    const std::vector<std::pair<real, std::string>>& predictions,
    bool printProb,
    bool multiline) {
  bool first = true;
  for (const auto& prediction : predictions) {
    if (!first && !multiline) {
      std::cout << " ";
    }
    first = false;
    std::cout << prediction.second;
    if (printProb) {
      std::cout << " " << prediction.first;
    }
    if (multiline) {
      std::cout << std::endl;
    }
  }
  if (!multiline) {
    std::cout << std::endl;
  }
}

void predict(const std::vector<std::string>& args) {
  if (args.size() < 4 || args.size() > 6) {
    printPredictUsage();
    exit(EXIT_FAILURE);
  }
  int32_t k = 1;
  real threshold = 0.0;
  if (args.size() > 4) {
    k = std::stoi(args[4]);
    if (args.size() == 6) {
      threshold = std::stof(args[5]);
    }
  }

  bool printProb = args[1] == "predict-prob";
  FastText fasttext;
  fasttext.loadModel(std::string(args[2]));

  std::ifstream ifs;
  std::string infile(args[3]);
  bool inputIsStdIn = infile == "-";
  if (!inputIsStdIn) {
    ifs.open(infile);
    if (!inputIsStdIn && !ifs.is_open()) {
      std::cerr << "Input file cannot be opened!" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  std::istream& in = inputIsStdIn ? std::cin : ifs;
  std::vector<std::pair<real, std::string>> predictions;
  while (fasttext.predictLine(in, predictions, k, threshold)) {
    printPredictions(predictions, printProb, false);
  }
  if (ifs.is_open()) {
    ifs.close();
  }

  exit(0);
}

void printWordVectors(const std::vector<std::string> args) {
  if (args.size() != 4) {
    printPrintWordVectorsUsage();
    exit(EXIT_FAILURE);
  }
  FastText fasttext;
  fasttext.loadModel(std::string(args[2]));
  fasttext.readIdf(std::string(args[3]));
  std::string word;
  Vector vec(fasttext.getDimension());
  while (std::cin >> word) {
    //fasttext.getWordVector(vec, word);
    fasttext.getWordVector_w(vec, word);
    std::cout << word << " " << vec << std::endl;
  }
  exit(0);
}

void printSentenceVectors(const std::vector<std::string> args) {
  if (args.size() != 3) {
    printPrintSentenceVectorsUsage();
    exit(EXIT_FAILURE);
  }
  FastText fasttext;
  fasttext.loadModel(std::string(args[2]));
  Vector svec(fasttext.getDimension());
  while (std::cin.peek() != EOF) {
    fasttext.getSentenceVector(std::cin, svec);
    // Don't print sentence
    std::cout << svec << std::endl;
  }
  exit(0);
}

void printNgrams(const std::vector<std::string> args) {
  if (args.size() != 4) {
    printPrintNgramsUsage();
    exit(EXIT_FAILURE);
  }
  FastText fasttext;
  fasttext.loadModel(std::string(args[2]));

  std::string word(args[3]);
  std::vector<std::pair<std::string, Vector>> ngramVectors =
      fasttext.getNgramVectors(word);

  for (const auto& ngramVector : ngramVectors) {
    std::cout << ngramVector.first << " " << ngramVector.second << std::endl;
  }

  exit(0);
}

void printAllNgrams(const std::vector<std::string> args) {
	if (args.size() != 3) {
		std::cerr << "usage : cat queryfile | ./fasttext print-all-ngrams binaryfile" << std::endl;
		exit(EXIT_FAILURE);	
	}
	FastText fasttext;
	fasttext.loadModel(std::string(args[2]));

  	std::string word;
  	Vector vec(fasttext.getDimension());
  	while (std::cin >> word) {
  		std::vector<std::pair<std::string, Vector>> ngramVectors =
      				fasttext.getNgramVectors(word);

		std::cout << "## " << word << std::endl;
  		for (const auto& ngramVector : ngramVectors) {
			//if(re.find(ngramVector.first) == re.end()) continue;
    		std::cout << ngramVector.first << " " << ngramVector.second << std::endl;
  		}
  	}
  	exit(0);
}

void printIDFofNgrams(const std::vector<std::string> args) {
	std::shared_ptr<Dictionary> d_;
	std::string line;

	if ( args.size() != 3) {
		std::cerr << "./fasttext print-idf-of-ngrams inputfile" << std::endl;
		exit(EXIT_FAILURE);	
	}

	int eos=0;
	std::ifstream ifs(args[2]);
	std::string word;
	std::string ngram;
	std::vector<int32_t> ngrams;
	std::vector<std::string> substrings;

	std::map<std::string, int>  nghash;	// all
	std::map<std::string, int>  dhash;	// 1 document
	while(d_->readWord(ifs, word)){
		if(word == Dictionary::EOS){
			for(std::map<std::string, int>::iterator iter = dhash.begin(); iter != dhash.end(); iter++){
				nghash[iter->first] += 1;
			}
			dhash.clear();
			eos++;
			if(eos%100==0){
				std::cerr << eos << "\r";
			}	
			continue;
		}else{
			ngrams.clear();
			substrings.clear();
			d_->computeSubwords_only(Dictionary::BOW + word + Dictionary::EOW, ngrams, &substrings, 6, 3);
			substrings.push_back(word);
		}
		for(size_t i = 0; i < substrings.size(); i++){
			ngram = substrings[i];	
			//if(dhash.find(ngram) == dhash.end()){
			dhash[ngram]++;
			//std::cout << "\t" << substrings[i] << std::endl;
		}
	}
	double idf;
	float icnt;
	std::cout << eos << std::endl;
	for(std::map<std::string, int>::iterator iter = nghash.begin(); iter != nghash.end(); iter++){
		//idf = log((float)(eos+1) / ((float)iter->second + 1) );
		icnt = (float)iter->second+1;
		idf = log( ((float)eos-icnt)  / icnt );
		std::cout << iter->first << "\t" << iter->second << "\t" << idf << std::endl;	
	}
  	exit(0);
}

void nn(const std::vector<std::string> args) {
  int32_t k;
  if (args.size() == 3) {
    k = 10;
  } else if (args.size() == 4) {
    k = std::stoi(args[3]);
  } else {
    printNNUsage();
    exit(EXIT_FAILURE);
  }
  FastText fasttext;
  fasttext.loadModel(std::string(args[2]));
  std::string prompt("Query word? ");
  std::cout << prompt;

  std::string queryWord;
  while (std::cin >> queryWord) {
    printPredictions(fasttext.getNN(queryWord, k), true, true);
    std::cout << prompt;
  }
  exit(0);
}

void analogies(const std::vector<std::string> args) {
  int32_t k;
  if (args.size() == 4) {
    k = -1;
  } else if (args.size() == 5) {
    k = std::stoi(args[4]);
  } else {
    printAnalogiesUsage();
    exit(EXIT_FAILURE);
  }
  if (k <= 0) {
  	std::cerr << "Use all dictionary vcabulary" << std::endl;
    //throw std::invalid_argument("k needs to be 1 or higher!");
  }else{
  	std::cerr << "Use top-k dictionary vcabulary : " << k << std::endl;
  }
  FastText fasttext;
  std::string model(args[2]);
  std::cout << "Loading model " << model << std::endl;
  fasttext.loadModel(model);
  fasttext.readIdf(std::string(args[3]));

  std::string prompt("Query triplet (A - B + C)? ");
  std::string wordA, wordB, wordC;
  int32_t idA, idB, idC;
  //std::cout << prompt;
  while (true) {
    std::cin >> wordA;
    std::cin >> wordB;
    std::cin >> wordC;
    if(wordA == "stopstopstop") break;
    std::cout << wordA << " " << wordB << " " << wordC << " ";

	idA = fasttext.getWordId(wordA);
	idB = fasttext.getWordId(wordB);
	idC = fasttext.getWordId(wordC);

	if(idA > k || idB > k || idC > k){
		std::cout << "OutOfVocab" << " " << -1 << std::endl;
	}else{
    	printPredictions(fasttext.getAnalogies(k, wordA, wordB, wordC), true, true);
	}

    //std::cout << prompt;
  }
  exit(0);
}

void train(const std::vector<std::string> args) {
  Args a = Args();
  a.parseArgs(args);
  FastText fasttext;
  std::string outputFileName(a.output + ".bin");
  std::ofstream ofs(outputFileName);
  if (!ofs.is_open()) {
    throw std::invalid_argument(
        outputFileName + " cannot be opened for saving.");
  }
  ofs.close();
  fasttext.train(a);
  fasttext.saveModel(outputFileName);
  fasttext.saveVectors(a.output + ".vec");
  if (a.saveOutput) {
    fasttext.saveOutput(a.output + ".output");
  }
}

void dump(const std::vector<std::string>& args) {
  if (args.size() < 4) {
    printDumpUsage();
    exit(EXIT_FAILURE);
  }

  std::string modelPath = args[2];
  std::string option = args[3];

  FastText fasttext;
  fasttext.loadModel(modelPath);
  if (option == "args") {
    fasttext.getArgs().dump(std::cout);
  } else if (option == "dict") {
    fasttext.getDictionary()->dump(std::cout);
  } else if (option == "input") {
    if (fasttext.isQuant()) {
      std::cerr << "Not supported for quantized models." << std::endl;
    } else {
      fasttext.getInputMatrix()->dump(std::cout);
    }
  } else if (option == "output") {
    if (fasttext.isQuant()) {
      std::cerr << "Not supported for quantized models." << std::endl;
    } else {
      fasttext.getOutputMatrix()->dump(std::cout);
    }
  } else {
    printDumpUsage();
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char** argv) {
  std::vector<std::string> args(argv, argv + argc);
  if (args.size() < 2) {
    printUsage();
    exit(EXIT_FAILURE);
  }
  std::string command(args[1]);
  if (command == "skipgram" || command == "cbow" || command == "supervised") {
    train(args);
  } else if (command == "test" || command == "test-label") {
    test(args);
  } else if (command == "quantize") {
    quantize(args);
  } else if (command == "print-word-vectors") {
    printWordVectors(args);
  } else if (command == "print-sentence-vectors") {
    printSentenceVectors(args);
  } else if (command == "print-ngrams") {
    printNgrams(args);
  } else if (command == "print-all-ngrams") {
	printAllNgrams(args);
  } else if (command == "print-idf-of-ngrams"){
  	printIDFofNgrams(args);
  } else if (command == "nn") {
    nn(args);
  } else if (command == "analogies") {
    analogies(args);
  } else if (command == "predict" || command == "predict-prob") {
    predict(args);
  } else if (command == "dump") {
    dump(args);
  } else {
    printUsage();
    exit(EXIT_FAILURE);
  }
  return 0;
}
