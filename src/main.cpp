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

bool is_literal(const char c) {
  // todo - more regex meta characters to add
  return c != '\\' && c != '[' && c != '(' && c != '|';
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

bool is_alternator(const char c) {
  return c == '|';
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

struct backreference_t {
  int number;
  std::optional<quantifier_e> quantifier;
};

struct begin_anchor_t {};
struct end_anchor_t {};

struct capture_group_t {
  // type matches pattern_token_t
  using internal_pattern_t = std::vector<std::vector<std::variant<
    literal_t, digit_t, word_t, positive_character_group_t,
    negative_character_group_t, begin_anchor_t, end_anchor_t, wildcard_t,
    capture_group_t, backreference_t>>>;

  std::unique_ptr<internal_pattern_t> pattern;
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

std::vector<std::vector<pattern_token_t>> parse_pattern(
  const std::string_view pattern) {
  std::vector<std::vector<pattern_token_t>> all_pattern_tokens;
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
          } else if (pattern[i] == ')') {
            if (nesting_depth == 0) {
              sub_pattern = pattern.substr(offset, size);
              capture_group.pattern =
                std::make_unique<std::vector<std::vector<pattern_token_t>>>(
                  parse_pattern(sub_pattern));
              break;
            } else {
              nesting_depth--;
            }
          }
        }
        p += sub_pattern.size() + 2;
        pattern_tokens.push_back(std::move(capture_group));
      } else if (is_alternator(pattern[p])) {
        all_pattern_tokens.push_back(std::move(pattern_tokens));
        pattern_tokens.clear();
        p++;
      }
    }
  }
  if (anchored_end) {
    pattern_tokens.push_back(end_anchor_t{});
  }
  if (!pattern_tokens.empty()) {
    all_pattern_tokens.push_back(std::move(pattern_tokens));
  }
  return all_pattern_tokens;
}

struct match_result_t {
  int start;
  int move;
};

std::optional<int> match_here(
  const std::string_view input, const int input_pos,
  std::span<pattern_token_t> pattern, const int pattern_pos,
  const uint32_t anchors, std::span<capture_group_t*> captured_groups);

std::optional<int> do_match(
  std::span<pattern_token_t> pattern, const int pattern_pos,
  std::string_view input, const int input_pos, const uint32_t anchors,
  std::span<capture_group_t*> captured_groups) {
  if (input_pos >= input.size()) {
    return std::nullopt;
  }
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
  if (auto* literal = std::get_if<literal_t>(&token)) {
    if (literal->l == c) {
      return 1;
    }
    return std::nullopt;
  } else if (auto* digit = std::get_if<digit_t>(&token)) {
    if (std::isdigit(static_cast<unsigned char>(c))) {
      return 1;
    }
    return std::nullopt;
  } else if (auto* word = std::get_if<word_t>(&token)) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
      return 1;
    }
    return std::nullopt;
  } else if (auto* neg = std::get_if<negative_character_group_t>(&token)) {
    if (neg->group.find(c) == std::string::npos) {
      return 1;
    }
    return std::nullopt;
  } else if (auto* pos = std::get_if<positive_character_group_t>(&token)) {
    if (pos->group.find(c) != std::string::npos) {
      return 1;
    }
    return std::nullopt;
  } else if (auto* capture = std::get_if<capture_group_t>(&token)) {
    for (auto& pattern : *capture->pattern) {
      if (
        auto move = match_here(
          input, input_pos, pattern, 0, anchors_for_subpattern(),
          captured_groups)) {
        capture->match = std::string(
          input.begin() + input_pos, input.begin() + input_pos + *move);
        return *move;
      }
    }
    return std::nullopt;
  } else if (std::get_if<wildcard_t>(&token)) {
    return 1;
  } else if (auto* backreference = std::get_if<backreference_t>(&token)) {
    int capture_group_index = backreference->number - 1;
    if (capture_group_index < static_cast<int>(captured_groups.size())) {
      auto match = parse_pattern(captured_groups[capture_group_index]->match);
      for (auto& pattern : match) {
        if (
          auto move = match_here(
            input, input_pos, pattern, 0, anchors_for_subpattern(),
            captured_groups)) {
          return *move;
        }
      }
    }
    return std::nullopt;
  }
  // begin_anchor_t or end_anchor_t
  return std::nullopt;
}

std::optional<int> match_here(
  const std::string_view input, const int input_pos,
  std::span<pattern_token_t> pattern, const int pattern_pos,
  const uint32_t anchors, std::span<capture_group_t*> captured_groups) {
  // base case
  if (pattern_pos == pattern.size()) {
    if ((anchors & anchor_e::end) != 0) {
      return input_pos == input.size() ? std::make_optional(0) : std::nullopt;
    }
    return 0;
  }
  // base case
  const auto quantifier = get_quantifier(pattern[pattern_pos]);
  if (input_pos == input.size()) {
    return quantifier == quantifier_e::zero_or_one ? std::make_optional(0)
                                                   : std::nullopt;
  }
  std::optional<int> next_opt;
  const auto move_opt =
    do_match(pattern, pattern_pos, input, input_pos, anchors, captured_groups);
  if (!move_opt) {
    if (quantifier == quantifier_e::zero_or_one) {
      next_opt = match_here(
        input, input_pos, pattern, pattern_pos + 1, anchors, captured_groups);
      if (!next_opt) {
        return std::nullopt;
      }
    } else {
      return std::nullopt;
    }
  }
  auto move = *move_opt;
  if (quantifier == quantifier_e::one_or_more) {
    next_opt = match_here(
      input, input_pos + move, pattern, pattern_pos, anchors, captured_groups);
  }
  if (!next_opt) {
    next_opt = match_here(
      input, input_pos + move, pattern, pattern_pos + 1, anchors,
      captured_groups);
    if (!next_opt) {
      while (move > 1) {
        move--;
        // backtrack, walking backwards
        next_opt = match_here(
          input, input_pos + move, pattern, pattern_pos + 1, anchors,
          captured_groups);
        if (next_opt) {
          break;
        }
      }
    }
  }
  if (!next_opt) {
    return std::nullopt;
  }
  const auto next = *next_opt;
  return next + move;
}

std::optional<match_result_t> matcher(
  std::string_view input, std::vector<std::vector<pattern_token_t>>& patterns,
  std::span<capture_group_t*> captured_groups) {
  if (input.empty()) {
    return match_result_t{.start = 0, .move = 0};
  }
  if (patterns.empty()) {
    return std::nullopt;
  }
  uint32_t anchors = 0;
  auto& first_pattern = patterns.front();
  if (first_pattern.empty()) {
    return std::nullopt;
  }
  if (std::holds_alternative<begin_anchor_t>(first_pattern.front())) {
    anchors |= anchor_e::begin;
  }
  auto& last_pattern = patterns.back();
  if (last_pattern.empty()) {
    return std::nullopt;
  }
  if (std::holds_alternative<end_anchor_t>(last_pattern.back())) {
    anchors |= anchor_e::end;
  }
  for (int i = 0; i < input.size(); i++) {
    for (int p = 0; auto& pattern : patterns) {
      std::span<pattern_token_t> pattern_span = pattern;
      if ((anchors & anchor_e::begin) != 0) {
        pattern_span = pattern_span | std::views::drop(1);
      }
      if ((anchors & anchor_e::end) != 0) {
        pattern_span = pattern_span | std::views::take(pattern_span.size() - 1);
      }
      if (
        auto result =
          match_here(input, i, pattern_span, 0, anchors, captured_groups)) {
        return match_result_t{.start = i, .move = *result};
      } else if ((anchors & anchor_e::begin) != 0) {
        goto end;
      }
      p++;
    }
  }
end:
  return std::nullopt;
}

std::vector<capture_group_t*> get_capture_groups(
  std::vector<std::vector<pattern_token_t>>& parsed_pattern) {
  return std::accumulate(
    parsed_pattern.begin(), parsed_pattern.end(),
    std::vector<capture_group_t*>{},
    [](
      std::vector<capture_group_t*> acc,
      std::vector<pattern_token_t>& pattern_tokens) {
      for (auto& pattern_token : pattern_tokens) {
        if (
          auto* capture_group = std::get_if<capture_group_t>(&pattern_token)) {
          auto sub_capture_groups = get_capture_groups(*capture_group->pattern);
          acc.push_back(capture_group);
          acc.insert(
            acc.end(), sub_capture_groups.begin(), sub_capture_groups.end());
        }
      }
      return acc;
    });
}

int main(int argc, char* argv[]) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

#if 0
  {
    auto parsed_pattern =
      // parse_pattern("(([abc]+)-([def]+)) is \\1, not ([^xyz])+, \\2, or
      // \\3");
      parse_pattern(
        "(([abc]+)-([def]+)) is \\1, not ([^xyz]+), \\2, or \\3 or \\4");
    auto capture_groups = get_capture_groups(parsed_pattern);
    auto res = matcher(
      "abc-def is abc-def, not efg, abc, or def or efg", parsed_pattern,
      capture_groups);
    if (res) {
      std::cout << res->move << '\n';
    }
    int test;
    test = 0;
  }
#endif

#if 0
  {
    const auto input = std::string("not efg, abc, or def");
    auto parsed_pattern = parse_pattern("not ([^xyz]+),");
    const auto input = std::string("cat");
    auto parsed_pattern = parse_pattern("ca?t");
    const auto input = std::string("I see 2 dog3");
    auto parsed_pattern = parse_pattern("^I see \\d+ (cat|dog)s?$");
    const auto input = std::string("n ab,");
    auto parsed_pattern = parse_pattern("n ([^xyz]+),");
    const auto input = std::string("I see 1 cat");
    auto parsed_pattern = parse_pattern("^I see \\d+ (cat|dog)s?$");

    auto capture_groups = get_capture_groups(parsed_pattern);
    auto res = matcher(input, parsed_pattern, capture_groups);
    if (res) {
      // std::cerr << input.substr(res->start, res->move) << '\n';
    }
    int test;
    test = 0;
  }
#endif

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

#if 0
capture_group_t() = default;
capture_group_t(const capture_group_t& capture_group)
  : pattern(
      capture_group.pattern
        ? std::make_unique<internal_pattern_t>(*capture_group.pattern)
        : nullptr),
    quantifier(capture_group.quantifier), match(capture_group.match) {
}
capture_group_t& operator=(const capture_group_t& capture_group) {
  if (this != &capture_group) {
    capture_group_t temp(capture_group);
    pattern.swap(temp.pattern);
    match.swap(temp.match);
    quantifier.swap(temp.quantifier);
  }
  return *this;
}

capture_group_t(capture_group_t&&) noexcept = default;
capture_group_t& operator=(capture_group_t&&) noexcept = default;
#endif
