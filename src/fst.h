#ifndef FST_H
#define FST_H

#include "custom_types.h"
#include "helper.h"
#include "pef.h"
#include "index.h"
#include <fst/vector-fst.h>
#include <fst/minimize.h>
#include <fst/arcsort.h>
#include <fst/matcher.h>
#include <fst/compact-fst.h>
#include <optional>

using StdCompactFst = fst::CompactUnweightedFst<fst::StdArc>;

struct Fst_header {
  uint64_t alpha_pos;
  uint64_t float_pos;
  uint64_t fields_pos;
};

std::unique_ptr<StdCompactFst> read_from_disk(const std::string &file_name, const size_t read_pos = 0);
size_t write_to_disk(const StdCompactFst &fst, std::ofstream &file);
Fst_header get_header_from_fst(const std::string &file_name);

template <class G, class S>
void get_paths(const G &fst, const int state, S &current,
               std::vector<S> &results) {
  if (fst.Final(state) != fst::TropicalWeight::Zero())
    results.push_back(current);

  for (fst::ArcIterator<G> aiter(fst, state); !aiter.Done(); aiter.Next()) {
    const fst::StdArc &arc = aiter.Value();
    current.push_back(static_cast<typename S::value_type>(arc.ilabel - 1));
    get_paths(fst, arc.nextstate, current, results);
    current.pop_back();
  }
}

template <FstQueryType Q>
fst::StdVectorFst create_trie(const std::vector<Q> &sequences) {
  fst::StdVectorFst f;
  f.SetStart(f.AddState());

  for (uint32_t i = 0; i < sequences.size(); ++i) {
    const Q &seq = sequences[i];
    fst::StdArc::StateId s = 0;
    for (size_t j = 0; j < seq.size(); ++j) {
      const int label = static_cast<uint8_t>(seq[j]) + 1;
      const bool last = (j + 1 == seq.size());
      fst::StdArc::StateId next = fst::kNoStateId;
      for (fst::MutableArcIterator<fst::StdVectorFst> it(&f, s); !it.Done(); it.Next()) {
        fst::StdArc a = it.Value();
        if (a.ilabel == label) {
          if (last) { 
            a.olabel = i + 1; 
            it.SetValue(a); 
          }
          next = a.nextstate;
          break;
        }
      }
      if (next == fst::kNoStateId) {
        next = f.AddState();
        f.AddArc(s, fst::StdArc(label, last ? i + 1 : 0, fst::TropicalWeight::One(), next));
      }
      s = next;
    }
    f.SetFinal(s, fst::TropicalWeight::One());
  }
  return f;
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

template <class Q, class G> std::vector<Q> get_sequences_from_fst(const G &fst) {
  std::vector<Q> results;
  if (fst.Start() == fst::kNoStateId)
    return results;
  Q current;
  get_paths(fst, fst.Start(), current, results);
  return results;
}

template <class G> std::vector<std::string> get_strings_from_fst(const G &fst) {
  return get_sequences_from_fst<std::string>(fst);
}

template <class G>
int64_t get_bound_from_fst(const G &fst, const float val, const bool bound) {
  std::vector<std::vector<uint8_t>> all_float =
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
StdCompactFst create_fst(const std::vector<Q> &tokens) {
  fst::StdVectorFst trie = create_trie(tokens);

  fst::Minimize(&trie);
  fst::ArcSort(&trie, fst::ILabelCompare<fst::StdArc>());
  
  return StdCompactFst(trie);
}

template <MapType M>
bool check_fst(const std::vector<M> &tokens, const std::string &file_name, const uint8_t fst_type) {
  // Read in header
  Fst_header fst_header = get_header_from_fst(file_name);

  size_t read_pos;
  switch (fst_type) {
    case FstType::ALPHA:
      read_pos = fst_header.alpha_pos;
      break;
    case FstType::FLOAT:
      read_pos = fst_header.float_pos;
      break;
    case FstType::FIELDS:
      read_pos = fst_header.fields_pos;
      break;
    case FstType::OTHER:
      read_pos = 0;
      break;
    default:
      throw std::runtime_error("Fst_type not valid: " + std::to_string(fst_type));
  }

  std::cout << "read in fst: " << +fst_type << std::endl;
  std::unique_ptr<StdCompactFst> fst = read_from_disk(file_name, read_pos);

  std::vector<M> out;
  if constexpr (std::is_same_v<M, std::string>) {
    out = get_strings_from_fst(*fst);  // already sorted
  } else {
    for (const auto &v : get_sequences_from_fst<std::vector<uint8_t>>(*fst))
      out.push_back(from_bytes<M>(v));
    std::ranges::sort(out);
  }

  std::vector<M> tokens_sorted = tokens;

  std::sort(tokens_sorted.begin(), tokens_sorted.end());

  if (out == tokens_sorted) {
    std::cout << "correct" << std::endl;
  } else {
    std::cout << "incorrect" << std::endl;
  }

  return out == tokens_sorted;
}

template <class G>
const std::string id_to_string(const G &fst, const uint64_t id) {
  return get_strings_from_fst(fst).at(id);
}

#endif