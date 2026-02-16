#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>

using namespace std;

// uint64_t allows us to handle differences up to 63. 
// If alpha > 64, use __uint128_t or std::bitset.
typedef uint64_t bitmask;

bool backtrack(int* current_set, int size, bitmask seen_once, bitmask seen_twice, int n_target, int alpha) {
    // Base case: We have n-1 elements. Check if the final 'alpha' is valid.
    if (size == n_target - 1) {
        for (int i = 0; i < size; ++i) {
            int diff = alpha - current_set[i];
            if ((seen_twice >> diff) & 1ULL) return false;
        }
        return true; // Found a solution!
    }

    int last_val = current_set[size - 1];
    int remaining = n_target - size;
    int end = alpha - remaining + 1;

    for (int cand = last_val + 1; cand <= end; ++cand) {
        bitmask new_diffs_bits = 0;
        bool conflict = false;

        for (int i = 0; i < size; ++i) {
            int diff = cand - current_set[i];
            if ((seen_twice >> diff) & 1ULL) {
                conflict = true;
                break;
            }
            new_diffs_bits |= (1ULL << diff);
        }

        if (!conflict) {
            bitmask next_twice = seen_twice | (seen_once & new_diffs_bits);
            bitmask next_once = seen_once | new_diffs_bits;
            
            current_set[size] = cand;
            if (backtrack(current_set, size + 1, next_once, next_twice, n_target, alpha)) {
                return true;
            }
        }
    }
    return false;
}

int main() {
    int n = 15;
    int alpha = 75;
    int current_set[15]; // Fixed size array for speed

    cout << "Searching for n=" << n << " starting at alpha=" << alpha << "..." << endl;

    while (true) {
        auto start_time = chrono::high_resolution_clock::now();

        // Initialize with [1, 2]
        current_set[0] = 1;
        current_set[1] = 2;
        bitmask initial_once = (1ULL << 1); // diff (2-1) = 1
        bitmask initial_twice = 0;

        if (backtrack(current_set, 2, initial_once, initial_twice, n, alpha)) {
            cout << "FOUND! n=" << n << " | alpha=" << alpha << " | [";
            for (int i = 0; i < n - 1; ++i) cout << current_set[i] << ", ";
            cout << alpha << "]" << endl;
            break;
        } else {
            auto end_time = chrono::high_resolution_clock::now();
            chrono::duration<double> elapsed = end_time - start_time;
            cout << "alpha=" << alpha << " failed in " << elapsed.count() << "s" << endl;
            alpha++;
        }
    }

    return 0;
}