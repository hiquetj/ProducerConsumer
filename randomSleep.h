#ifndef RANDOMSLEEP_H
#define RANDOMSLEEP_H

#define TEN_TO_THE_NINTH 1000000000

void random_seed(long int seed);
double random_exponential(double mean);
double format_time();
int nsleep(long nseconds);
FILE *CreateLogFile();

#endif
