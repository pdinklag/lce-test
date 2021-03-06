#include <fstream>
#include <sys/time.h>
#include "structs/util/util.hpp"

#include "structs/lce_naive.hpp"
#include "structs/lce_prezza.hpp"
#include "structs/lce_synchronizing_sets.hpp"

//#define charTest
//#define randomLce
//#define exhaustiveLce


using namespace std;

const string fileNames{"ecoli.fa"};
//const string files[] {"../../text/dna.50MB"};
const string files[] {"/scratch/text/ecoli.fa"};

double timestamp();


int main(int argc, char *argv[]) {
	ofstream log("../testResults/correctness.txt", ios::out|ios::trunc);
	LceNaive T1(files[0]);
	LceSyncSets T2(files[0]);
	//LcePrezza T2(files[0]);
	
	//log << "Prime: "; util::printInt128(T2.getPrime());
	
	uint64_t textSizeInBytes = T1.getSizeInBytes();
	log << "Text: " << fileNames << '\n'
	    << "Size: " << textSizeInBytes << '\n';

	    
#ifdef charTest
	/* Test if character extraction works */
	log << "Testing character extraction (exhaustive)" << endl;
	for(uint64_t i = 0; i < textSizeInBytes; i++) {
		if (T1[i] != T2[i]) {
			log << "Mismatch at position " << i << endl;
			log << "NaiveLCE: " << T1[i] << endl;
			log << "FastLCE: " << T2[i] << endl; 
		}
	}
#endif
	
#ifdef randomLce
	log << "Testing LCE (random)" << endl;
	uint64_t i, j;
	for(uint64_t k = 0; k < 100000000; k++) {
		i = util::randomIndex(textSizeInBytes);
		j = util::randomIndex(textSizeInBytes);
		if (T1.lce(i, j) != T2.lce(i, j)) {
			log << "Wrong LCE at position " << i << " and " << j << endl;
			log << "NaiveLCE says: " << T1.lce(i, j) << endl;
			log << "FastLCE says: " << T2.lce(i, j) << endl;
		}
	}
#endif

#ifdef exhaustiveLce
	int c = 0;
	log << "Testing LCE (exhaustive)" << endl;
	for(uint64_t i = 0; i < textSizeInBytes; ++i) {
		//std::cout << i << std::endl;
		for(uint64_t j = 0; j < textSizeInBytes; ++j) {
		//std::cout << j << std::endl;
		uint64_t lce1 = T1.lce(i, j);
		uint64_t lce2 = T2.lce(i, j);
		
			if (lce1 != lce2) {
				if(c > 10000) {return 0;}
				++c;
				log << "WRONG LCE AT POSITION " << i << " and " << j << endl;
				log << "lceNaive  at " << i << " and " << j << ": " << lce1 << '\n';
				log << "lcePrezza at " << i << " and " << j << ": " << lce2 << '\n';
			}
		}
	}
#endif
	uint64_t i = 3200771;
	uint64_t j = 3083212;
	uint64_t lce1 = T1.lce(i, j);
	uint64_t lce2 = T2.lce(i, j);
	
	cout << "lceNaive  at " << i << " and " << j << ": " << lce1 << '\n';
	cout << "lceSSS at " << i << " and " << j << ": " << lce2 << '\n';
	log << "Test finished" << endl;
	return EXIT_SUCCESS;
}

double timestamp() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec / 1e6;
}
