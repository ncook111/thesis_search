#include <vector>

int eytzinger(uint64_t i, uint64_t k, const std::vector<uint64_t> &a, std::vector<uint64_t> &b, uint64_t n) {
    if (k <= n) {
        i = eytzinger(i, 2 * k, a, b, n);
        b[k] = a[i++];
        i = eytzinger(i, 2 * k + 1, a, b, n);
    }
    return i;
}

std::vector<uint64_t> make_eytzinger(const std::vector<uint64_t> &a) {
    uint64_t n = a.size();
    std::vector<uint64_t> b(n+1);
    eytzinger(0, 1, a, b, n);

    return b;
}

uint64_t eytzinger_search(uint64_t x, const std::vector<uint64_t> &b) {
    uint64_t n = b.size();
    uint64_t k = 1;
    while (k <= n) {
        k = 2 * k + (b[k] < x);
    }
    k >>= __builtin_ffs(~k);
    return k;
}