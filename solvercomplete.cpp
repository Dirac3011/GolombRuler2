#include <iostream>
#include <vector>
#include <chrono>

using namespace std;

typedef __uint128_t bitmask;

// Pruning Table
const int g_min_span[] = {
    0,  // k=0
    0,  // k=1
    1,  // k=2
    2,  // k=3
    4,  // k=4
    6,  // k=5
    9,  // k=6
    13, // k=7
    18, // k=8
    23, // k=9
    29, // k=10
    36, // k=11
    44, // k=12
    53, // k=13
    63, // k=14
    73, // k=15
    83  // k=16 (Estimated/Known lower bound)
};

bool backtrack(int* current_set, int size, bitmask seen_once, bitmask seen_twice, int n_target, int alpha) {
    int last_val = current_set[size - 1];
    int remaining = n_target - size;

    // Pruning: if current value + min span for remaining elements > target alpha
    if (last_val + g_min_span[remaining + 1] > alpha) {
        return false;
    }

    // Base case: check the final 'alpha' as the last element
    if (size == n_target - 1) {
        for (int i = 0; i < size; ++i) {
            int diff = alpha - current_set[i];
            if ((seen_twice >> diff) & 1) return false;
        }
        current_set[size] = alpha; 
        return true; 
    }

    int max_cand = alpha - g_min_span[remaining];

    for (int cand = last_val + 1; cand <= max_cand; ++cand) {
        bitmask new_diffs_bits = 0;
        bool conflict = false;

        for (int i = 0; i < size; ++i) {
            int diff = cand - current_set[i];
            if ((seen_twice >> diff) & 1) {
                conflict = true;
                break;
            }
            new_diffs_bits |= ((bitmask)1 << diff);
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
    int alpha = 74; // Starting target
    int current_set[25]; 

    cout << "Searching for n=" << n << " starting at alpha=" << alpha << " (Universal Search)..." << endl;

    while (true) {
        auto start_time = chrono::high_resolution_clock::now();

        // Start with ONLY the first element. 
        // This removes the {1, 2} restriction.
        current_set[0] = 1;
        bitmask initial_once = 0; 
        bitmask initial_twice = 0;

        // Start backtracking from size=1
        if (backtrack(current_set, 1, initial_once, initial_twice, n, alpha)) {
            cout << "\nFOUND! n=" << n << " | alpha=" << alpha << " | [";
            for (int i = 0; i < n; ++i) {
                cout << current_set[i] << (i == n - 1 ? "" : ", ");
            }
            cout << "]" << endl;
            break;
        } else {
            auto end_time = chrono::high_resolution_clock::now();
            chrono::duration<double> elapsed = end_time - start_time;
            cout << "alpha=" << alpha << " failed (" << elapsed.count() << "s)" << endl;
            alpha++;
        }
    }

    return 0;
}