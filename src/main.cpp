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

struct begin_anchor_t {};
struct end_anchor_t {};

using pattern_token_t = std::variant<
  literal_t, digit_t, word_t, positive_character_group_t,
  negative_character_group_t, begin_anchor_t, end_anchor_t, wildcard_t>;

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
      [&](wildcard_t& wildcard) { wildcard.quantifier = quantifier; },
      [&](begin_anchor_t& begin_anchor) { /* noop */ },
      [&](end_anchor_t& end_anchor) { /* noop */ },
    },
    pattern_token);
}

std::optional<quantifier_e> get_quantifier(
  const pattern_token_t& pattern_token) {
  std::optional<quantifier_e> quantifier;
  std::visit(
    overloaded{
      [&](const literal_t& literal) { quantifier = literal.quantifier; },
      [&](const digit_t& digit) { quantifier = digit.quantifier; },
      [&](const word_t& word) { quantifier = word.quantifier; },
      [&](const negative_character_group_t& negative_character_group) {
        quantifier = negative_character_group.quantifier;
      },
      [&](const positive_character_group_t& positive_character_group) {
        quantifier = positive_character_group.quantifier;
      },
      [&](const wildcard_t& wildcard) { quantifier = wildcard.quantifier; },
      [&](const begin_anchor_t& begin_anchor) {},
      [&](const end_anchor_t& end_anchor) {}},
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
        if (is_digit(pattern[p + 1])) {
          pattern_tokens.push_back(digit_t{});
          p += 2;
        } else if (is_word(pattern[p + 1])) {
          pattern_tokens.push_back(word_t{});
          p += 2;
        } else if (pattern[p + 1] == '\\') {
          pattern_tokens.push_back(literal_t{.l = '\\'});
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

bool do_match(
  std::span<pattern_token_t> pattern,
  std::span<pattern_token_t>::size_type pattern_pos, std::string_view input,
  std::string_view::size_type input_pos) {
  bool match = false;
  const char c = input[input_pos];
  std::visit(
    overloaded{
      [&](const literal_t& l) { match = l.l == c; },
      [&](const digit_t& digit) { match = std::isdigit(c); },
      [&](const word_t& word) { match = std::isalnum(c) || c == '_'; },
      [&](const negative_character_group_t& negative_character_group) {
        match = negative_character_group.group.find(c) == std::string::npos;
      },
      [&](const positive_character_group_t& positive_character_group) {
        match = positive_character_group.group.find(c) != std::string::npos;
      },
      [&](const wildcard_t& wildcard) { match = true; },
      [&](const begin_anchor_t& begin_anchor) { /* noop */ },
      [&](const end_anchor_t& end_anchor) { /* noop */ }},
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
  const auto quantifier = get_quantifier(pattern[pattern_pos]);
  if (input_pos == input.size()) {
    return quantifier == quantifier_e::zero_or_one ? true : false;
  }
  if (do_match(pattern, pattern_pos, input, input_pos)) {
    if (quantifier == quantifier_e::one_or_more) {
      // try to match more of the pattern (greedy)
      if (matcher_internal(
            input, input_pos + 1, pattern, pattern_pos, anchors)) {
        return true;
      }
    }
    return matcher_internal(
      input, input_pos + 1, pattern, pattern_pos + 1, anchors);
  } else {
    if (quantifier == quantifier_e::one_or_more) {
      // no match at current position
      return false;
    } else if (quantifier == quantifier_e::zero_or_one) {
      return matcher_internal(
        input, input_pos, pattern, pattern_pos + 1, anchors);
    }
    if ((anchors & anchor_e::begin) == 0) {
      return matcher_internal(input, input_pos + 1, pattern, 0, anchors);
    } else {
      return false;
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
    if (auto parsed_pattern = parse_pattern(pattern);
        matcher(input_line, parsed_pattern)) {
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
