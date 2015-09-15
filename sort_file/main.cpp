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

#define FILENAME "d:\\work\\kraftway\\file_creator\\file_creator\\unsorted"
#define OUT_FILENAME "sorted"
//#define MEM_LIMIT (1000*1024*1024) // 1000 Mb
//#define CHUNK_SIZE (50*1024*1024) // 50 Mb

#define MEM_LIMIT (1024) // 1024 Bytes
#define CHUNK_SIZE 256 // 50 Bytes
#define BUF_SIZE 40 // 40 Bytes

class ExternalSortException : public std::runtime_error {
public:
    ExternalSortException(const string& message) 
            : std::runtime_error(message) { };
};


class ExternalSortTest;

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
	ifstream m_file;
	ofstream m_out_file;
	const char *m_out_filename;
	//std::ifstream::pos_type m_size; 
	void write_chunk(const char *filename, std::vector<uint32_t>& what);
	bool push_chunk(vector<uint32_t> &chunk);
	void merge(vector< vector<uint32_t> > &what, vector<uint32_t> &goal_buf);
	void save(vector<uint32_t> goal_buf);

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
				//cout << v << ' ';
			//cout << '\n';
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
	

	std::vector<uint32_t> res;
	read_file(m_out_filename, res);

}

void ExternalSort::merge(vector< vector<uint32_t> > &what, vector<uint32_t> &goal_buf)
{
	assert(what.size());

	unsigned buf_size = m_buf_size/sizeof(uint32_t);
	vector< vector<uint32_t> >::iterator it = what.begin();
	unsigned buf_cnt = what.size();
	unsigned i = 0;
	unsigned j = 0;
	unsigned to_del = 0;
	unsigned min = 0;
	unsigned min_idx = 0;

	while(i < buf_size) {
		
		while(min_idx < buf_size) {
			if (what[min_idx].size()) {
				min = what[min_idx][0];
				to_del = min_idx;
				break;
			}
			min_idx++;
		}
		
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
		throw std::exception();
	}
}

bool chunk_compare (int i, int j) { return (i < j); }

bool ExternalSort::push_chunk(vector<uint32_t> &chunk)
{
		bool eof = false;
		const unsigned chunk_num_cnt  = m_chunk_size / sizeof(uint32_t);
		assert(chunk_num_cnt > 1);
		
		unsigned i = 0;
		uint32_t v;
		while (i < chunk_num_cnt) {
			if (m_file.read((char *)&(v), sizeof(v)))
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

void ExternalSort::sort()
{
	bool eof = false;
	unsigned i = 0;
	vector<std::string> chunk_files;
	
	while(!eof) {
		vector<uint32_t> chunk;
		eof = push_chunk(chunk);
		
		if (chunk.size()) {
			std::stringstream ss;
			std::string s = to_string((long double)++i);
			ss << "chunk" << s.c_str();
			write_chunk(ss.str().c_str(), chunk);
			chunk_files.push_back(ss.str());
		}
	}

	assert(chunk_files.size());
	
	{
		vector< vector<uint32_t> > bufs;
		vector<std::string>::iterator it = chunk_files.begin();
		// for every chunk file we have to remember current position
		std::vector<unsigned> chunk_pos;
		chunk_pos.resize(chunk_files.size(), 0);
		// create buffers
		while (it != chunk_files.end()) {
			std::vector<uint32_t> buf;
			//buf.resize(buf_size, -1);
			bufs.push_back(buf);
			it++;
		}
		// walk throught chunk files
		it = chunk_files.begin();
		unsigned buf_size = m_buf_size/sizeof(uint32_t);
		bool not_eof = true;
		uint32_t v;
		
		while (not_eof) {
			
			it = chunk_files.begin();
			unsigned buf_idx = 0;
			not_eof = false;
			while (it != chunk_files.end()) {
				
				unsigned buf_cnt = buf_size;
				std::vector<uint32_t> buf;
				ifstream chunk_file(*it);
				chunk_file.seekg(chunk_pos[buf_idx] * sizeof(uint32_t));
				unsigned idx = 0;
				while (buf_cnt--) {
					if (buf_size > bufs[buf_idx].size()) {		
						if (chunk_file.read((char *)&(v), sizeof(v))) {
							bufs[buf_idx].push_back(v);
							not_eof = true;
							chunk_pos[buf_idx]++;
						}
					}
					idx++;
				}
				//bufs.push_back(buf);
				it++;
				buf_idx++;				
				//buf_num++;
			}
			
			// send values from buffers into goal buffer
			std::vector<uint32_t> goal_buf;
			merge(bufs, goal_buf);
			// save goal buffer in file
			save(goal_buf);		
		}



		
	}

}

class ExternalSortTest : public ExternalSort
{
public:
	ExternalSortTest(const char *filename, const char *out_filename, unsigned chunk_size, unsigned buf_size) : ExternalSort(filename, out_filename, chunk_size, buf_size) 
	{
		m_out_filename = out_filename;
	}
	void print_file()
	{		
		uint32_t v;
		while(m_file.read((char *)&v, sizeof(v))) 
			cout << v << ' ';
		cout << '\n';
	};

	void print_out_file()
	{	
		std::ifstream file;
		file.open(m_out_filename, std::ofstream::binary);	
		if (file.is_open()) {
			uint32_t v;
			while(file.read((char *)&v, sizeof(v))) 
				cout << v << ' ';
			cout << '\n';
			file.close();
		}
	};

private:
		const char *m_out_filename;
	
};


int main()
{
	try {
		ExternalSortTest est(FILENAME, OUT_FILENAME, 40, 12);
		//est.print();
		est.sort();
		est.print_out_file();
	} catch (std::exception)
	{
		fprintf(stderr, "Exception is occured\n");
	}
	 
	return 0;
}
