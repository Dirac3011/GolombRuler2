#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>
#include <omp.h>

using namespace std;

typedef __uint128_t bitmask;

// Pruning Table
const int g_min_span[] = {
    0, 0, 1, 2, 4, 6, 9, 13, 18, 23, 29, 36, 44, 53, 63, 73, 84, 94
};

// Shared flag to stop all threads once a solution is found for the current alpha
atomic<bool> g_found_at_alpha(false);

bool backtrack(int* current_set, int size, bitmask seen_once, bitmask seen_twice, int n_target, int alpha) {
    // Check if another thread already found the answer
    if (g_found_at_alpha) return false;

    int last_val = current_set[size - 1];
    int remaining = n_target - size;

    if (last_val + g_min_span[remaining + 1] > alpha) return false;

    if (size == n_target - 1) {
        for (int i = 0; i < size; ++i) {
            int diff = alpha - current_set[i];
            if ((seen_twice >> diff) & 1) return false;
        }
        current_set[size] = alpha; 
        g_found_at_alpha = true; // Signal all other threads to stop
        return true; 
    }

    int max_cand = alpha - g_min_span[remaining];
    for (int cand = last_val + 1; cand <= max_cand; ++cand) {
        bitmask new_diffs_bits = 0;
        bool conflict = false;

        for (int i = 0; i < size; ++i) {
            int diff = cand - current_set[i];
            if ((seen_twice >> diff) & 1) { conflict = true; break; }
            new_diffs_bits |= ((bitmask)1 << diff);
        }

        if (!conflict) {
            bitmask next_twice = seen_twice | (seen_once & new_diffs_bits);
            bitmask next_once = seen_once | new_diffs_bits;
            current_set[size] = cand;
            if (backtrack(current_set, size + 1, next_once, next_twice, n_target, alpha)) return true;
        }
    }
    return false;
}

int main() {
    int n = 15; 
    int alpha = 74; 
    
    cout << "Multi-threaded Search (OpenMP) | N=" << n << " | Threads=" << omp_get_max_threads() << endl;

    while (true) {
        auto start_time = chrono::high_resolution_clock::now();
        g_found_at_alpha = false;
        
        // We will store the winning set here
        int final_set[25];
        bool success = false;

        // Generate work units at depth 2 (the second element)
        // This allows us to parallelize across all possible starting combinations
        vector<int> second_elements;
        // Optimization: second element doesn't need to go higher than alpha/2 usually
        for (int s = 2; s < alpha / 2; ++s) second_elements.push_back(s);

        #pragma omp parallel
        {
            int thread_set[25];
            thread_set[0] = 1;

            // Use a dynamic schedule because some branches prune much faster than others
            #pragma omp for schedule(dynamic)
            for (int i = 0; i < second_elements.size(); ++i) {
                if (g_found_at_alpha) continue;

                int cand2 = second_elements[i];
                thread_set[1] = cand2;
                bitmask s1_once = ((bitmask)1 << (cand2 - 1));
                bitmask s1_twice = 0;

                // Further parallelization: split by the THIRD element
                // This ensures we have hundreds of tasks for the CPU cores
                int max_c3 = alpha - g_min_span[n - 2];
                for (int cand3 = cand2 + 1; cand3 <= max_c3; ++cand3) {
                    if (g_found_at_alpha) break;

                    // Calculate diffs for cand3 manually for the thread
                    int d1 = cand3 - 1;
                    int d2 = cand3 - cand2;
                    
                    if (!((s1_twice >> d1) & 1) && !((s1_twice >> d2) & 1)) {
                        bitmask new_diffs = ((bitmask)1 << d1) | ((bitmask)1 << d2);
                        bitmask s2_twice = s1_twice | (s1_once & new_diffs);
                        bitmask s2_once = s1_once | new_diffs;
                        
                        thread_set[2] = cand3;
                        if (backtrack(thread_set, 3, s2_once, s2_twice, n, alpha)) {
                            #pragma omp critical
                            {
                                success = true;
                                for(int k=0; k<n; ++k) final_set[k] = thread_set[k];
                            }
                        }
                    }
                }
            }
        }

        if (success) {
            cout << "\nFOUND! alpha=" << alpha << " | [";
            for (int i = 0; i < n; ++i) cout << final_set[i] << (i == n - 1 ? "" : ", ");
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