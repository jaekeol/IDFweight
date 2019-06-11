# This is a sample script that trains the Gaussian mixture representations for multi-sense embeddings on a small text corpus text8. 
mkdir model600
./multift skipgram -input "data/wiki.600K" -output model600/W600.300D.5EP -lr 1e-5 -dim 300 \
    -ws 10 -epoch 5 -minCount 5 -loss ns -bucket 2000000 \
    -minn 3 -maxn 6 -thread 15 -t 1e-5 -lrUpdateRate 100 -multi 1 -var_scale 2e-4 -margin 1 -idf ngrams.idf_sel
