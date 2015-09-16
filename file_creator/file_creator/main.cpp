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
	/*int m[100]={0};
    std::srand((unsigned)time(NULL));  
   
    for(int i = 0; i < 100; i++)
    { 
		m[i] = rand()%10;
		std::cout << m[i] << ' ';            
    }
    cout << '\n';*/
	uint32_t m[30] = { 10, 12, 13, 9, 5, 6, 8, 50, 25, 40, 32, 9, 75, 28, 10, 12, 17, 36, 35, 33, 34, 15, 1, 5, 6, 19, 40, 42, 37, 12 };
    
	std::ofstream file;
	file.open("unsorted", std::ofstream::binary);	
	if (file.is_open()) {
		for(int i = 0; i < sizeof(m)/sizeof(uint32_t); i++) {
			file.write((char *)&m[i], sizeof(int)); 
		}
	}
	
    file.close();
	return 0;
}