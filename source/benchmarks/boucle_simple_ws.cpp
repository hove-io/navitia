#include "threadpool.h"
#include "data_structures.h"

#define LENGTH 170000
#define BUFFER_LENGTH 30000
#define RESULT_LENGTH 10000

using namespace webservice;
struct Data {
  int nb_threads;
  int * data;//[LENGTH];
  Data() : nb_threads(8) {
      data = (int*) calloc(LENGTH, sizeof(int));
    for(int i = 0; i< LENGTH; i++){
      data[i] = i;
    }
  }
};

struct Worker {
  int *buffer;
  char *p_result;
  Worker() {
    buffer = (int*) calloc(BUFFER_LENGTH, sizeof(int));
    p_result = (char*) calloc(RESULT_LENGTH, sizeof(char));
  }
  
  ResponseData operator()(const RequestData & data, Data & d){
    char *current = NULL;
    long unsigned int index;
    long unsigned int counter = 0;
    int value = (rand() % 2) + 1 ;
    for(index=0; index<LENGTH; index++){
        if(d.data[index] % value == 0){
          buffer[counter % BUFFER_LENGTH] = d.data[index];
          counter++;
        }
    }
    current = p_result;
    
    for(index=0; (index<counter && index<RESULT_LENGTH); index++){
        *current = 32 + (buffer[index]%223);
        current++;
        //la plage de caractére imprimable ascii s'étend entre 32 et 255
    }
    ResponseData rd;
    p_result[RESULT_LENGTH -1] = '\0';
    rd.response = p_result + (RESULT_LENGTH*3/4); // On retourne juste le quart pour ne pas être limité par le réseau
    rd.content_type = "text/html";
    rd.status_code = 200;
    rd.charset = "latin1";
   return rd;
  }
};

MAKE_WEBSERVICE(Data, Worker)
