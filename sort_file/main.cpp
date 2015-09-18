#include <iostream>
#include <fstream>
#include <stdint.h>
#include <ctime>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>
#include <stdexcept>
#include <exception>

using namespace std;

#define FILENAME "unsorted"
#define FILENAME1 "unsorted1"
#define FILENAME2 "unsorted2"
#define OUT_FILENAME "sorted"
//#define MEM_LIMIT (1000*1024*1024) // 1000 Mb
//#define CHUNK_SIZE (50*1024*1024) // 50 Mb

#define MEM_LIMIT (1024) // 1024 Bytes
#define CHUNK_SIZE 256 // 50 Bytes
#define BUF_SIZE 40 // 40 Bytes

// custom exception to avoid non standart std::exception
class ExternalSortException : public std::runtime_error {
public:
    ExternalSortException(const string& message) 
            : std::runtime_error(message) { };
};


class ExternalSortTest;

/**
  * External sort method implementation
  * Big file is divided into several chunks, each chunk is sorted separately
  * From every chunk small buffer is extracted and merged into goal buffer (consequentially)
  * till numbers is over
  */
class ExternalSort
{
public:
	ExternalSort(const char *filename, const char*out_filename, unsigned chunk_size, unsigned buf_size) 
		: m_out_filename(out_filename)
	{
		assert(chunk_size > buf_size && chunk_size > 1);
		assert(buf_size > 1);
		m_file.open(filename,  ios::binary);
		if (!m_file.is_open()) {
			throw ExternalSortException("Couldn't open input file");
		}

		m_out_file.open(out_filename,  ios::binary);
		if (!m_out_file.is_open()) {
			throw ExternalSortException("Couldn't open output file");
		}
		
		m_chunk_size = chunk_size;
		m_buf_size = buf_size;
	}

	~ExternalSort()
	{
		if (m_file.is_open())
			m_file.close();
	}

	friend class ExternalSortTest;

	void sort();
	
private:

	ifstream m_file;			// input unsorted file
	ofstream m_out_file;		// output sorted file
	const char *m_out_filename;	// output filename

	// write chunk numbers into file represented by @filename
	void write_chunk(const char *filename, std::vector<uint32_t>& what);
	// read numbers from file and create chunk (must be executed consequentially)
	bool push_chunk(vector<uint32_t> &chunk, ifstream &file);
	// erase minimal numbers from @what and fill @goal_buf
	void merge(vector< vector<uint32_t> > &what, vector<uint32_t> &goal_buf);
	// save sorted values in output file
	void save(vector<uint32_t> goal_buf);
	// walk throught chunk files for buffer creation
	void walk_chunk_files(vector<std::string> &chunk_files);

	//	for test purposes
	void read_file(const char *filename, std::vector<uint32_t> &buf);
	
	unsigned m_chunk_size;
	unsigned m_buf_size;

};

void ExternalSort::read_file(const char *filename, std::vector<uint32_t> &buf)
{	
		std::ifstream file;
		file.open(filename, std::ofstream::binary);	
		if (file.is_open()) {
			uint32_t v;
			while(file.read((char *)&v, sizeof(v))) 
				buf.push_back(v);				
			file.close();
		}
};

void ExternalSort::save(vector<uint32_t> goal_buf)
{
	if (m_out_file.is_open()) {
		uint32_t v;
		for(unsigned i = 0; i < goal_buf.size(); i++) {
			v = goal_buf[i];
			m_out_file.write((char *)&v, sizeof(v)); 
		}
		m_out_file.flush();
	}
	
#ifdef DEBUG
	std::vector<uint32_t> res;
	read_file(m_out_filename, res);
#endif

}

void ExternalSort::merge(vector< vector<uint32_t> > &what, vector<uint32_t> &goal_buf)
{
	assert(what.size());
	assert(m_buf_size%sizeof(uint32_t) == 0);
	
	unsigned buf_size = m_buf_size/sizeof(uint32_t);
	vector< vector<uint32_t> >::iterator it = what.begin();
	unsigned buf_cnt = what.size();
	unsigned i = 0;
	unsigned j = 0;
	unsigned to_del = 0;
	unsigned min = 0;
	unsigned min_idx = 0;
	bool what_empty = false;

	while(i < buf_size) {

		what_empty = true;
		
		while(min_idx < buf_cnt) {
			if (what[min_idx].size()) {
				min = what[min_idx][0];
				to_del = min_idx;
				what_empty = false;
				break;
			}
			min_idx++;
		}

		if (what_empty)
			break;
		
		while (j < buf_cnt) {
			if (what[j].size() && what[j][0] < min) {
				min = what[j][0];
				to_del = j;
			}
			j++;
		}

		goal_buf.push_back(min);
		what[to_del].erase(what[to_del].begin());		
		i++;
		j = 0;
	}
}

void ExternalSort::write_chunk(const char *filename, std::vector<uint32_t>& what)
{
	std::ofstream file;
	file.open(filename, std::ofstream::binary);	
	if (file.is_open()) {
		uint32_t v;
		for(unsigned i = 0; i < what.size(); i++) {
			v = what[i];
			file.write((char *)&v, sizeof(v)); 
		}
		file.close();
	} else {
		throw ExternalSortException("Couldn't open chunk file");
	}
}

bool chunk_compare (int i, int j) { return (i < j); }

bool ExternalSort::push_chunk(vector<uint32_t> &chunk, ifstream &file)
{
		bool eof = false;
		assert(m_chunk_size%sizeof(uint32_t) == 0);
		const unsigned chunk_num_cnt  = m_chunk_size / sizeof(uint32_t);

		assert(chunk_num_cnt > 1);		
		
		unsigned i = 0;
		uint32_t v;
		while (i < chunk_num_cnt) {
			if (file.read((char *)&(v), sizeof(v)))
				chunk.push_back(v);
			else {
				eof = true;
				break;
			}
			i++;
		}
		std::sort(chunk.begin(), chunk.end(), chunk_compare);		

		return eof;
}

void ExternalSort::walk_chunk_files(vector<std::string> &chunk_files)
{
		assert(chunk_files.size());

		vector< vector<uint32_t> > bufs;
		vector<std::string>::iterator it = chunk_files.begin();
		
		// for every chunk file we have to remember current position
		std::vector<unsigned> chunk_pos;
		chunk_pos.resize(chunk_files.size(), 0);
		// create buffers
		while (it != chunk_files.end()) {
			std::vector<uint32_t> buf;			
			bufs.push_back(buf);
			it++;
		}

		// walk throught chunk files
		it = chunk_files.begin();
		
		assert(m_buf_size%sizeof(uint32_t) == 0);

		unsigned buf_size = m_buf_size/sizeof(uint32_t);
		bool not_eof = true;
		uint32_t v;
		
		// while there is data in files
		while (not_eof) {
			
			it = chunk_files.begin();
			unsigned buf_idx = 0;
			not_eof = false;
			while (it != chunk_files.end()) {
				
				unsigned buf_cnt = buf_size;
				std::vector<uint32_t> buf;
				ifstream chunk_file(*it);
				chunk_file.seekg(chunk_pos[buf_idx] * sizeof(uint32_t));				
				while (buf_cnt--) {
					if (buf_size > bufs[buf_idx].size()) {		
						if (chunk_file.read((char *)&(v), sizeof(v))) {
							bufs[buf_idx].push_back(v);
							not_eof = true;
							chunk_pos[buf_idx]++;
						}
					}					
				}				
				it++;
				buf_idx++;				
			}

			// send values from buffers into goal buffer
			std::vector<uint32_t> goal_buf;
			merge(bufs, goal_buf);
			// save goal buffer in file
			save(goal_buf);		
		}

		// it the rest in buffer exists merge it out
		std::vector<uint32_t> goal_buf;
		merge(bufs, goal_buf);
		save(goal_buf);	

}

void ExternalSort::sort()
{
	bool eof = false;
	unsigned i = 0;
	vector<std::string> chunk_files;
	
	while(!eof) {
		vector<uint32_t> chunk;
		eof = push_chunk(chunk, m_file);
		
		if (chunk.size()) {
			std::stringstream ss;
			std::string s = to_string((long double)++i);
			ss << "chunk" << s.c_str();
			write_chunk(ss.str().c_str(), chunk);
			chunk_files.push_back(ss.str());
		}
	}

	walk_chunk_files(chunk_files);
}

class ExternalSortTest : public ExternalSort
{
public:
	ExternalSortTest(const char *filename, const char *out_filename, unsigned chunk_size, unsigned buf_size) : ExternalSort(filename, out_filename, chunk_size, buf_size) {}
	void print_file()
	{		
		uint32_t v;
		while(m_file.read((char *)&v, sizeof(v))) 
			cout << v << ' ';
		cout << '\n';
	};

	void print_out_file()
	{	
		cout << "output filename: " << m_out_filename << endl;
		m_out_file.close();
		std::ifstream file;
		file.open(m_out_filename, std::ofstream::binary);	
		if (file.is_open()) {
			uint32_t v;
			while(file.read((char *)&v, sizeof(v))) 
				cout << v << ' ';
			cout << '\n';
			file.close();
		} else {
			throw ExternalSortException("Couldn't open output file");
		}
	};

	void test_push_chunk()
	{
		std::vector<uint32_t> chunk;

		ifstream file;
		
		file.open("unsorted1",  ios::binary);
		if (!m_file.is_open()) {
			throw ExternalSortException("Couldn't open input file");
		}

		// uint32_t v;
		
		/*if (!file.read((char *)&(v), sizeof(v))) {
			throw ExternalSortException("Couldn't read input file");
		}*/

		//m_chunk_size = 1;
		//push_chunk(chunk, file);	// assert - ok

		// chunk.clear();
		// m_chunk_size=100;
		// push_chunk(chunk, file); // ok

		//chunk.clear();
		//m_chunk_size=29;  
		//push_chunk(chunk, file);	// assert - ok (must be multiple of 4) 

		chunk.clear();
		m_chunk_size=4294967292;	// 2^32-4
		push_chunk(chunk, file);

		file.close();

		file.open("unsorted2",  ios::binary);
		if (!m_file.is_open()) {
			throw ExternalSortException("Couldn't open input file");
		}

		 chunk.clear();
		 m_chunk_size=100;
		 push_chunk(chunk, file); // ok

		 file.close();
	}

	void test_merge()
	{
		vector< vector<uint32_t> > what;
		vector<uint32_t> goal_buf;
		
		// merge(what, goal_buf); // assert - ok

		//what.resize(1000000);
		//merge(what, goal_buf);	// assert - ok

		vector<uint32_t> v;
		v.resize(100, 10);
		what.clear();
		what.push_back(v);
		merge(what, goal_buf);
	}	
	
};


int main()
{
	{
		/*try {
			ExternalSortTest est(FILENAME, OUT_FILENAME, 40, 12);
			est.test_push_chunk();
			est.test_merge();
			//est.sort();
			//est.print_out_file();
		} catch (std::exception)
		{
			fprintf(stderr, "Exception is occurred\n");
		}*/
	}

	{
		try {
			ExternalSortTest est(FILENAME1, OUT_FILENAME, 40, 12);			
			est.sort();
			est.print_out_file();
		} catch (std::exception)
		{
			fprintf(stderr, "Exception is occurred\n");
		}
	}

	{
		/*try {
			ExternalSortTest est(FILENAME2, OUT_FILENAME, 100, 12);			
			est.sort();
			est.print_out_file();
		} catch (std::exception)
		{
			fprintf(stderr, "Exception is occurred\n");
		}*/
	}


	 
	return 0;
}
