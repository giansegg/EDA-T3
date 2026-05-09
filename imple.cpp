#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <random>
#include <vector>
using namespace std;

struct Node { int32_t key; Node *l, *r; };

static vector<Node> pool;
static int ptr = 0;

Node* build_bst(const vector<int32_t>& v, int lo, int hi) {
    if (lo > hi) return nullptr;
    int m = lo + (hi-lo)/2;
    Node* n = &pool[ptr++];
    n->key = v[m];
    n->l = build_bst(v, lo, m-1);
    n->r = build_bst(v, m+1, hi);
    return n;
}

bool find_bst(const Node* r, int32_t k) {
    while (r) {
        if (k == r->key) return true;
        r = k < r->key ? r->l : r->r;
    }
    return false;
}

int build_veb(int32_t* out, const int32_t* s, int h) {
    if (h == 0) return 0;
    if (h == 1) { *out = *s; return 1; }
    int th = h/2, bh = h-th, step = 1<<bh;
    vector<int32_t> top((1<<th)-1);
    for (int j = 0; j < (int)top.size(); j++)
        top[j] = s[(size_t)j*step + step-1];
    int p = build_veb(out, top.data(), th);
    for (int k = 0; k < (1<<th); k++)
        p += build_veb(out+p, s+(size_t)k*step, bh);
    return p;
}

bool veb_dyn(const int32_t* a, int h, int32_t k, int& g) {
    if (h == 0) { g = 0; return false; }
    if (h == 1) { g = (k >= *a); return *a == k; }
    int th = h/2, bh = h-th, j, q;
    if (veb_dyn(a, th, k, j)) return true;
    bool f = veb_dyn(a + (1<<th)-1 + (size_t)j*((1<<bh)-1), bh, k, q);
    g = (j<<bh)|q;
    return f;
}

template<int H>
[[gnu::always_inline]] bool find_fast(const int32_t* a, int32_t k, int& g) {
    if constexpr (H == 0) { g = 0; return false; }
    else if constexpr (H == 1) { g = (k >= *a); return *a == k; }
    else {
        constexpr int th = H/2, bh = H-th;
        int j, q;
        if (find_fast<th>(a, k, j)) return true;
        bool f = find_fast<bh>(a + (1<<th)-1 + (size_t)j*((1<<bh)-1), k, q);
        g = (j<<bh)|q;
        return f;
    }
}

struct VEBTree {
    vector<int32_t> data;
    int h = 0;

    void build(vector<int32_t> keys) {
        while ((1<<h)-1 < (int)keys.size()) h++;
        keys.resize((size_t)(1<<h)-1, INT32_MAX);
        data.resize(keys.size());
        build_veb(data.data(), keys.data(), h);
    }

    bool find_opt(int32_t k) const {
        int g;
        switch (h) {
            case  1: return find_fast< 1>(data.data(), k, g);
            case  2: return find_fast< 2>(data.data(), k, g);
            case  3: return find_fast< 3>(data.data(), k, g);
            case  4: return find_fast< 4>(data.data(), k, g);
            case  5: return find_fast< 5>(data.data(), k, g);
            case  6: return find_fast< 6>(data.data(), k, g);
            case  7: return find_fast< 7>(data.data(), k, g);
            case  8: return find_fast< 8>(data.data(), k, g);
            case  9: return find_fast< 9>(data.data(), k, g);
            case 10: return find_fast<10>(data.data(), k, g);
            case 11: return find_fast<11>(data.data(), k, g);
            case 12: return find_fast<12>(data.data(), k, g);
            case 13: return find_fast<13>(data.data(), k, g);
            case 14: return find_fast<14>(data.data(), k, g);
            case 15: return find_fast<15>(data.data(), k, g);
            case 16: return find_fast<16>(data.data(), k, g);
            case 17: return find_fast<17>(data.data(), k, g);
            case 18: return find_fast<18>(data.data(), k, g);
            case 19: return find_fast<19>(data.data(), k, g);
            case 20: return find_fast<20>(data.data(), k, g);
            case 21: return find_fast<21>(data.data(), k, g);
            case 22: return find_fast<22>(data.data(), k, g);
            case 23: return find_fast<23>(data.data(), k, g);
            case 24: return find_fast<24>(data.data(), k, g);
            case 25: return find_fast<25>(data.data(), k, g);
            default: return veb_dyn(data.data(), h, k, g);
        }
    }

    bool find_slow(int32_t k) const {
        int g;
        return veb_dyn(data.data(), h, k, g);
    }
};

template<typename Fn>
double elapsed_ms(Fn fn) {
    auto t0 = chrono::high_resolution_clock::now();
    fn();
    return chrono::duration<double, milli>(chrono::high_resolution_clock::now() - t0).count();
}

int main() {
    const int N = 20000000, Q = 1000000, T = 7;

    mt19937 rng(42);
    uniform_int_distribution<int32_t> dist(1, 10*N);

    vector<int32_t> keys(N);
    for (auto& k : keys) k = dist(rng);
    sort(keys.begin(), keys.end());
    keys.erase(unique(keys.begin(), keys.end()), keys.end());
    int actual_N = keys.size();

    vector<int32_t> queries(Q);
    for (auto& q : queries) q = dist(rng);

    printf("Building (N=%d)...\n", actual_N);
    VEBTree veb;
    veb.build(keys);
    printf("  vEB: h=%d, %.0f MB\n", veb.h, veb.data.size()*4.0/1e6);

    pool.resize(actual_N); ptr = 0;
    Node* root = build_bst(keys, 0, actual_N-1);
    printf("  BST: %.0f MB (pool)\n\n", actual_N*sizeof(Node)/1e6);

    volatile int64_t sink = 0;
    { int64_t s=0; for (int32_t q : queries) s += veb.find_opt(q); sink+=s; }
    { int64_t s=0; for (int32_t q : queries) s += veb.find_slow(q); sink+=s; }
    { int64_t s=0; for (int32_t q : queries) s += find_bst(root, q); sink+=s; }

    vector<double> vo(T), vd(T), bt(T);
    for (int t = 0; t < T; t++) {
        int64_t sv=0, sd=0, sb=0;
        vo[t] = elapsed_ms([&]{ for (int32_t q : queries) sv += veb.find_opt(q); });
        vd[t] = elapsed_ms([&]{ for (int32_t q : queries) sd += veb.find_slow(q); });
        bt[t] = elapsed_ms([&]{ for (int32_t q : queries) sb += find_bst(root, q); });
        sink += sv+sd+sb;
    }

    auto avg  = [](const vector<double>& v){ return accumulate(v.begin(), v.end(), 0.0)/v.size(); };
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
