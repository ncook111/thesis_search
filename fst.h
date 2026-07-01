#ifndef FST_H
#define FST_H

#include <fst/fstlib.h>
#include "helper.h"
#include "custom_types.h"

void read_from_disk(fst::StdConstFst *&fst, const std::string &file_name);

template <FstQueryType Q>
fst::StdVectorFst create_trie(const std::vector<Q> &sequences) {
    fst::StdVectorFst fst;
    fst.AddState();
    fst.SetStart(0);

    const uint32_t sequences_size = static_cast<uint32_t>(sequences.size());

    for (uint32_t i = 0; i < sequences_size; ++i) {
        const Q &seq = sequences[i];
        const uint32_t seq_size = static_cast<uint32_t>(seq.size());
        long state = 0;

        for (uint32_t j = 0; j < seq_size; ++j) {
            const int64_t label = static_cast<uint8_t>(seq[j]) + 1;
            const int64_t olabel = (j == seq_size - 1) ? i + 1 : 0;

            long next = -1;
            if (j < seq_size - 1) {
                for (fst::ArcIterator<fst::StdVectorFst> aiter(fst, state);
                     !aiter.Done(); aiter.Next()) {
                    if (aiter.Value().ilabel == label) {
                        next = aiter.Value().nextstate;
                        break;
                    }
                }
            }

            if (next == -1) {
                next = fst.AddState();
                fst.AddArc(state, fst::StdArc(label, olabel, 0.0, next));
            }

            state = next;
        }

        fst.SetFinal(state, fst::TropicalWeight::One());
    }

    return fst;
}

template <class G>
void get_paths(const G &fst, int state, std::string &current,
               std::set<std::string> &results) {
    if (fst.Final(state) != fst::TropicalWeight::Zero())
        results.insert(current);

    for (fst::ArcIterator<G> aiter(fst, state); !aiter.Done(); aiter.Next()) {
        const fst::StdArc &arc = aiter.Value();
        current.push_back(static_cast<char>(arc.ilabel - 1));
        get_paths(fst, arc.nextstate, current, results);
        current.pop_back();
    }
}

template <class F, class G>
std::set<F> get_strings_from_fst(const G &fst) {
    std::set<F> results;
    if (fst.Start() == fst::kNoStateId) return results;
    F current;
    get_paths(fst, fst.Start(), current, results);
    return results;
}

template <class G, QueryType Q>
int get_id_from_fst(const G &fst, const Q &query) {
    long state = fst.Start();
    long word_id = -1;

    for (uint8_t c : query) {
        const int64_t label = static_cast<uint8_t>(c) + 1;
        bool found = false;
        for (fst::ArcIterator<G> aiter(fst, state); !aiter.Done(); aiter.Next()) {
            const auto &arc = aiter.Value();
            if (arc.ilabel == label) {
                if (arc.olabel != 0)
                    word_id = arc.olabel - 1;
                state = arc.nextstate;
                found = true;
                break;
            }
        }
        if (!found) return -1;
    }

    return (fst.Final(state) != fst::TropicalWeight::Zero()) ? word_id : -1;
}

template <class G>
void get_paths(const G &fst, int state, std::vector<uint8_t> &current,
               std::set<std::vector<uint8_t>> &results) {
    if (fst.Final(state) != fst::TropicalWeight::Zero())
        results.insert(current);

    for (fst::ArcIterator<G> aiter(fst, state); !aiter.Done(); aiter.Next()) {
        const fst::StdArc &arc = aiter.Value();
        current.push_back(static_cast<uint8_t>(arc.ilabel - 1));
        get_paths(fst, arc.nextstate, current, results);
        current.pop_back();
    }
}

template <class Q, class G>
std::set<Q> get_sequences_from_fst(const G &fst) {
    std::set<Q> results;
    if (fst.Start() == fst::kNoStateId) return {};
    Q current;
    get_paths(fst, fst.Start(), current, results);
    return results;
}

template <class G>
int get_id_from_fst(const G &fst, const std::vector<uint8_t> &query) {
    if (fst.Start() == fst::kNoStateId) return -1;

    long state = fst.Start();
    long word_id = -1;

    for (uint8_t b : query) {
        const int64_t label = static_cast<int64_t>(b) + 1;
        bool found = false;
        for (fst::ArcIterator<G> aiter(fst, state); !aiter.Done(); aiter.Next()) {
            const auto &arc = aiter.Value();
            if (arc.ilabel == label) {
                if (arc.olabel != 0)
                    word_id = arc.olabel - 1;
                state = arc.nextstate;
                found = true;
                break;
            }
        }
        if (!found) return -1;
    }

    return (fst.Final(state) != fst::TropicalWeight::Zero()) ? word_id : -1;
}

template <FstQueryType Q> 
void create_fst(const std::vector<Q> &tokens, const std::string &file_name) {
    fst::StdVectorFst trie = create_trie(tokens);

    fst::RmEpsilon(&trie);
    fst::StdVectorFst det;
    fst::Determinize(trie, &det);
    fst::ArcSort(&det, fst::ILabelCompare<fst::StdArc>());
    fst::Minimize(&det);
    fst::StdConstFst compact(det);

    compact.Write(file_name);
}

template <MapType M>
bool check_fst(const std::vector<M> &tokens, const std::string &file_name) {
        fst::StdConstFst* fst = nullptr;
        std::cout << "read in filename: " << file_name << std::endl;
        read_from_disk(fst, file_name);

        std::set<M> set;

        if constexpr (std::is_same_v<M, std::string>) {
            set = get_strings_from_fst<M>(*fst);
        } else {
            std::set<std::vector<uint8_t>> vals = get_sequences_from_fst<std::vector<uint8_t>>(*fst);

            for (auto &v : vals) {
                set.insert(from_bytes<M>(v));
            }
        }

        std::vector<M> out;

        for (auto i : set) {
            out.push_back(i);
        }

        if (out == tokens) {
            std::cout << "correct" << std::endl;
        } else {
            std::cout << "incorrect" << std::endl;
        }

        return out == tokens;
}

#endif