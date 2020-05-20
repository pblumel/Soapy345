#ifndef SYMBOLLENTRACKER_H_
#define SYMBOLLENTRACKER_H_

#include <math.h>

template <typename T>
class SymbolLenTracker {
public:
	SymbolLenTracker(const unsigned int& size, const unsigned int& est_symbol_len);
	~SymbolLenTracker();
	void newSymbol();
	void operator++(int) const;
	const unsigned int getCurSymbolCount() const;
	void computeSyncAvg();
	void resetSyncAvg();
private:
	T* symbol_lengths {nullptr};
	unsigned int size;
	unsigned int front {0};
	const unsigned int est_symbol_len;
	float avg_symbol_len;	// Calculated per-message value, based on sync
};


template <typename T>
SymbolLenTracker<T>::SymbolLenTracker(const unsigned int& size, const unsigned int& est_symbol_len) :
		size(size), est_symbol_len(est_symbol_len), avg_symbol_len(est_symbol_len) {

	symbol_lengths = new unsigned int(size);	// Don't really care to initialize values,
									// they will be overwritten as values shift in
}


template <typename T>
SymbolLenTracker<T>::~SymbolLenTracker() {
	delete[] symbol_lengths;
}


template <typename T>
void SymbolLenTracker<T>::operator++(int) const {
	symbol_lengths[front]++;
}


template <typename T>
const unsigned int SymbolLenTracker<T>::getCurSymbolCount() const {
	unsigned int symbol_count = symbol_lengths[front]/avg_symbol_len;
	if (fmod(symbol_lengths[front], avg_symbol_len) > (avg_symbol_len/2.0)) {
		symbol_count++;
	}

	// Limit symbol count to two due to constraints of manchester encoding
	if (symbol_count > 2) {
		symbol_count = 2;
	}

	return symbol_count;
}


template <typename T>
void SymbolLenTracker<T>::newSymbol() {
	front = (front+1) % size;	// Rotate through tracker array
	symbol_lengths[front] = 1;	// One sample for this symbol has been seen,
								// it triggered the change in symbol state
}


template <typename T>
void SymbolLenTracker<T>::computeSyncAvg() {
	unsigned int sum = 0;
	for (unsigned int i = 0; i<(size-2); i++) {	// Skip the two most recent counts,
												// they may be distorted due to b0110 sequence
		auto index = (front+1+i) % size;	// Start with oldest manchester bit
		sum += symbol_lengths[index];
	}

	avg_symbol_len = float(sum)/(size-2);	// Compute average symbol length for this message
}


template <typename T>
void SymbolLenTracker<T>::resetSyncAvg() {
	avg_symbol_len = est_symbol_len;
}


#endif /* SYMBOLLENTRACKER_H_ */
