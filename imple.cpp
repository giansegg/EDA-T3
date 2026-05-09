// bench.cpp — vEB layout vs Pointer BST
// g++ -O3 -march=native -std=c++17 -o bench bench.cpp

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <random>
#include <vector>

using namespace std;

// ─── Pointer BST ─────────────────────────────────────────────────────────────

struct Node { int32_t key; Node *left, *right; };

static vector<Node> bst_pool;
static int          bst_pool_idx;

static Node* bst_build(const vector<int32_t>& v, int lo, int hi) {
    if (lo > hi) return nullptr;
    int mid  = lo + (hi - lo) / 2;
    Node* n  = &bst_pool[bst_pool_idx++];
    n->key   = v[mid];
    n->left  = bst_build(v, lo, mid - 1);
    n->right = bst_build(v, mid + 1, hi);
    return n;
}

static bool bst_find(const Node* r, int32_t k) {
    while (r) {
        if (k == r->key) return true;
        r = k < r->key ? r->left : r->right;
    }
    return false;
}

// ─── vEB build ───────────────────────────────────────────────────────────────

static int veb_build(int32_t* out, const int32_t* sorted, int h) {
    if (h == 0) return 0;
    if (h == 1) { *out = *sorted; return 1; }

    const int top_h   = h / 2;
    const int bot_h   = h - top_h;
    const int num_top = (1 << top_h) - 1;
    const int bot_sz  = (1 << bot_h) - 1;
    const int stride  = bot_sz + 1;             // 2^bot_h
    const int num_bot = 1 << top_h;

    vector<int32_t> top(num_top);
    for (int j = 0; j < num_top; j++)
        top[j] = sorted[(size_t)j * stride + (stride - 1)]; // in-order medianas del top

    int pos = veb_build(out, top.data(), top_h);
    for (int k = 0; k < num_bot; k++)
        pos += veb_build(out + pos, sorted + (size_t)k * stride, bot_h);
    return pos;
}

// ─── vEB find — recursivo dinámico (baseline) ────────────────────────────────

static bool veb_find_dyn(const int32_t* a, int h, int32_t key, int& gap) {
    if (h == 0) { gap = 0; return false; }
    if (h == 1) { gap = (key >= *a); return *a == key; }

    const int top_h   = h / 2;
    const int bot_h   = h - top_h;
    const int num_top = (1 << top_h) - 1;
    const int bot_sz  = (1 << bot_h) - 1;

    int j;
    if (veb_find_dyn(a, top_h, key, j)) return true;
    int q;
    bool found = veb_find_dyn(a + num_top + (size_t)j * bot_sz, bot_h, key, q);
    gap = (j << bot_h) | q;
    return found;
}

// ─── vEB find — template inlined, cero overhead de llamadas ──────────────────

template<int H>
[[gnu::always_inline]]
static bool veb_find_t(const int32_t* a, int32_t key, int& gap) {
    if constexpr (H == 0) {
        gap = 0; return false;
    } else if constexpr (H == 1) {
        gap = (key >= *a);          // branchless
        return *a == key;
    } else {
        constexpr int top_h   = H / 2;
        constexpr int bot_h   = H - top_h;
        constexpr int num_top = (1 << top_h) - 1;
        constexpr int bot_sz  = (1 << bot_h) - 1;

        int j;
        if (veb_find_t<top_h>(a, key, j)) return true;
        int q;
        bool found = veb_find_t<bot_h>(a + num_top + (size_t)j * bot_sz, key, q);
        gap = (j << bot_h) | q;
        return found;
    }
}

// ─── VEBTree ─────────────────────────────────────────────────────────────────

struct VEBTree {
    vector<int32_t> data;
    int h = 0;

    void build(vector<int32_t> keys) {
        h = 0;
        while ((1 << h) - 1 < (int)keys.size()) ++h;
        keys.resize((size_t)(1 << h) - 1, INT32_MAX); // relleno con centinela
        data.resize(keys.size());
        veb_build(data.data(), keys.data(), h);
    }

    bool find_opt(int32_t k) const {
        int gap;
        switch (h) {                                   // dispatch estático por altura
            case  1: return veb_find_t< 1>(data.data(), k, gap);
            case  2: return veb_find_t< 2>(data.data(), k, gap);
            case  3: return veb_find_t< 3>(data.data(), k, gap);
            case  4: return veb_find_t< 4>(data.data(), k, gap);
            case  5: return veb_find_t< 5>(data.data(), k, gap);
            case  6: return veb_find_t< 6>(data.data(), k, gap);
            case  7: return veb_find_t< 7>(data.data(), k, gap);
            case  8: return veb_find_t< 8>(data.data(), k, gap);
            case  9: return veb_find_t< 9>(data.data(), k, gap);
            case 10: return veb_find_t<10>(data.data(), k, gap);
            case 11: return veb_find_t<11>(data.data(), k, gap);
            case 12: return veb_find_t<12>(data.data(), k, gap);
            case 13: return veb_find_t<13>(data.data(), k, gap);
            case 14: return veb_find_t<14>(data.data(), k, gap);
            case 15: return veb_find_t<15>(data.data(), k, gap);
            case 16: return veb_find_t<16>(data.data(), k, gap);
            case 17: return veb_find_t<17>(data.data(), k, gap);
            case 18: return veb_find_t<18>(data.data(), k, gap);
            case 19: return veb_find_t<19>(data.data(), k, gap);
            case 20: return veb_find_t<20>(data.data(), k, gap);
            case 21: return veb_find_t<21>(data.data(), k, gap);
            case 22: return veb_find_t<22>(data.data(), k, gap);
            case 23: return veb_find_t<23>(data.data(), k, gap);
            case 24: return veb_find_t<24>(data.data(), k, gap);
            case 25: return veb_find_t<25>(data.data(), k, gap);
            default: return veb_find_dyn(data.data(), h, k, gap);
        }
    }

    bool find_dyn(int32_t k) const {
        int gap;
        return veb_find_dyn(data.data(), h, k, gap);
    }
};

// ─── Benchmark ───────────────────────────────────────────────────────────────

template<typename Fn>
static double elapsed_ms(Fn fn) {
    using Clock = chrono::high_resolution_clock;
    auto t0 = Clock::now();
    fn();
    return chrono::duration<double, milli>(Clock::now() - t0).count();
}

int main() {
    constexpr int N = 20'000'000;
    constexpr int Q =  1'000'000;
    constexpr int T = 7;

    mt19937 rng(42);
    uniform_int_distribution<int32_t> dist(1, 10 * N);

    vector<int32_t> keys(N);
    for (auto& k : keys) k = dist(rng);
    sort(keys.begin(), keys.end());
    keys.erase(unique(keys.begin(), keys.end()), keys.end());
    const int actual_N = (int)keys.size();

    vector<int32_t> queries(Q);
    for (auto& q : queries) q = dist(rng);

    printf("Building (N=%d)...\n", actual_N);

    VEBTree veb;
    veb.build(keys);
    printf("  vEB: h=%d, %.0f MB\n", veb.h, veb.data.size() * 4.0 / 1e6);

    bst_pool.resize(actual_N);
    bst_pool_idx = 0;
    Node* bst_root = bst_build(keys, 0, actual_N - 1);
    printf("  BST: %.0f MB (pool)\n\n", actual_N * sizeof(Node) / 1e6);

    volatile int64_t sink = 0;

    // warmup
    { int64_t s=0; for (int32_t q : queries) s += veb.find_opt(q); sink+=s; }
    { int64_t s=0; for (int32_t q : queries) s += veb.find_dyn(q); sink+=s; }
    { int64_t s=0; for (int32_t q : queries) s += bst_find(bst_root, q); sink+=s; }

    vector<double> vo(T), vd(T), bt(T);
    for (int t = 0; t < T; t++) {
        int64_t sv=0, sd=0, sb=0;
        vo[t] = elapsed_ms([&]{ for (int32_t q : queries) sv += veb.find_opt(q); });
        vd[t] = elapsed_ms([&]{ for (int32_t q : queries) sd += veb.find_dyn(q); });
        bt[t] = elapsed_ms([&]{ for (int32_t q : queries) sb += bst_find(bst_root, q); });
        sink += sv + sd + sb;
    }

    auto avg  = [](const vector<double>& v){ return accumulate(v.begin(), v.end(), 0.0) / v.size(); };
    auto minv = [](const vector<double>& v){ return *min_element(v.begin(), v.end()); };

    printf("N=%d  Q=%d  T=%d\n\n", actual_N, Q, T);
    printf("%-12s  %8s  %8s  %14s\n", "Structure", "avg(ms)", "min(ms)", "avg(ns/query)");
    printf("%-12s  %8s  %8s  %14s\n", "------------", "-------", "-------", "-------------");
    printf("%-12s  %8.1f  %8.1f  %14.1f\n", "vEB-opt", avg(vo), minv(vo), avg(vo)*1e6/Q);
    printf("%-12s  %8.1f  %8.1f  %14.1f\n", "vEB-dyn", avg(vd), minv(vd), avg(vd)*1e6/Q);
    printf("%-12s  %8.1f  %8.1f  %14.1f\n", "BST",     avg(bt), minv(bt), avg(bt)*1e6/Q);

    printf("\nPer-run (ms):\n  vEB-opt:");
    for (double x : vo) printf(" %6.1f", x);
    printf("\n  vEB-dyn:");
    for (double x : vd) printf(" %6.1f", x);
    printf("\n  BST:    ");
    for (double x : bt) printf(" %6.1f", x);
    printf("\n\nchecksum: %lld\n", (long long)sink);
}
