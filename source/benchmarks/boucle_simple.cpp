// dllmain.cpp : Définit le point d'entrée pour l'application DLL.
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#define LENGTH 17000000
#define BUFFER_LENGTH 300000
#define RESULT_LENGTH 100000
void treatment(char*, long unsigned int);
static int data[LENGTH];



void treatment(char* p_result, long  unsigned int result_length)
{
	int *buffer = (int*) calloc(BUFFER_LENGTH, sizeof(int));
    char *current = NULL;
    long unsigned int index;
    long unsigned int counter = 0;
	int value = (rand() % 2) + 1 ;
    for(index=0; index<LENGTH; index++){
        if(data[index] % value == 0){
			buffer[counter % BUFFER_LENGTH] = data[index];
            counter++;
        }
    }
    current = p_result;
    for(index=0; (index<counter && index<result_length); index++){
        *current = 32 + (buffer[index]%223);
        current++;
        //la plage de caractére imprimable ascii s'étend entre 32 et 255
	}
	free(buffer);
}

int main(int, char**){
  std::cout <<"Démarage" << std::endl;
  char response[RESULT_LENGTH];
  std::cout <<"C'est parti" << std::endl;
  for(int i = 0; i< LENGTH; i++){
    data[i] = i;
  }
  
  std::cout <<"Début du traitemment" << std::endl;
  for(int i=0; i<1000;i++){
    treatment(response, RESULT_LENGTH);
    if(i%100 == 0) std::cout<<  "Itération"  <<i<< std::endl;
  }
}