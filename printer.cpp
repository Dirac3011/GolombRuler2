#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <thread>
#include <mutex>
#include <algorithm>

using namespace std;

typedef __uint128_t bitmask;

const int g_min_span[] = {
    0, 0, 1, 2, 4, 6, 9, 13, 18, 23, 29, 36, 44, 53, 63, 74, 84, 96
};

vector<vector<int>> all_optimal_sets;
mutex mtx;

void backtrack_all(int* A, int size, bitmask m1, bitmask m2, bitmask m3, int n_target, int alpha) {
    int last_val = A[size - 1];
    int remaining = n_target - size;

    if (last_val + g_min_span[remaining + 1] > alpha) {
        return;
    }

    // Base Case
    if (size == n_target - 1) {
        for (int i = 0; i < size; ++i) {
            int diff = alpha - A[i];
            // CHANGE: Reject if this distance already exists TWICE (m2 bit set)
            if ((m2 >> diff) & 1) return;
        }
        
        vector<int> found_set(n_target);
        for(int i=0; i<size; ++i) found_set[i] = A[i];
        found_set[n_target-1] = alpha;

        lock_guard<mutex> lock(mtx);
        all_optimal_sets.push_back(found_set);
        return;
    }

    int max_cand = alpha - g_min_span[remaining];

    for (int cand = last_val + 1; cand <= max_cand; ++cand) {
        bitmask new_diffs = 0;
        bool conflict = false;

        for (int i = 0; i < size; ++i) {
            int d = cand - A[i];
            // CHANGE: Reject if the 2nd bit is set (meaning this would be the 3rd appearance)
            if ((m2 >> d) & 1) {
                conflict = true;
                break;
            }
            new_diffs |= ((bitmask)1 << d);
        }

        if (!conflict) {
            // Bitmasks continue to update normally:
            // m3 will still track 3rds, but we now stop the search before they are allowed
            bitmask next_m3 = m3 | (m2 & new_diffs);
            bitmask next_m2 = m2 | (m1 & new_diffs);
            bitmask next_m1 = m1 | new_diffs;

            A[size] = cand;
            backtrack_all(A, size + 1, next_m1, next_m2, next_m3, n_target, alpha);
        }
    }
}

void worker(int a1_val, int n_target, int alpha) {
    int local_set[64];
    local_set[0] = 0;
    local_set[1] = a1_val;

    bitmask m1 = ((bitmask)1 << a1_val);
    bitmask m2 = 0, m3 = 0;

    backtrack_all(local_set, 2, m1, m2, m3, n_target, alpha);
}

void save_to_json(int n, int alpha, double elapsed) {
    string filename = "optimal_sets_n" + to_string(n) + "_a" + to_string(alpha) + ".json";
    ofstream file(filename);

    file << "{\n";
    file << "  \"n\": " << n << ",\n";
    file << "  \"alpha\": " << alpha << ",\n";
    file << "  \"count\": " << all_optimal_sets.size() << ",\n";
    file << "  \"time_seconds\": " << fixed << setprecision(4) << elapsed << ",\n";
    file << "  \"sets\": [\n";

    for (size_t i = 0; i < all_optimal_sets.size(); ++i) {
        file << "    [";
        for (size_t j = 0; j < all_optimal_sets[i].size(); ++j) {
            file << all_optimal_sets[i][j] << (j == all_optimal_sets[i].size() - 1 ? "" : ", ");
        }
        file << "]" << (i == all_optimal_sets.size() - 1 ? "" : ",") << "\n";
    }

    file << "  ]\n";
    file << "}\n";
    cout << "Results saved to " << filename << endl;
}

int main() {
    int n, alpha;
    cout << "Target n: "; cin >> n;
    cout << "Optimal alpha: "; cin >> alpha;

    auto start = chrono::high_resolution_clock::now();

    vector<thread> threads;
    int max_a1 = alpha / 2;

    cout << "Starting exhaustive search across " << max_a1 << " main branches..." << endl;
    for (int a1 = 1; a1 <= max_a1; ++a1) {
        threads.emplace_back(worker, a1, n, alpha);
    }

    for (auto& t : threads) t.join();

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    cout << "\nSearch complete. Found " << all_optimal_sets.size() << " unique sets." << endl;
    save_to_json(n, alpha, elapsed.count());

    return 0;
}