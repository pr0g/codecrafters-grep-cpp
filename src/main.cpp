#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <variant>
#include <vector>

template<class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
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

struct capture_group_t {
  std::string matched;
  std::string pattern;
  std::optional<quantifier_e> quantifier;
};

struct alternation_t {};

struct backreference_t {
  int number;
  std::optional<quantifier_e> quantifier;
};

struct begin_anchor_t {};
struct end_anchor_t {};

struct anchor_e {
  enum { begin = 1 << 0, end = 1 << 1 };
};

using pattern_token_t = std::variant<
  literal_t, digit_t, word_t, positive_character_group_t,
  negative_character_group_t, begin_anchor_t, end_anchor_t, wildcard_t,
  capture_group_t, backreference_t>;

void set_quantifier(pattern_token_t& pattern_token, quantifier_e quantifier) {
  std::visit(
    overloaded{
      [&](literal_t& literal) { literal.quantifier = quantifier; },
      [&](digit_t& digit) { digit.quantifier = quantifier; },
      [&](word_t& word) { word.quantifier = quantifier; },
      [&](negative_character_group_t& negative_character_group) {
        negative_character_group.quantifier = quantifier;
      },
      [&](positive_character_group_t& positive_character_group) {
        positive_character_group.quantifier = quantifier;
      },
      [&](capture_group_t& capture_group) {
        capture_group.quantifier = quantifier;
      },
      [&](wildcard_t& wildcard) { wildcard.quantifier = quantifier; },
      [&](backreference_t& backreference) {
        backreference.quantifier = quantifier;
      },
      [&](begin_anchor_t& begin_anchor) { /* noop */ },
      [&](end_anchor_t& end_anchor) { /* noop */ },
    },
    pattern_token);
}

std::optional<quantifier_e> get_quantifier(
  const pattern_token_t& pattern_token) {
  return std::visit(
    overloaded{
      [&](const literal_t& literal) { return literal.quantifier; },
      [&](const digit_t& digit) { return digit.quantifier; },
      [&](const word_t& word) { return word.quantifier; },
      [&](const negative_character_group_t& negative_character_group) {
        return negative_character_group.quantifier;
      },
      [&](const positive_character_group_t& positive_character_group) {
        return positive_character_group.quantifier;
      },
      [&](const capture_group_t& capture_group) {
        return capture_group.quantifier;
      },
      [&](const wildcard_t& wildcard) { return wildcard.quantifier; },
      [&](const backreference_t& backreference) {
        return backreference.quantifier;
      },
      [&](const begin_anchor_t& begin_anchor) {
        return std::optional<quantifier_e>(std::nullopt);
      },
      [&](const end_anchor_t& end_anchor) {
        return std::optional<quantifier_e>(std::nullopt);
      }},
    pattern_token);
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
        int nesting_depth = 0;
        for (int i = offset, size = 0;; size++, i++) {
          if (pattern[i] == '(') {
            nesting_depth++;
          }
          if (pattern[i] == ')') {
            if (nesting_depth == 0) {
              capture_group.pattern = pattern.substr(offset, size);
              break;
            } else {
              nesting_depth--;
            }
          }
        }
        p += capture_group.pattern.size() + 2;
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

// forward declaration
std::optional<match_result_t> matcher_internal(
  std::string_view input, const int input_pos,
  std::span<pattern_token_t> pattern, const int pattern_pos,
  const uint32_t anchors, std::span<capture_group_t*> capture_groups);

std::optional<match_result_t> do_match(
  std::span<pattern_token_t> pattern, const int pattern_pos,
  std::string_view input, const int input_pos, const uint32_t anchors,
  std::span<capture_group_t*> capture_groups) {
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
  return std::visit(
    overloaded{
      [&](const literal_t& l) {
        return l.l == c ? std::make_optional(
                            match_result_t{.start = input_pos, .move = 1})
                        : std::optional<match_result_t>(std::nullopt);
      },
      [&](const digit_t& digit) {
        return std::isdigit(c)
               ? std::make_optional(
                   match_result_t{.start = input_pos, .move = 1})
               : std::optional<match_result_t>(std::nullopt);
      },
      [&](const word_t& word) {
        return std::isalnum(c) || c == '_'
               ? std::make_optional(
                   match_result_t{.start = input_pos, .move = 1})
               : std::optional<match_result_t>(std::nullopt);
      },
      [&](const negative_character_group_t& negative_character_group) {
        return negative_character_group.group.find(c) == std::string::npos
               ? std::make_optional(
                   match_result_t{.start = input_pos, .move = 1})
               : std::optional<match_result_t>(std::nullopt);
      },
      [&](const positive_character_group_t& positive_character_group) {
        return positive_character_group.group.find(c) != std::string::npos
               ? std::make_optional(
                   match_result_t{.start = input_pos, .move = 1})
               : std::optional<match_result_t>(std::nullopt);
      },
      [&](capture_group_t& capture_group) {
        auto pattern = parse_pattern(capture_group.pattern);
        if (
          auto next_match = matcher_internal(
            input, input_pos, pattern, 0, anchors_for_subpattern(),
            capture_groups)) {
          capture_group.matched = std::string(
            input.begin() + next_match->start,
            input.begin() + input_pos + next_match->move);
          return next_match;
        }
        return std::optional<match_result_t>(std::nullopt);
      },
      [&](const wildcard_t& wildcard) {
        return std::make_optional(
          match_result_t{.start = input_pos, .move = 1});
      },
      [&](const backreference_t& backreference) {
        if (int capture_group_index = backreference.number - 1;
            capture_group_index < capture_groups.size()) {
          const auto& word = capture_groups[capture_group_index]->matched;
          auto pattern = parse_pattern(word);
          if (
            auto next_match = matcher_internal(
              input, input_pos, pattern, 0, anchors_for_subpattern(),
              capture_groups)) {
            return next_match;
          }
        }
        return std::optional<match_result_t>(std::nullopt);
      },
      [&](const begin_anchor_t& begin_anchor) {
        return std::optional<match_result_t>(std::nullopt);
      },
      [&](const end_anchor_t& end_anchor) {
        return std::optional<match_result_t>(std::nullopt);
      }},
    pattern[pattern_pos]);
}

std::optional<match_result_t> matcher_internal(
  const std::string_view input, const int input_pos,
  std::span<pattern_token_t> pattern, const int pattern_pos,
  const uint32_t anchors, std::span<capture_group_t*> capture_groups) {
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
  return do_match(
           pattern, pattern_pos, input, input_pos, anchors, capture_groups)
    .and_then([&](match_result_t match_result) {
      if (quantifier == quantifier_e::one_or_more) {
        // try to match more of the pattern (greedy)
        if (
          const auto match = matcher_internal(
                               input, input_pos + 1, pattern, pattern_pos,
                               anchors, capture_groups)
                               .transform([&](match_result_t match) {
                                 match.move += 1;
                                 return match;
                               })) {
          return match;
        }
      }
      return matcher_internal(
               input, input_pos + match_result.move, pattern, pattern_pos + 1,
               anchors, capture_groups)
        .transform([&](match_result_t match) {
          match.start = std::min(match_result.start, match.start);
          match.move += match_result.move;
          return match;
        })
        .or_else([&] {
          // backtrack
          return matcher_internal(
                   input, input_pos + match_result.move, pattern, 0, anchors,
                   capture_groups)
            .transform([&](match_result_t match) {
              match.move += match_result.move;
              return match;
            });
        });
    })
    .or_else([&] {
      if (quantifier == quantifier_e::one_or_more) {
        // no match at current position
        return std::optional<match_result_t>(std::nullopt);
      } else if (quantifier == quantifier_e::zero_or_one) {
        return matcher_internal(
                 input, input_pos, pattern, pattern_pos + 1, anchors,
                 capture_groups)
          .transform([&](match_result_t match) {
            match.move += 1;
            return match;
          });
      }
      // backtrack
      if ((anchors & anchor_e::begin) == 0) {
        return matcher_internal(
                 input, input_pos + 1, pattern, 0, anchors, capture_groups)
          .transform([&](match_result_t match) {
            match.move += 1;
            return match;
          });
      } else {
        return std::optional<match_result_t>(std::nullopt);
      }
    });
}

std::optional<match_result_t> matcher(
  std::string_view input, std::span<pattern_token_t> pattern,
  std::span<capture_group_t*> capture_groups) {
  uint32_t anchors = 0;
  auto p = pattern;
  if (std::holds_alternative<begin_anchor_t>(pattern.front())) {
    p = p | std::views::drop(1);
    anchors |= anchor_e::begin;
  }
  if (std::holds_alternative<end_anchor_t>(pattern.back())) {
    p = p | std::views::take(p.size() - 1);
    anchors |= anchor_e::end;
  }
  return matcher_internal(input, 0, p, 0, anchors, capture_groups);
}

std::vector<capture_group_t*> get_capture_groups(
  std::span<pattern_token_t> parsed_pattern) {
  return std::accumulate(
    parsed_pattern.begin(), parsed_pattern.end(),
    std::vector<capture_group_t*>{},
    [](std::vector<capture_group_t*> acc, pattern_token_t& pattern_token) {
      if (std::holds_alternative<capture_group_t>(pattern_token)) {
        auto* capture_group = std::get_if<capture_group_t>(&pattern_token);
        auto sub_parsed_pattern = parse_pattern(capture_group->pattern);
        auto sub_capture_groups = get_capture_groups(sub_parsed_pattern);
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

  {
    auto parsed_pattern = parse_pattern("('(cat) and \\2') is the same as \\1");
    auto capture_groups = get_capture_groups(parsed_pattern);
    auto res = matcher(
      "'cat and cat' is the same as 'cat and cat'", parsed_pattern,
      capture_groups);
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

  std::string input_line;
  std::getline(std::cin, input_line);

  try {
    auto parsed_pattern = parse_pattern(pattern);
    auto capture_groups = get_capture_groups(parsed_pattern);
    if (auto match = matcher(input_line, parsed_pattern, capture_groups)) {
      // debug output matching part of string
      // std::cerr << input_line.substr(match->start, match->move -
      // match->start)
      //           << '\n';
      return 0;
    } else {
      return 1;
    }
  } catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
