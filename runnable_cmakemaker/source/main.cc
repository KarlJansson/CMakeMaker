#include "cmake_writer.h"
#include "repo_searcher.h"

int main(int argc, char** argv) { 
	RepoSearcher searcher;
	CmakeWriter writer(searcher);
	writer.WriteCmakeFiles();
	return 0; 
}