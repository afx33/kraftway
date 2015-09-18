/**
  * Create binary file with random numbers
  */

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <ctime>
#include <stdint.h>

using namespace std;

int main()
{
	
    uint32_t m[30] = { 10, 12, 13, 9, 5, 6, 8, 50, 25, 40, 32, 9, 75, 28, 10, 12, 17, 36, 35, 33, 34, 15, 1, 5, 6, 19, 40, 42, 37, 12 };

	{
		std::ofstream file;
		file.open("unsorted", std::ofstream::binary);	
		if (file.is_open()) {
			for(unsigned i = 0; i < sizeof(m)/sizeof(uint32_t); i++) {
				file.write((char *)&m[i], sizeof(int)); 
			}
		}
	
		file.close();
	}

	{
		std::ofstream file;
		file.open("unsorted1", std::ofstream::binary);	
		if (file.is_open()) {
			for(unsigned i = 0; i < sizeof(m)/sizeof(uint32_t); i++) {
				file.write((char *)&m[i], sizeof(int)); 
			}
		}
	
		file.close();
	}

	{
		uint32_t *pm = new uint32_t[10];
		std::ofstream file;
		file.open("unsorted2", std::ofstream::binary);	
		for(unsigned k=0; k < 10; k++)	{	
			if (file.is_open()) {
				for(unsigned i = 0; i < sizeof(m)/sizeof(uint32_t); i++) { 
					m[i] = rand();		
					file.write((char *)&m[i], sizeof(uint32_t)); 					
				}
			}
		}
		file.close();
		delete [] pm;
	}

	/*{	// create file ~ 11Gb
		uint32_t *pm = new uint32_t[100000000];
		std::ofstream file;
		file.open("unsorted3", std::ofstream::binary);	
		for(unsigned k=0; k < 100000000; k++)	{	
			if (file.is_open()) {
				for(unsigned i = 0; i < sizeof(m)/sizeof(uint32_t); i++) { 
					m[i] = rand();		
					file.write((char *)&m[i], sizeof(uint32_t)); 					
				}
			}
		}
		file.close();
		delete [] pm;
	}*/
	return 0;
}
