#include <vector>

// https://algorithmica.org/en/eytzinger
class Eytzinger {
    public:
        Eytzinger(const std::vector<uint64_t> &in)
        : a(in), n(a.size()), b(n + 1) {
            eytzinger(0, 1);

        }

        const uint64_t search(const uint64_t x) const {
            uint64_t k = 1;
            while (k <= n) {
                k = 2 * k + (b[k] < x);
            }
            k >>= __builtin_ffs(~k);
            return k;
        }

        const uint64_t size() const {
            return b.size();
        }

        const uint64_t at(uint64_t i) const {
            return b[i];
        }
    private:
        const std::vector<uint64_t> &a;
        const uint64_t n;
        std::vector<uint64_t> b;

        const uint64_t eytzinger(uint64_t i, const uint64_t k) {
            if (k <= n) {
                i = eytzinger(i, 2 * k);
                b[k] = a[i++];
                i = eytzinger(i, 2 * k + 1);
            }
            return i;
        }
};

