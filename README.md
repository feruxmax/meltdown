# meltdown
Meltdown/Spectre experiments

./meltdown BRANCH_PREDICTOR_TRAIN_COUN SECRET
 

Reproduced on i7 3770, ubuntu 16.04, gcc 5.4 with -O0 option. Reproducing is unstable (6 runs from 10 for BRANCH_PREDICTOR_TRAIN_COUN=2).
