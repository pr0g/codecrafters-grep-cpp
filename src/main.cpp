#include <algorithm>
#include <cassert>
#include <iostream>
#include <span>
#include <string>
#include <variant>
#include <vector>

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

struct literal_t {
  char l;
};

struct digit_t {};

struct word_t {};

// todo - fix me (remove - handle as literal)
struct backslash_t {};

struct positive_character_group_t {
  std::string group;
};

struct negative_character_group_t {
  std::string group;
};

struct begin_anchor_t {};

struct end_anchor_t {};

struct one_or_more_t {};

using pattern_token_t = std::variant<
  literal_t, digit_t, word_t, positive_character_group_t,
  negative_character_group_t, begin_anchor_t, end_anchor_t, backslash_t,
  one_or_more_t>;

std::vector<pattern_token_t> parse_pattern(const std::string_view pattern) {
  std::vector<pattern_token_t> pattern_tokens;
  bool anchored_beginning = anchored_at_beginning(pattern.front());
  if (anchored_beginning) {
    pattern_tokens.push_back(begin_anchor_t{});
  }
  bool anchored_end = anchored_at_end(pattern.substr(pattern.size() - 2, 2));
  for (int p = anchored_beginning ? 1 : 0;
       p < (anchored_end ? pattern.size() - 1 : pattern.size());) {
    if (is_literal(pattern[p])) {
      if (pattern[p] == '+') {
        pattern_tokens.push_back(one_or_more_t{});
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

// helper type for the visitor #4
template<class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

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
      },
      [&](const one_or_more_t& one_or_more) {
        if (p - 1 >= 0) {
          auto before = pattern_tokens[p - 1];
          int i_before;
          int i_after = i;
          int ignored_p;
          int more = 0;
          do {
            i_before = i_after;
            std::tie(i_after, ignored_p) = match(
              before, i_after, input_line, p, pattern_tokens,
              anchored_beginning);
            if (i_after == i_before + 1) {
              more++;
            }
          } while (i_after > i_before && i_after < input_line.size());
          p++;
          i += more;
          int pp = p;
          while (pp < pattern_tokens.size()
                 && pattern_tokens[pp].index() == before.index()) {
            auto* b = std::get_if<literal_t>(&before);
            auto* n = std::get_if<literal_t>(&pattern_tokens[pp]);
            if (b != nullptr && n != nullptr) {
              if (b->l == n->l) {
                i--;
                pp++;
              } else {
                break;
              }
            } else if (b == nullptr && n == nullptr) {
              i--;
              pp++;
            }
          }
          int check;
          check = 0;
        }
        // todo
        // while input can recursively only match previous pattern, continue
        // when fails, continue processing rest of pattern
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

// enum class match_e { match_advance, match_hold, clash_advance, clash_abort };

// match_e do_match(pattern_token_t pattern_token, char c) {
//   match_e match = match_e::clash_abort;
//   std::visit(
//     overloaded{
//       [&](const literal_t& l) {
//         match = l.l == c ? match_e::match_advance : match_e::clash_abort;
//       },
//       [&](const digit_t& digit) {
//         match = std::isdigit(c) ? match_e::match_advance :
//         match_e::clash_abort;
//         ;
//       },
//       [&](const word_t& word) {
//         match = std::isalnum(c) || c == '_' ? match_e::match_advance
//                                             : match_e::clash_abort;
//       },
//       [&](const negative_character_group_t& negative_character_group) {
//         match = negative_character_group.group.find(c) == std::string::npos
//                 ? match_e::match_hold
//                 : match_e::clash_advance;
//       },
//       [&](const positive_character_group_t& positive_character_group) {
//         match = positive_character_group.group.find(c) != std::string::npos
//                 ? match_e::match_hold
//                 : match_e::clash_advance;
//       },
//       [&](const begin_anchor_t& begin_anchor) {},
//       [&](const end_anchor_t& end_anchor) {},
//       [&](const backslash_t& backslash) {},
//       [&](const one_or_more_t& one_or_more) {}},
//     pattern_token);
//   return match;
// }

std::optional<int> do_match(
  std::span<pattern_token_t> pattern,
  std::span<pattern_token_t>::size_type pattern_pos, char c) {
  std::optional<int> offset;
  std::visit(
    overloaded{
      [&](const literal_t& l) {
        if (l.l == c) {
          offset = 1;
        } else {
          offset = std::nullopt;
        }
      },
      [&](const digit_t& digit) {
        if (std::isdigit(c)) {
          offset = 1;
        } else {
          offset = std::nullopt;
        }
      },
      [&](const word_t& word) {
        if (std::isalnum(c) || c == '_') {
          offset = 1;
        } else {
          offset = std::nullopt;
        }
      },
      [&](const negative_character_group_t& negative_character_group) {
        if (int position = negative_character_group.group.find(c);
            position == std::string::npos) {
          offset = 1;
        } else {
          offset = std::nullopt;
        }
      },
      [&](const positive_character_group_t& positive_character_group) {
        if (int position = positive_character_group.group.find(c);
            position != std::string::npos) {
          offset = position;
        } else {
          offset = std::nullopt;
        }
      },
      [&](const begin_anchor_t& begin_anchor) {},
      [&](const end_anchor_t& end_anchor) {},
      [&](const backslash_t& backslash) {
        if (c == '\\') {
          offset = 1;
        } else {
          offset = std::nullopt;
        }
      },
      [&](const one_or_more_t& one_or_more) {
        // noop - skipped
      }},
    pattern[pattern_pos]);
  return offset;
}

bool matcher(
  std::string_view input, std::string_view::size_type input_pos,
  std::span<pattern_token_t> pattern,
  std::span<pattern_token_t>::size_type pattern_pos) {
  if (input_pos == input.size() && pattern_pos == pattern.size()) {
    return true;
  }
  if (input_pos == input.size() || pattern_pos == pattern.size()) {
    return false;
  }
  auto match = do_match(pattern, pattern_pos, input[input_pos]);
  if (match.has_value()) {
    if (pattern_pos < pattern.size() - 1) {
      if (std::holds_alternative<one_or_more_t>(pattern[pattern_pos + 1])) {
        return matcher(input, input_pos + 1, pattern, pattern_pos);
      } else {
        return matcher(input, input_pos + 1, pattern, pattern_pos + 1);
      }
    } else {
      return matcher(input, input_pos + 1, pattern, pattern_pos + 1);
    }
  } else {
    if (pattern_pos < pattern.size() - 1) {
      if (std::holds_alternative<one_or_more_t>(pattern[pattern_pos + 1])) {
        return matcher(input, input_pos, pattern, pattern_pos + 2);
      } else {
        return matcher(input, input_pos + 1, pattern, pattern_pos);
      }
    } else {
      return matcher(input, input_pos + 1, pattern, pattern_pos);
    }
  }
}

int main(int argc, char* argv[]) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  {
    // auto p = parse_pattern(std::string("[^hell]o"));
    auto p = parse_pattern(std::string("[^opq]q\\\\"));
    // auto p = parse_pattern(std::string("x[abc]+y"));
    // auto p = parse_pattern(std::string("a\\d+"));
    bool test = false;
    // auto input = std::string("123");
    auto input = std::string("orangeq\\");
    // auto input = std::string("aaaxbbbacy");
    // for (int i = 0; i < input.size(); i++) {
    // move starting position forward
    test = matcher(input, 0, p, 0);
    // if (test) {
    // break;
    // }
    // }
    std::println("{}", test);
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

  auto rr1 = /* match_pattern_2 */ matcher(std::string("4 cats"), 0, t1, 0);
  auto rr2 =
    /* match_pattern_2 */ matcher(std::string("sally has 1 orange"), 0, t2, 0);
  auto rr3 = /* match_pattern_2 */ matcher(std::string("orange"), 0, t3, 0);
  auto rr4 = /* match_pattern_2 */ matcher(std::string("e"), 0, t4, 0);
  auto rr5 =
    /* match_pattern_2 */ matcher(std::string("sally has 12 apples"), 0, t5, 0);
  auto rr6 = /* match_pattern_2 */ matcher(std::string("abc123cde"), 0, t6, 0);
  auto rr7 =
    /* match_pattern_2 */ matcher(
      std::string("strawberry_strawberry"), 0, t7, 0);
  auto rr8 = /* match_pattern_2 */ matcher(std::string("123"), 0, t8, 0);
  auto rr9 = /* match_pattern_2 */ matcher(std::string("caaars"), 0, t9, 0);

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
    if (match_pattern_2(input_line, parsed_pattern)) {
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
