#include <algorithm>
#include <cassert>
#include <iostream>
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
  return c != '\\' && c != '[';
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

bool is_negating(const std::string_view subpattern) {
  assert(subpattern.size() == 2);
  return subpattern[0] == '[' && subpattern[1] == '^';
}

bool is_digit(const char c) {
  return c == 'd';
}

bool is_word(const char c) {
  return c == 'w';
}

template<typename character_group_fn>
std::tuple<int, int> character_groups_match(
  std::string_view input, int i, std::string_view pattern, int p, int skip,
  const character_group_fn& character_group_matcher) {
  const auto offset = p + skip;
  const auto end = pattern.find(']', offset);
  const auto characters = pattern.substr(offset, end - offset);
  auto result = character_group_matcher(input, characters);
  if (result) {
    return {i + 1, (end - p) + 1};
  } else {
    return {input.size(), p};
  }
}

template<typename character_class_fn>
std::tuple<int, int> character_class_match(
  std::string_view input, int i, int p,
  const character_class_fn& character_class_matcher) {
  if (character_class_matcher(input[i])) {
    return {i + 1, p + 2};
  } else {
    return {i + 1, p};
  }
}

std::tuple<int, int> literal_check(
  std::string_view input, int i, std::string_view pattern, int p,
  bool anchored_beginning) {
  if (input[i] == pattern[p]) {
    return {i + 1, p + 1};
  } else {
    // if there are currently no matches, move to the next character
    // otherwise, reset pattern search, and stay on the current character
    if (anchored_beginning) {
      return std::tuple<int, int>{input.size(), p};
    } else {
      return p == 0 ? std::tuple<int, int>{i + 1, p}
                    : std::tuple<int, int>{i, 0};
    }
  }
}

enum class quantifier_e { one_or_more, zero_or_more };

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

// todo - fix me (remove - handle as literal)
struct backslash_t {};

struct positive_character_group_t {
  std::string group;
  std::optional<quantifier_e> quantifier;
};

struct negative_character_group_t {
  std::string group;
  std::optional<quantifier_e> quantifier;
};

struct begin_anchor_t {};

struct end_anchor_t {};

// struct one_or_more_t {};

using pattern_token_t = std::variant<
  literal_t, digit_t, word_t, positive_character_group_t,
  negative_character_group_t, begin_anchor_t, end_anchor_t, backslash_t>;

void set_quantifier(pattern_token_t pattern_token, quantifier_e quantifier) {
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
      [&](begin_anchor_t& begin_anchor) {}, [&](end_anchor_t& end_anchor) {},
      [&](backslash_t& backslash) {}},
    pattern_token);
}

bool has_quantifier(const pattern_token_t& pattern_token) {
  bool quantifier = false;
  std::visit(
    overloaded{
      [&](const literal_t& literal) {
        quantifier = literal.quantifier.has_value();
      },
      [&](const digit_t& digit) { quantifier = digit.quantifier.has_value(); },
      [&](const word_t& word) { quantifier = word.quantifier.has_value(); },
      [&](const negative_character_group_t& negative_character_group) {
        quantifier = negative_character_group.quantifier.has_value();
      },
      [&](const positive_character_group_t& positive_character_group) {
        quantifier = positive_character_group.quantifier.has_value();
      },
      [&](const begin_anchor_t& begin_anchor) {},
      [&](const end_anchor_t& end_anchor) {},
      [&](const backslash_t& backslash) {}},
    pattern_token);
  return quantifier;
}

std::vector<pattern_token_t> parse_pattern(const std::string_view pattern) {
  std::vector<pattern_token_t> pattern_tokens;
  bool anchored_beginning = anchored_at_beginning(pattern.front());
  if (anchored_beginning) {
    pattern_tokens.push_back(begin_anchor_t{});
  }
  bool anchored_end = pattern.size() >= 2
                      ? anchored_at_end(pattern.substr(pattern.size() - 2, 2))
                      : false;
  for (int p = anchored_beginning ? 1 : 0;
       p < (anchored_end ? pattern.size() - 1 : pattern.size());) {
    if (is_literal(pattern[p])) {
      if (pattern[p] == '+') {
        auto& previous_pattern = pattern_tokens.back();
        set_quantifier(previous_pattern, quantifier_e::one_or_more);
        p++;
      } else {
        pattern_tokens.push_back(literal_t{.l = pattern[p++]});
      }
    } else {
      if (is_escape(pattern[p])) {
        if (is_digit(pattern[p + 1])) {
          pattern_tokens.push_back(digit_t{});
          p += 2;
        } else if (is_word(pattern[p + 1])) {
          pattern_tokens.push_back(word_t{});
          p += 2;
        } else if (pattern[p + 1] == '\\') {
          pattern_tokens.push_back(backslash_t{});
          p += 2;
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
      }
    }
  }
  if (anchored_end) {
    pattern_tokens.push_back(end_anchor_t{});
  }
  return pattern_tokens;
}

std::tuple<int, int> match(
  pattern_token_t pattern_token, int i, std::string_view input_line, int p,
  std::span<pattern_token_t> pattern_tokens, bool anchored_beginning) {
  std::visit(
    overloaded{
      [&](const literal_t& literal) {
        if (input_line[i] == literal.l) {
          i++;
          p++;
        } else {
          // if there are currently no matches, move to the next character
          // otherwise, reset pattern search, and stay on the current
          // character
          if (anchored_beginning) {
            i = input_line.size();
          } else {
            if (p == 0) {
              i++;
            } else {
              p = 0;
            }
          }
        }
      },
      [&](const digit_t& digit) {
        if (std::isdigit(input_line[i])) {
          i++;
          p++;
        } else {
          i++;
        }
      },
      [&](const word_t& word) {
        if (std::isalnum(input_line[i]) || input_line[i] == '_') {
          i++;
          p++;
        } else {
          i++;
        }
      },
      [&](const negative_character_group_t& negative_character_group) {
        if (std::any_of(
              input_line.begin(), input_line.end(), [&](const unsigned char c) {
                return negative_character_group.group.find(c)
                    == std::string::npos;
              })) {
          i++;
          p++;
        } else {
          i = input_line.size();
        }
      },
      [&](const positive_character_group_t& positive_character_group) {
        if (std::any_of(
              positive_character_group.group.begin(),
              positive_character_group.group.end(), [&](const unsigned char c) {
                return input_line.find(c) != std::string::npos;
              })) {
          i++;
          p++;
        } else {
          i = input_line.size();
        }
      },
      [&](const begin_anchor_t& begin_anchor) {},
      [&](const end_anchor_t& end_anchor) {},
      [&](const backslash_t& backslash) {
        i++;
        p++;
      }},
    pattern_token);
  return {i, p};
}

bool match_pattern_2(
  const std::string_view input_line,
  const std::span<pattern_token_t> pattern_tokens) {
  bool anchored_beginning =
    std::holds_alternative<begin_anchor_t>(pattern_tokens.front());
  bool anchored_end =
    std::holds_alternative<end_anchor_t>(pattern_tokens.back());
  int i = 0;
  int p = anchored_beginning ? 1 : 0;
  int pattern_size =
    anchored_end ? pattern_tokens.size() - 1 : pattern_tokens.size();
  while (i < input_line.size() && p < pattern_size) {
    std::tie(i, p) = match(
      pattern_tokens[p], i, input_line, p, pattern_tokens, anchored_beginning);
    if (anchored_end) {
      if (p >= pattern_size && i < input_line.size()) {
        p = anchored_beginning ? 1 : 0;
      }
    }
  }
  if (anchored_end) {
    return p == pattern_size && i == input_line.size();
  } else {
    return p == pattern_size;
  }
}

bool match_pattern(
  const std::string_view input_line, const std::string_view pattern) {
  bool anchored_beginning = anchored_at_beginning(pattern.front());
  bool anchored_end = anchored_at_end(pattern.substr(pattern.size() - 2, 2));
  int i = 0;
  int p = anchored_beginning ? 1 : 0;
  int pattern_size = anchored_end ? pattern.size() - 1 : pattern.size();
  while (i < input_line.size() && p < pattern_size) {
    if (is_literal(pattern[p])) {
      std::tie(i, p) =
        literal_check(input_line, i, pattern, p, anchored_beginning);
    } else {
      if (is_escape(pattern[p])) {
        if (is_digit(pattern[p + 1])) {
          std::tie(i, p) = character_class_match(
            input_line, i, p, [](const char c) { return std::isdigit(c); });
        } else if (is_word(pattern[p + 1])) {
          std::tie(i, p) =
            character_class_match(input_line, i, p, [](const char c) {
              return std::isalnum(c) || c == '_';
            });
        } else if (pattern[p + 1] == '\\') {
          i++;
          p++;
        }
      } else if (is_character_opener(pattern[p])) {
        if (is_negating(pattern.substr(p, 2))) {
          std::tie(i, p) = character_groups_match(
            input_line, i, pattern, p, 2,
            [](std::string_view input, std::string_view characters) {
              return std::any_of(
                input.begin(), input.end(),
                [&characters](const unsigned char c) {
                  return characters.find(c) == std::string::npos;
                });
            });
        } else {
          std::tie(i, p) = character_groups_match(
            input_line, i, pattern, p, 1,
            [](std::string_view input, std::string_view characters) {
              return std::any_of(
                characters.begin(), characters.end(),
                [&input](const unsigned char c) {
                  return input.find(c) != std::string::npos;
                });
            });
        }
      }
    }
    if (anchored_end) {
      if (p >= pattern_size && i < input_line.size()) {
        p = 0;
      }
    }
  }
  if (anchored_end) {
    return p == pattern_size && i == input_line.size();
  } else {
    return p == pattern_size;
  }
}

std::optional<std::string_view::size_type> match_literal(
  literal_t literal, std::string_view input,
  std::string_view::size_type input_pos) {
  if (input[input_pos] == literal.l) {
    return input_pos + 1;
  }
  return std::nullopt;
}

template<typename Range>
auto head_tail(Range&& r) {
  std::span s{r};
  return std::tuple<decltype(s.front()), decltype(s.subspan(1))>(
    s.front(), s.subspan(1));
}

auto head_tail(std::string_view s) {
  return std::pair<decltype(s.front()), decltype(s.substr(1))>(
    s.front(), s.substr(1));
}

bool do_match(
  std::span<pattern_token_t> pattern,
  std::span<pattern_token_t>::size_type pattern_pos, std::string_view input,
  std::string_view::size_type input_pos) {
  bool match = false;
  const char c = input[input_pos];
  std::visit(
    overloaded{
      [&](const literal_t& l) {
        if (l.l == c) {
          match = true;
        }
      },
      [&](const digit_t& digit) {
        if (std::isdigit(c)) {
          match = true;
        }
      },
      [&](const word_t& word) {
        if (std::isalnum(c) || c == '_') {
          match = true;
        }
      },
      [&](const negative_character_group_t& negative_character_group) {
        if (int position = negative_character_group.group.find(c);
            position == std::string::npos) {
          match = true;
        }
      },
      [&](const positive_character_group_t& positive_character_group) {
        if (int position = positive_character_group.group.find(c);
            position != std::string::npos) {
          match = true;
        }
      },
      [&](const begin_anchor_t& begin_anchor) {},
      [&](const end_anchor_t& end_anchor) {},
      [&](const backslash_t& backslash) {
        if (c == '\\') {
          match = true;
        }
      }},
    pattern[pattern_pos]);
  return match;
}

struct anchor_e {
  enum { begin = 1 << 0, end = 1 << 1 };
};

bool matcher_internal(
  std::string_view input, std::string_view::size_type input_pos,
  std::span<pattern_token_t> pattern,
  std::span<pattern_token_t>::size_type pattern_pos, int anchors) {
  if (pattern_pos == pattern.size()) {
    return (anchors & anchor_e::end) != 0 ? input_pos == input.size() : true;
  }
  if (input_pos == input.size()) {
    return false;
  }
  if (do_match(pattern, pattern_pos, input, input_pos)) {
    if (has_quantifier(pattern[pattern_pos])) {
      return matcher_internal(
        input, input_pos + 1, pattern, pattern_pos, anchors);
    }
    return matcher_internal(
      input, input_pos + 1, pattern, pattern_pos + 1, anchors);
  } else {
    if (has_quantifier(pattern[pattern_pos])) {
      return matcher_internal(
        input, input_pos, pattern, pattern_pos + 1, anchors);
    }
    if ((anchors & anchor_e::begin) != 1) {
      return matcher_internal(
        input, input_pos + 1, pattern, pattern_pos, anchors);
    } else {
      return input_pos != 0;
    }
  }
}

bool matcher(std::string_view input, std::span<pattern_token_t> pattern) {
  int anchors = 0;
  auto p = pattern;
  if (std::holds_alternative<begin_anchor_t>(pattern.front())) {
    p = p | std::views::drop(1);
    anchors |= anchor_e::begin;
  }
  if (std::holds_alternative<end_anchor_t>(pattern.back())) {
    p = p | std::views::take(p.size() - 1);
    anchors |= anchor_e::end;
  }
  return matcher_internal(input, 0, p, 0, anchors);
}

int main(int argc, char* argv[]) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  {
    // auto p = parse_pattern(std::string("[^opq]q\\\\"));
    // auto p = parse_pattern(std::string("x[abc]+y"));
    // auto p = parse_pattern(std::string("a\\d+"));
    // auto p = parse_pattern(std::string("a[123]+123"));
    // auto p = parse_pattern(std::string("a123$"));
    auto p = parse_pattern(std::string("^abc"));
    // auto p = parse_pattern(std::string("^[jmav]+"));
    // auto p = parse_pattern(std::string("this$"));
    // auto p = parse_pattern(std::string("ca+aars"));
    // auto p = parse_pattern(std::string("d"));
    // auto p = parse_pattern(std::string("^strawberry$"));
    // auto p = parse_pattern(std::string("^abc_\\d+_xyz$"));

    bool test = false;
    // auto input = std::string("orangeq\\");
    // auto input = std::string("aaaxbbbacy");
    // auto input = std::string("helloa123");
    // auto input = std::string("a123123123123");
    auto input = std::string("abcthisisabc");
    // auto input = std::string("thisisajvm");
    // auto input = std::string("thisisnotthis");
    // auto input = std::string("caaars");
    // auto input = std::string("dog");
    // auto input = std::string("strawberry");
    // auto input = std::string("abc_123_xyz");
    // for (int i = 0; i < input.size(); i++) {
    // move starting position forward
    test = matcher(input, p);
    // if (test) {
    // break;
    // }
    // }
    // std::println("{}", test);
  }

  auto r1 = match_pattern(std::string("4 cats"), std::string("\\d \\w\\w\\ws"));
  auto r2 =
    match_pattern(std::string("sally has 1 orange"), std::string("\\d apple"));
  auto r3 = match_pattern(std::string("orange"), std::string("[^opq]"));
  auto r4 = match_pattern(std::string("e"), std::string("[orange]"));
  auto r5 = match_pattern(
    std::string("sally has 12 apples"), std::string("\\d\\\\d\\\\d apples"));
  auto r6 = match_pattern(std::string("abc123cde"), std::string("\\w\\w\\w$"));
  auto r7 = match_pattern(
    std::string("strawberry_strawberry"), std::string("^strawberry$"));

  auto t1 = parse_pattern(std::string("\\d \\w\\w\\ws"));
  auto t2 = parse_pattern(std::string("\\d apple"));
  auto t3 = parse_pattern(std::string("[^opq]"));
  auto t4 = parse_pattern(std::string("[orange]"));
  auto t5 = parse_pattern(std::string("\\d\\\\d\\\\d apples"));
  auto t6 = parse_pattern(std::string("\\w\\w\\w$"));
  auto t7 = parse_pattern(std::string("^strawberry$"));
  auto t8 = parse_pattern(std::string("\\d+"));
  auto t9 = parse_pattern(std::string("ca+aaars"));

  auto rr1 = /* match_pattern_2 */ matcher(std::string("4 cats"), t1);
  auto rr2 =
    /* match_pattern_2 */ matcher(std::string("sally has 1 orange"), t2);
  auto rr3 = /* match_pattern_2 */ matcher(std::string("orange"), t3);
  auto rr4 = /* match_pattern_2 */ matcher(std::string("e"), t4);
  auto rr5 =
    /* match_pattern_2 */ matcher(std::string("sally has 12 apples"), t5);
  auto rr6 = /* match_pattern_2 */ matcher(std::string("abc123cde"), t6);
  auto rr7 =
    /* match_pattern_2 */ matcher(std::string("strawberry_strawberry"), t7);
  auto rr8 = /* match_pattern_2 */ matcher(std::string("123"), t8);
  auto rr9 = /* match_pattern_2 */ matcher(std::string("caaars"), t9);

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
    // if (match_pattern(input_line, pattern)) {
    auto parsed_pattern = parse_pattern(pattern);
    if (matcher(input_line, parsed_pattern)) {
      std::cerr << std::format("0\n");
      return 0;
    } else {
      std::cerr << std::format("1\n");
      return 1;
    }
  } catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
