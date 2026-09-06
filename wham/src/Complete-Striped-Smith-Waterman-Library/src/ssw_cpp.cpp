// Portable pure-C++ implementation of the StripedSmithWaterman C++ API.
// The original ssw_cpp.cpp delegated to ssw.h, which uses x86 SSE intrinsics
// and cannot compile on Apple Silicon (arm64). This replacement implements the
// same public API (Aligner/Filter/Alignment) with a classic affine-gap
// Smith-Waterman so it builds and runs on any architecture. Sequences here are
// short (hundreds of bp), so the O(mn) DP is fine.
// ponytail: single-best alignment only; sw_score_next_best / ref_end_next_best
// are reported as 0 (the WHAM callers only use sw_score, ref_begin, query_begin,
// mismatches, and cigar). Upgrade path: restore the SSE/NEON SSW core if
// second-best reporting or speed becomes important.
#include "ssw_cpp.h"

#include <cstring>
#include <sstream>
#include <vector>

namespace {

static const int8_t kBaseTranslation[128] = {
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
    4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
  //   A     C            G
    4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4,
  //             T
    4, 4, 4, 4,  3, 0, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
  //   a     c            g
    4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4,
  //             t
    4, 4, 4, 4,  3, 0, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4
};

void BuildSwScoreMatrix(const uint8_t& match_score,
                        const uint8_t& mismatch_penalty,
                        int8_t* matrix) {
  //                 // A,  C,  G,  T,  N
  //  score_matrix_ = { 2, -2, -2, -2,  0, // A
  //                   -2,  2, -2, -2,  0, // C
  //                   -2, -2,  2, -2,  0, // G
  //                   -2, -2, -2,  2,  0, // T
  //                    0,  0,  0,  0,  0};// N
  int id = 0;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      matrix[id] = ((i == j) ? match_score
                             : static_cast<int8_t>(-mismatch_penalty));
      ++id;
    }
    matrix[id] = 0;
    ++id;
  }
  for (int i = 0; i < 5; ++i)
    matrix[id++] = 0;
}

static inline uint32_t to_cigar_int(uint32_t length, char op_letter) {
  uint8_t op_code;
  switch (op_letter) {
    case 'M':
    default:
      op_code = 0;
      break;
    case 'I':
      op_code = 1;
      break;
    case 'D':
      op_code = 2;
      break;
    case 'N':
      op_code = 3;
      break;
    case 'S':
      op_code = 4;
      break;
    case 'H':
      op_code = 5;
      break;
    case 'P':
      op_code = 6;
      break;
    case '=':
      op_code = 7;
      break;
    case 'X':
      op_code = 8;
      break;
  }
  return (length << 4) | op_code;
}

// Traceback op codes.
enum { TB_DIAG = 0, TB_UP = 1, TB_LEFT = 2 };

// Run affine-gap Smith-Waterman. query/ref are translated (0..4) sequences.
// Fills *alignment with the best local alignment (score, boundaries, cigar).
// mismatches is set to the number of mismatched/indels in the alignment.
static void RunSmithWaterman(const int8_t* query, int qlen,
                             const int8_t* ref, int rlen,
                             const int8_t* score_matrix,
                             int score_matrix_size,
                             int gap_open, int gap_extend,
                             StripedSmithWaterman::Alignment* alignment) {
  const int m = qlen;
  const int n = rlen;
  const int NEG_INF = -1000000;

  // H: best score ending at (i,j) with no open gap.
  // E: best score with an open gap in the query (insertion to ref).
  // F: best score with an open gap in the ref (deletion from ref).
  std::vector<short> H((m + 1) * (n + 1), 0);
  std::vector<short> E((m + 1) * (n + 1), 0);
  std::vector<short> F((m + 1) * (n + 1), 0);
  std::vector<uint8_t> tb((m + 1) * (n + 1), TB_DIAG);

  int best = 0, bi = 0, bj = 0;
  for (int i = 1; i <= m; ++i) {
    for (int j = 1; j <= n; ++j) {
      int s = score_matrix[query[i - 1] * score_matrix_size + ref[j - 1]];
      int h00 = H[i * (n + 1) + (j - 1)];
      int h10 = H[(i - 1) * (n + 1) + (j - 1)];
      int h01 = H[i * (n + 1) + j];
      int e10 = E[(i - 1) * (n + 1) + (j - 1)];
      int f01 = F[i * (n + 1) + (j - 1)];

      int e = (e10 - gap_extend) > (h00 - gap_open) ? (e10 - gap_extend)
                                                    : (h00 - gap_open);
      int f = (f01 - gap_extend) > (h01 - gap_open) ? (f01 - gap_extend)
                                                    : (h01 - gap_open);
      int diag = h10 + s;
      int h = diag;
      uint8_t op = TB_DIAG;
      if (e > h) { h = e; op = TB_UP; }
      if (f > h) { h = f; op = TB_LEFT; }
      if (h < 0) { h = 0; op = TB_DIAG; }

      E[i * (n + 1) + j] = e;
      F[i * (n + 1) + j] = f;
      H[i * (n + 1) + j] = (short)h;
      tb[i * (n + 1) + j] = op;

      if (h > best) { best = h; bi = i; bj = j; }
    }
  }

  // Trace back from the optimal cell to the start of the local alignment.
  std::vector<uint8_t> path;
  int i = bi, j = bj;
  while (i > 0 && j > 0 && H[i * (n + 1) + j] > 0) {
    path.push_back(tb[i * (n + 1) + j]);
    if (tb[i * (n + 1) + j] == TB_UP)      --i;
    else if (tb[i * (n + 1) + j] == TB_LEFT) --j;
    else { --i; --j; }
  }
  int qi = i + 1; // query 1-based start
  int ri = j + 1; // ref   1-based start
  std::reverse(path.begin(), path.end());

  // Build cigar (M/I/D) and count mismatches by comparing translated bases.
  std::ostringstream cigar_string;
  std::vector<uint32_t> cigar;
  int q = qi - 1;
  int r = ri - 1;
  int mismatches = 0;
  for (size_t k = 0; k < path.size(); ++k) {
    uint8_t op = path[k];
    if (op == TB_DIAG) {
      if (query[q] != ref[r]) ++mismatches;
      cigar.push_back(to_cigar_int(1, 'M'));
      cigar_string << "1M";
      ++q; ++r;
    } else if (op == TB_UP) {
      cigar.push_back(to_cigar_int(1, 'I'));
      cigar_string << "1I";
      ++mismatches;
      ++q;
    } else { // TB_LEFT
      cigar.push_back(to_cigar_int(1, 'D'));
      cigar_string << "1D";
      ++mismatches;
      ++r;
    }
  }

  int qend = q; // 1-based query end
  int rend = r; // 1-based ref end

  // Prepend/append soft clips so the cigar spans the full query length.
  std::ostringstream full;
  std::vector<uint32_t> full_cigar;
  if (qi > 1) {
    int clip = qi - 1;
    full_cigar.push_back(to_cigar_int(clip, 'S'));
    full << clip << "S";
  }
  full << cigar_string.str();
  full_cigar.insert(full_cigar.end(), cigar.begin(), cigar.end());
  int end = qlen - qend;
  if (end > 0) {
    full_cigar.push_back(to_cigar_int(end, 'S'));
    full << end << "S";
  }

  alignment->sw_score = (uint16_t)best;
  alignment->sw_score_next_best = 0;
  alignment->ref_begin = ri;
  alignment->ref_end = rend;
  alignment->query_begin = qi;
  alignment->query_end = qend;
  alignment->ref_end_next_best = 0;
  alignment->mismatches = mismatches;
  alignment->cigar_string = full.str();
  alignment->cigar = full_cigar;
}

} // namespace

namespace StripedSmithWaterman {

Aligner::Aligner(void)
    : score_matrix_(NULL)
    , score_matrix_size_(5)
    , translation_matrix_(NULL)
    , match_score_(2)
    , mismatch_penalty_(2)
    , gap_opening_penalty_(3)
    , gap_extending_penalty_(1)
    , translated_reference_(NULL)
    , reference_length_(0)
{
  BuildDefaultMatrix();
}

Aligner::Aligner(
    const uint8_t& match_score,
    const uint8_t& mismatch_penalty,
    const uint8_t& gap_opening_penalty,
    const uint8_t& gap_extending_penalty)
    : score_matrix_(NULL)
    , score_matrix_size_(5)
    , translation_matrix_(NULL)
    , match_score_(match_score)
    , mismatch_penalty_(mismatch_penalty)
    , gap_opening_penalty_(gap_opening_penalty)
    , gap_extending_penalty_(gap_extending_penalty)
    , translated_reference_(NULL)
    , reference_length_(0)
{
  BuildDefaultMatrix();
}

Aligner::Aligner(const int8_t* score_matrix,
                 const int&    score_matrix_size,
                 const int8_t* translation_matrix,
                 const int&    translation_matrix_size)
    : score_matrix_(NULL)
    , score_matrix_size_(score_matrix_size)
    , translation_matrix_(NULL)
    , match_score_(2)
    , mismatch_penalty_(2)
    , gap_opening_penalty_(3)
    , gap_extending_penalty_(1)
    , translated_reference_(NULL)
    , reference_length_(0)
{
  score_matrix_ = new int8_t[score_matrix_size_ * score_matrix_size_];
  memcpy(score_matrix_, score_matrix,
         sizeof(int8_t) * score_matrix_size_ * score_matrix_size_);
  translation_matrix_ = new int8_t[translation_matrix_size];
  memcpy(translation_matrix_, translation_matrix,
         sizeof(int8_t) * translation_matrix_size);
}

Aligner::~Aligner(void) {
  Clear();
}

int Aligner::SetReferenceSequence(const char* seq, const int& length) {
  int len = 0;
  CleanReferenceSequence();
  translated_reference_ = new int8_t[length];
  len = TranslateBase(seq, length, translated_reference_);
  reference_length_ = len;
  return len;
}

int Aligner::TranslateBase(const char* bases, const int& length,
                           int8_t* translated) const {
  const char* ptr = bases;
  int len = 0;
  for (int i = 0; i < length; ++i) {
    translated[i] = translation_matrix_[(int)*ptr];
    ++ptr;
    ++len;
  }
  return len;
}

bool Aligner::Align(const char* query, const Filter& filter,
                    Alignment* alignment) const {
  if (reference_length_ == 0) return false;

  int query_len = (int)strlen(query);
  int8_t* translated_query = new int8_t[query_len];
  TranslateBase(query, query_len, translated_query);

  alignment->Clear();
  RunSmithWaterman(translated_query, query_len, translated_reference_,
                   reference_length_, score_matrix_, score_matrix_size_,
                   gap_opening_penalty_, gap_extending_penalty_, alignment);

  delete[] translated_query;
  return true;
}

bool Aligner::Align(const char* query, const char* ref, const int& ref_len,
                    const Filter& filter, Alignment* alignment) const {
  int query_len = (int)strlen(query);
  int8_t* translated_query = new int8_t[query_len];
  int8_t* translated_ref = new int8_t[ref_len];
  TranslateBase(query, query_len, translated_query);
  TranslateBase(ref, ref_len, translated_ref);

  alignment->Clear();
  RunSmithWaterman(translated_query, query_len, translated_ref, ref_len,
                   score_matrix_, score_matrix_size_, gap_opening_penalty_,
                   gap_extending_penalty_, alignment);

  delete[] translated_query;
  delete[] translated_ref;
  return true;
}

void Aligner::Clear(void) {
  ClearMatrices();
  CleanReferenceSequence();
}

void Aligner::SetAllDefault(void) {
  score_matrix_size_     = 5;
  match_score_           = 2;
  mismatch_penalty_      = 2;
  gap_opening_penalty_   = 3;
  gap_extending_penalty_ = 1;
  reference_length_      = 0;
}

bool Aligner::ReBuild(void) {
  if (translation_matrix_) return false;
  SetAllDefault();
  BuildDefaultMatrix();
  return true;
}

bool Aligner::ReBuild(const uint8_t& match_score,
                      const uint8_t& mismatch_penalty,
                      const uint8_t& gap_opening_penalty,
                      const uint8_t& gap_extending_penalty) {
  if (translation_matrix_) return false;
  SetAllDefault();
  match_score_           = match_score;
  mismatch_penalty_      = mismatch_penalty;
  gap_opening_penalty_   = gap_opening_penalty;
  gap_extending_penalty_ = gap_extending_penalty;
  BuildDefaultMatrix();
  return true;
}

bool Aligner::ReBuild(const int8_t* score_matrix,
                      const int&    score_matrix_size,
                      const int8_t* translation_matrix,
                      const int&    translation_matrix_size) {
  ClearMatrices();
  score_matrix_size_ = score_matrix_size;
  score_matrix_ = new int8_t[score_matrix_size_ * score_matrix_size_];
  memcpy(score_matrix_, score_matrix,
         sizeof(int8_t) * score_matrix_size_ * score_matrix_size_);
  translation_matrix_ = new int8_t[translation_matrix_size];
  memcpy(translation_matrix_, translation_matrix,
         sizeof(int8_t) * translation_matrix_size);
  return true;
}

void Aligner::BuildDefaultMatrix(void) {
  ClearMatrices();
  score_matrix_ = new int8_t[score_matrix_size_ * score_matrix_size_];
  BuildSwScoreMatrix(match_score_, mismatch_penalty_, score_matrix_);
  translation_matrix_ = new int8_t[128];
  memcpy(translation_matrix_, kBaseTranslation, sizeof(int8_t) * 128);
}

void Aligner::ClearMatrices(void) {
  delete[] score_matrix_;
  score_matrix_ = NULL;
  delete[] translation_matrix_;
  translation_matrix_ = NULL;
}

} // namespace StripedSmithWaterman
