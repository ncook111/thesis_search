#ifndef FST_H
#define FST_H

#include "custom_types.h"
#include "helper.h"
#include "pef.h"
#include <fst/fstlib.h>
#include <optional>

std::unique_ptr<fst::StdConstFst> read_from_disk(const std::string &file_name);

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
void get_paths(const G &fst, const int state, std::string &current,
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

template <class G> std::set<std::string> get_strings_from_fst(const G &fst) {
  std::set<std::string> results;
  if (fst.Start() == fst::kNoStateId)
    return results;
  std::string current;
  get_paths(fst, fst.Start(), current, results);
  return results;
}

template <class G>
std::vector<std::string> get_vec_strings_from_fst(const G &fst) {
  const std::set<std::string> results = get_strings_from_fst(fst);
  const std::vector<std::string> results_vec(results.begin(), results.end());
  return results_vec;
}

template <class G, QueryType Q>
int get_id_from_fst(const G &fst, const Q &query) {
  long state = fst.Start();
  long word_id = -1;

  if (state == fst::kNoStateId)
    return -1;

  for (uint8_t c : query) {
    const int64_t label = c + 1;
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
    if (!found)
      return -1;
  }

  return (fst.Final(state) != fst::TropicalWeight::Zero()) ? word_id : -1;
}

template <class G>
void get_paths(const G &fst, const int state, std::vector<uint8_t> &current,
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

template <class Q, class G> std::set<Q> get_sequences_from_fst(const G &fst) {
  std::set<Q> results;
  if (fst.Start() == fst::kNoStateId)
    return {};
  Q current;
  get_paths(fst, fst.Start(), current, results);
  return results;
}

template <class G>
int get_id_from_fst(const G &fst, const std::vector<uint8_t> &query) {
  if (fst.Start() == fst::kNoStateId)
    return -1;

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
    if (!found)
      return -1;
  }

  return (fst.Final(state) != fst::TropicalWeight::Zero()) ? word_id : -1;
}

template <class G>
int64_t get_bound_from_fst(const G &fst, const float val, const bool bound) {
  std::set<std::vector<uint8_t>> all_float =
      get_sequences_from_fst<std::vector<uint8_t>>(fst);
  std::vector<float> float_vec;

  for (const auto &fl : all_float) {
    float_vec.push_back(from_bytes<float>(fl));
  }

  std::ranges::sort(float_vec);

  const auto it = std::ranges::lower_bound(float_vec, val);

  if (it == float_vec.end()) {
    return float_vec.size() - 1;
  } else {
    if (!bound && it != float_vec.begin())
      return get_id_from_fst(fst, to_bytes(*(it - 1)));
    else
      return get_id_from_fst(fst, to_bytes(*it));
  }
}

template <class G>
const std::optional<Range>
get_id_range_from_fst(const G &fst, const std::pair<float, float> &range) {
  const auto [min, max] = range;
  int64_t min_id, max_id;

  if (min == -std::numeric_limits<float>::infinity())
    min_id = 0;
  else
    min_id = get_id_from_fst(fst, to_bytes(min));

  if (max == std::numeric_limits<float>::infinity())
    max_id = get_sequences_from_fst<std::vector<uint8_t>>(fst).size() - 1;
  else
    max_id = get_id_from_fst(fst, to_bytes(max));

  if (min_id == -1) {
    min_id = get_bound_from_fst(fst, min, true);
  }

  if (max_id == -1) {
    max_id = get_bound_from_fst(fst, max, false);
  }
  Range ret = {min_id, max_id};

  return std::make_optional(Range(min_id, max_id));
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
  std::cout << "read in filename: " << file_name << std::endl;
  std::unique_ptr<fst::StdConstFst> fst = read_from_disk(file_name);

  std::set<M> set;

  if constexpr (std::is_same_v<M, std::string>) {
    set = get_strings_from_fst(*fst);
  } else {
    std::set<std::vector<uint8_t>> vals =
        get_sequences_from_fst<std::vector<uint8_t>>(*fst);

    for (const auto &v : vals) {
      set.insert(from_bytes<M>(v));
    }
  }

  std::vector<M> out;

  for (const auto i : set) {
    out.push_back(i);
  }

  std::vector<M> tokens_sorted = tokens;

  std::sort(tokens_sorted.begin(), tokens_sorted.end());

  if (out == tokens_sorted) {
    std::cout << "correct" << std::endl;
  } else {
    std::cout << "incorrect" << std::endl;
  }

  return out == tokens;
}

template <class G>
const std::string id_to_string(const G &fst, const uint64_t id) {
  const std::vector<std::string> strings = get_vec_strings_from_fst(fst);

  return strings[id];
}

#endif