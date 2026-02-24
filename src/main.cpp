#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

template<class T>
void hash_combine(std::size_t& seed, const T& v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct pair_hash_t {
  size_t operator()(const std::pair<uint64_t, uint64_t>& p) const {
    size_t seed = 0;
    hash_combine(seed, p.first);
    hash_combine(seed, p.second);
    return seed;
  }
};

struct pair_equals_t {
  size_t operator()(
    const std::pair<uint64_t, uint64_t>& lhs,
    const std::pair<uint64_t, uint64_t>& rhs) const {
    return lhs.first == rhs.first && lhs.second == rhs.second;
  }
};

bool is_literal(const char c) {
  // todo - more regex meta characters to add
  return c != '\\' && c != '[' && c != '(';
}

bool anchored_at_beginning(const char c) {
  return c == '^';
}

bool anchored_at_end(const std::string_view subpattern) {
  assert(subpattern.size() == 2);
  return subpattern[0] != '\\' && subpattern[1] == '$';
}

bool is_escape(const char c) {
  return c == '\\';
}

bool is_character_opener(const char c) {
  return c == '[';
}

bool is_capture_group_opener(const char c) {
  return c == '(';
}

bool is_negating(const std::string_view subpattern) {
  assert(subpattern.size() == 2);
  return subpattern[0] == '[' && subpattern[1] == '^';
}

bool is_digit_character_class(const char c) {
  return c == 'd';
}

bool is_word_character_class(const char c) {
  return c == 'w';
}

bool is_number(unsigned char c) {
  return std::isdigit(c);
}

enum class quantifier_e { one_or_more, zero_or_one };

struct literal_t {
  std::optional<quantifier_e> quantifier;
  char l;
};

struct digit_t {
  std::optional<quantifier_e> quantifier;
};

struct word_t {
  std::optional<quantifier_e> quantifier;
};

struct positive_character_group_t {
  std::string group;
  std::optional<quantifier_e> quantifier;
};

struct negative_character_group_t {
  std::string group;
  std::optional<quantifier_e> quantifier;
};

struct wildcard_t {
  std::optional<quantifier_e> quantifier;
};

struct alternation_t {};

struct backreference_t {
  int number;
  std::optional<quantifier_e> quantifier;
};

struct begin_anchor_t {};
struct end_anchor_t {};

struct capture_group_t {
  // type matches pattern_token_t
  std::unique_ptr<std::vector<std::variant<
    literal_t, digit_t, word_t, positive_character_group_t,
    negative_character_group_t, begin_anchor_t, end_anchor_t, wildcard_t,
    capture_group_t, backreference_t>>>
    pattern;
  std::string match;

  std::optional<quantifier_e> quantifier;
};

struct anchor_e {
  enum { begin = 1 << 0, end = 1 << 1 };
};

using pattern_token_t = std::variant<
  literal_t, digit_t, word_t, positive_character_group_t,
  negative_character_group_t, begin_anchor_t, end_anchor_t, wildcard_t,
  capture_group_t, backreference_t>;

void set_quantifier(pattern_token_t& token, const quantifier_e quantifier) {
  if (auto* p = std::get_if<literal_t>(&token)) {
    p->quantifier = quantifier;
  } else if (auto* p = std::get_if<digit_t>(&token)) {
    p->quantifier = quantifier;
  } else if (auto* p = std::get_if<word_t>(&token)) {
    p->quantifier = quantifier;
  } else if (auto* p = std::get_if<negative_character_group_t>(&token)) {
    p->quantifier = quantifier;
  } else if (auto* p = std::get_if<positive_character_group_t>(&token)) {
    p->quantifier = quantifier;
  } else if (auto* p = std::get_if<capture_group_t>(&token)) {
    p->quantifier = quantifier;
  } else if (auto* p = std::get_if<wildcard_t>(&token)) {
    p->quantifier = quantifier;
  } else if (auto* p = std::get_if<backreference_t>(&token)) {
    p->quantifier = quantifier;
  } else if (std::holds_alternative<begin_anchor_t>(token)) {
    // noop
  } else if (std::holds_alternative<end_anchor_t>(token)) {
    // noop
  }
}

std::optional<quantifier_e> get_quantifier(const pattern_token_t& token) {
  if (auto* p = std::get_if<literal_t>(&token)) {
    return p->quantifier;
  } else if (auto* p = std::get_if<digit_t>(&token)) {
    return p->quantifier;
  } else if (auto* p = std::get_if<word_t>(&token)) {
    return p->quantifier;
  } else if (auto* p = std::get_if<negative_character_group_t>(&token)) {
    return p->quantifier;
  } else if (auto* p = std::get_if<positive_character_group_t>(&token)) {
    return p->quantifier;
  } else if (auto* p = std::get_if<capture_group_t>(&token)) {
    return p->quantifier;
  } else if (auto* p = std::get_if<wildcard_t>(&token)) {
    return p->quantifier;
  } else if (auto* p = std::get_if<backreference_t>(&token)) {
    return p->quantifier;
  }
  // begin_anchor_t or end_anchor_t
  return std::nullopt;
}

std::vector<pattern_token_t> parse_pattern(const std::string_view pattern) {
  std::vector<pattern_token_t> pattern_tokens;
  const bool anchored_beginning = anchored_at_beginning(pattern.front());
  if (anchored_beginning) {
    pattern_tokens.push_back(begin_anchor_t{});
  }
  const bool anchored_end =
    pattern.size() >= 2 ? anchored_at_end(pattern.substr(pattern.size() - 2, 2))
                        : false;
  for (int p = anchored_beginning ? 1 : 0;
       p < (anchored_end ? pattern.size() - 1 : pattern.size());) {
    if (is_literal(pattern[p])) {
      if (pattern[p] == '+') {
        set_quantifier(pattern_tokens.back(), quantifier_e::one_or_more);
        p++;
      } else if (pattern[p] == '?') {
        set_quantifier(pattern_tokens.back(), quantifier_e::zero_or_one);
        p++;
      } else if (pattern[p] == '.') {
        pattern_tokens.push_back(wildcard_t{});
        p++;
      } else {
        pattern_tokens.push_back(literal_t{.l = pattern[p++]});
      }
    } else {
      if (is_escape(pattern[p])) {
        if (is_digit_character_class(pattern[p + 1])) {
          pattern_tokens.push_back(digit_t{});
          p += 2;
        } else if (is_word_character_class(pattern[p + 1])) {
          pattern_tokens.push_back(word_t{});
          p += 2;
        } else if (pattern[p + 1] == '\\') {
          pattern_tokens.push_back(literal_t{.l = '\\'});
          p += 2;
        } else if (is_number(pattern[p + 1])) {
          const auto number_start = pattern.begin() + p + 1;
          const auto number_end =
            std::find_if_not(number_start, pattern.end(), is_number);
          const auto number =
            std::stoi(std::string(std::string_view(number_start, number_end)));
          pattern_tokens.push_back(backreference_t{.number = number});
          p += 1 + std::distance(number_start, number_end);
        }
      } else if (is_character_opener(pattern[p])) {
        if (is_negating(pattern.substr(p, 2))) {
          const auto offset = p + 2;
          const auto end = pattern.find(']', offset);
          const auto characters = pattern.substr(offset, end - offset);
          pattern_tokens.push_back(
            negative_character_group_t{.group = std::string(characters)});
          p += characters.size() + 3;
        } else {
          const auto offset = p + 1;
          const auto end = pattern.find(']', offset);
          const auto characters = pattern.substr(offset, end - offset);
          pattern_tokens.push_back(
            positive_character_group_t{.group = std::string(characters)});
          p += characters.size() + 2;
        }
      } else if (is_capture_group_opener(pattern[p])) {
        const auto offset = p + 1;
        capture_group_t capture_group;
        std::string sub_pattern;
        int nesting_depth = 0;
        for (int i = offset, size = 0;; size++, i++) {
          if (pattern[i] == '(') {
            nesting_depth++;
          }
          if (pattern[i] == ')') {
            if (nesting_depth == 0) {
              sub_pattern = pattern.substr(offset, size);
              capture_group.pattern =
                std::make_unique<std::vector<pattern_token_t>>(
                  parse_pattern(sub_pattern));
              break;
            } else {
              nesting_depth--;
            }
          }
        }
        p += sub_pattern.size() + 2;
        pattern_tokens.push_back(std::move(capture_group));
      }
    }
  }
  if (anchored_end) {
    pattern_tokens.push_back(end_anchor_t{});
  }
  return pattern_tokens;
}

struct match_result_t {
  int start;
  int move;
};

using cache_t = std::unordered_map<
  std::pair<int, int>, std::optional<match_result_t>, pair_hash_t,
  pair_equals_t>;

// forward declaration
std::optional<match_result_t> matcher_internal(
  std::string_view input, const int input_pos,
  std::span<pattern_token_t> pattern, const int pattern_pos,
  const uint32_t anchors, std::span<const capture_group_t*> capture_groups,
  cache_t& cache);

std::optional<match_result_t> do_match(
  std::span<pattern_token_t> pattern, const int pattern_pos,
  std::string_view input, const int input_pos, const uint32_t anchors,
  std::span<const capture_group_t*> captured_groups) {
  const char c = input[input_pos];
  const auto anchors_for_subpattern = [&pattern, pattern_pos, anchors]() {
    if (pattern_pos == 0) {
      return ((anchors & anchor_e::begin) != 0) ? anchor_e::begin : 0;
    }
    if (pattern_pos == pattern.size() - 1) {
      return ((anchors & anchor_e::end) != 0) ? anchor_e::end : 0;
    }
    return 0;
  };
  auto& token = pattern[pattern_pos];
  if (auto* l = std::get_if<literal_t>(&token)) {
    if (l->l == c) {
      return std::make_optional(match_result_t{.start = input_pos, .move = 1});
    }
    return std::nullopt;
  } else if (auto* digit = std::get_if<digit_t>(&token)) {
    if (std::isdigit(static_cast<unsigned char>(c))) {
      return std::make_optional(match_result_t{.start = input_pos, .move = 1});
    }
    return std::nullopt;
  } else if (auto* word = std::get_if<word_t>(&token)) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
      return std::make_optional(match_result_t{.start = input_pos, .move = 1});
    }
    return std::nullopt;
  } else if (auto* neg = std::get_if<negative_character_group_t>(&token)) {
    if (neg->group.find(c) == std::string::npos) {
      return std::make_optional(match_result_t{.start = input_pos, .move = 1});
    }
    return std::nullopt;
  } else if (auto* pos = std::get_if<positive_character_group_t>(&token)) {
    if (pos->group.find(c) != std::string::npos) {
      return std::make_optional(match_result_t{.start = input_pos, .move = 1});
    }
    return std::nullopt;
  } else if (auto* capture = std::get_if<capture_group_t>(&token)) {
    cache_t cache;
    if (
      auto next_match = matcher_internal(
        input, input_pos, *capture->pattern, 0, anchors_for_subpattern(),
        captured_groups, cache)) {
      capture->match = std::string(
        input.begin() + next_match->start,
        input.begin() + input_pos + next_match->move);
      return next_match;
    }
    return std::nullopt;
  } else if (std::get_if<wildcard_t>(&token)) {
    return std::make_optional(match_result_t{.start = input_pos, .move = 1});
  } else if (auto* backreference = std::get_if<backreference_t>(&token)) {
    cache_t cache;
    int capture_group_index = backreference->number - 1;
    if (capture_group_index < static_cast<int>(captured_groups.size())) {
      auto match = parse_pattern(captured_groups[capture_group_index]->match);
      if (
        auto next_match = matcher_internal(
          input, input_pos, match, 0, anchors_for_subpattern(), captured_groups,
          cache)) {
        return next_match;
      }
    }
    return std::nullopt;
  }
  // begin_anchor_t or end_anchor_t
  return std::nullopt;
}

std::optional<int> do_match_2(
  std::span<pattern_token_t> pattern, const int pattern_pos,
  std::string_view input, const int input_pos, const uint32_t anchors,
  std::span<const capture_group_t*> captured_groups) {
  return do_match(
           pattern, pattern_pos, input, input_pos, anchors, captured_groups)
    .transform([](const auto match_result) { return match_result.move; });
}

std::optional<int> match_here(
  const std::string_view input, const int input_pos,
  std::span<pattern_token_t> pattern, const int pattern_pos,
  const uint32_t anchors) {
  // base case
  if (pattern_pos == pattern.size()) {
    return 0;
  }
  // base case
  if (input_pos == input.size()) {
    return std::nullopt;
  }
  auto move = do_match_2(pattern, pattern_pos, input, input_pos, anchors, {});
  if (!move) {
    return std::nullopt;
  }
  std::optional<int> next =
    match_here(input, input_pos + *move, pattern, pattern_pos + 1, anchors);
  if (!next) {
    return std::nullopt;
  }
  return *next + *move;
}

std::optional<match_result_t> matcher_internal(
  const std::string_view input, const int input_pos,
  std::span<pattern_token_t> pattern, const int pattern_pos,
  const uint32_t anchors, std::span<const capture_group_t*> captured_groups,
  cache_t& cache) {
  if (pattern_pos == pattern.size()) {
    if ((anchors & anchor_e::end) != 0) {
      return input_pos == input.size()
             ? std::make_optional(match_result_t{.start = input_pos, .move = 0})
             : std::optional<match_result_t>(std::nullopt);
    } else {
      return std::make_optional(match_result_t{.start = input_pos, .move = 0});
    }
  }
  const auto quantifier = get_quantifier(pattern[pattern_pos]);
  if (input_pos == input.size()) {
    return quantifier == quantifier_e::zero_or_one
           ? std::make_optional(match_result_t{.start = input_pos, .move = 0})
           : std::optional<match_result_t>(std::nullopt);
  }
  if (auto it = cache.find({input_pos, pattern_pos}); it != cache.end()) {
    return it->second;
  }
  const auto result =
    do_match(pattern, pattern_pos, input, input_pos, anchors, captured_groups);
  if (result.has_value()) {
    const match_result_t& match_result = *result;
    if (quantifier == quantifier_e::one_or_more) {
      // try to match more of the pattern (greedy)
      auto greedy_match = matcher_internal(
        input, input_pos + 1, pattern, pattern_pos, anchors, captured_groups,
        cache);
      if (greedy_match.has_value()) {
        greedy_match->start = std::min(match_result.start, greedy_match->start);
        greedy_match->move += 1;
        return greedy_match;
      }
    }
    auto match = matcher_internal(
      input, input_pos + match_result.move, pattern, pattern_pos + 1, anchors,
      captured_groups, cache);
    if (match.has_value()) {
      match->start = std::min(match_result.start, match->start);
      match->move += match_result.move;
      return match;
    } else {
      auto next_match = matcher_internal(
        input, input_pos + match_result.move, pattern, 0, anchors,
        captured_groups, cache);
      if (next_match.has_value()) {
        next_match->start = std::min(match_result.start, next_match->start);
        next_match->move += match_result.move;
        return next_match;
      } else {
        return std::nullopt;
      }
    }
  } else {
    if (quantifier == quantifier_e::one_or_more) {
      // no match at current position
      return std::optional<match_result_t>(std::nullopt);
    } else if (quantifier == quantifier_e::zero_or_one) {
      auto match = matcher_internal(
        input, input_pos, pattern, pattern_pos + 1, anchors, captured_groups,
        cache);
      if (match.has_value()) {
        match->move += 1;
        return match;
      } else {
        return std::nullopt;
      }
    }
    // backtrack
    if ((anchors & anchor_e::begin) == 0) {
      auto match = matcher_internal(
        input, input_pos + 1, pattern, 0, anchors, captured_groups, cache);
      if (match.has_value()) {
        match->move += 1;
        return match;
      } else {
        return std::nullopt;
      }
    } else {
      return std::optional<match_result_t>(std::nullopt);
    }
  }
  cache.insert({{input_pos, pattern_pos}, result});
  return result;
}

std::optional<match_result_t> matcher(
  std::string_view input, std::span<pattern_token_t> pattern,
  std::span<const capture_group_t*> captured_groups) {
  uint32_t anchors = 0;
  if (std::holds_alternative<begin_anchor_t>(pattern.front())) {
    pattern = pattern | std::views::drop(1);
    anchors |= anchor_e::begin;
  }
  if (std::holds_alternative<end_anchor_t>(pattern.back())) {
    pattern = pattern | std::views::take(pattern.size() - 1);
    anchors |= anchor_e::end;
  }
  for (int i = 0; i < input.size(); i++) {
    auto result = match_here(input, i, pattern, 0, anchors);
    if (result) {
      return match_result_t{.start = i, .move = *result};
    } else if ((anchors & anchor_e::begin) != 0) {
      break;
    }
  }
  return std::nullopt;
}

std::vector<const capture_group_t*> get_capture_groups(
  std::span<const pattern_token_t> parsed_pattern) {
  return std::accumulate(
    parsed_pattern.begin(), parsed_pattern.end(),
    std::vector<const capture_group_t*>{},
    [](
      std::vector<const capture_group_t*> acc,
      const pattern_token_t& pattern_token) {
      if (std::holds_alternative<capture_group_t>(pattern_token)) {
        auto* capture_group = std::get_if<capture_group_t>(&pattern_token);
        auto sub_capture_groups = get_capture_groups(*capture_group->pattern);
        acc.push_back(capture_group);
        acc.insert(
          acc.end(), sub_capture_groups.begin(), sub_capture_groups.end());
      }
      return acc;
    });
}

int main(int argc, char* argv[]) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // {
  //   auto parsed_pattern =
  //     // parse_pattern("(([abc]+)-([def]+)) is \\1, not ([^xyz])+, \\2, or
  //     // \\3");
  //     parse_pattern(
  //       "(([abc]+)-([def]+)) is \\1, not ([^xyz]+), \\2, or \\3 or \\4");
  //   auto capture_groups = get_capture_groups(parsed_pattern);
  //   auto res = matcher(
  //     "abc-def is abc-def, not efg, abc, or def or efg", parsed_pattern,
  //     capture_groups);
  //   if (res) {
  //     std::cout << res->move << '\n';
  //   }
  //   int test;
  //   test = 0;
  // }

  // {
  //   auto parsed_pattern = parse_pattern("not ([^xyz]+),");
  //   auto capture_groups = get_capture_groups(parsed_pattern);
  //   auto res = matcher("not efg, abc, or def", parsed_pattern,
  //   capture_groups); if (res) {
  //     std::cout << res->move << '\n';
  //   }
  //   int test;
  //   test = 0;
  // }

  {
    const std::string input = "orange_pear";
    auto parsed_pattern = parse_pattern("^orange");
    auto capture_groups = get_capture_groups(parsed_pattern);
    auto res = matcher(input, parsed_pattern, capture_groups);
    // if (res) {
    //   std::cerr << input.substr(res->start, res->move) << '\n';
    // }
    int test;
    test = 0;
  }

  if (argc != 3) {
    std::cerr << "Expected two arguments" << std::endl;
    return 1;
  }

  std::string flag = argv[1];
  std::string pattern = argv[2];

  if (flag != "-E") {
    std::cerr << "Expected first argument to be '-E'" << std::endl;
    return 1;
  }

  std::string input;
  std::getline(std::cin, input);

  try {
    auto parsed_pattern = parse_pattern(pattern);
    auto capture_groups = get_capture_groups(parsed_pattern);
    if (auto match = matcher(input, parsed_pattern, capture_groups)) {
      // debug output matching part of string
      // std::cerr << input_line.substr(match->start, match->move -
      // match->start)
      //           << '\n';
      // std::cerr << input.substr(match->start, match->move) << '\n';
      return 0;
    } else {
      return 1;
    }
  } catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
