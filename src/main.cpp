#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
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

enum class quantifier_e { one_or_more, zero_or_one, zero_or_more };

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
      } else if (pattern[p] == '*') {
        set_quantifier(pattern_tokens.back(), quantifier_e::zero_or_more);
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
    return quantifier == quantifier_e::zero_or_one
            || quantifier == quantifier_e::zero_or_more
           ? std::make_optional(0)
           : std::nullopt;
  }
  std::optional<int> next_opt;
  auto move_opt =
    do_match(pattern, pattern_pos, input, input_pos, anchors, captured_groups);
  if (!move_opt) {
    if (
      quantifier != quantifier_e::zero_or_one
      && quantifier != quantifier_e::zero_or_more) {
      return std::nullopt;
    }
    // try next pattern position, ignoring the previous mismatch
    next_opt = match_here(
      input, input_pos, pattern, pattern_pos + 1, anchors, captured_groups);
    if (!next_opt) {
      return std::nullopt;
    }
    move_opt = next_opt;
  }
  if (!move_opt && quantifier == quantifier_e::zero_or_more) {
    next_opt = match_here(
      input, input_pos + 1, pattern, pattern_pos, anchors, captured_groups);
    if (!next_opt) {
      return std::nullopt;
    }
    move_opt = next_opt;
  }
  if (
    !next_opt
    && (quantifier == quantifier_e::one_or_more || quantifier == quantifier_e::zero_or_more)) {
    // match again at next input position with current pattern position
    next_opt = match_here(
      input, input_pos + *move_opt, pattern, pattern_pos, anchors,
      captured_groups);
  }

  if (!next_opt) {
    // normal case, move to the next pattern position and input position
    next_opt = match_here(
      input, input_pos + *move_opt, pattern, pattern_pos + 1, anchors,
      captured_groups);
    // if previous case was a greedy matcher that failed, 'give back' characters
    // repeatedly until we find a match with the next character in the input
    if (!next_opt) {
      for (auto move = *move_opt - 1; move > 1; move--) {
        // backtrack
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
  return *next_opt + *move_opt;
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
    for (auto& pattern : patterns) {
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

int grep(const std::string_view pattern, const std::string_view input) {
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

using matches_t = std::vector<std::pair<std::string, std::vector<std::string>>>;

void do_matches(
  const std::string& filename, const std::string_view pattern,
  matches_t& matches) {
  if (std::ifstream reader(filename); reader.is_open()) {
    std::optional<std::string> matched_filename;
    std::vector<std::string> matched_lines;
    for (std::string line; std::getline(reader, line);) {
      if (grep(pattern, line) == 0) {
        if (!matched_filename) {
          matched_filename = filename;
        }
        matched_lines.push_back(line);
      }
    }
    if (matched_filename) {
      matches.push_back(
        {std::move(*matched_filename), std::move(matched_lines)});
    }
  }
}

int main(int argc, char* argv[]) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  {
    using std::literals::string_literals::operator""s;
    // auto match = grep("ca*t"s, "ct"s);
    auto match = grep("pear*"s, "pea"s);
    // auto match = grep("k\\d*t"s, "kt"s);
    int a;
    a = 0;
  }

  if (argc < 3) {
    std::cerr << "Expected at least three arguments" << std::endl;
    return 1;
  }

  const auto [flag, pattern, recursive] =
    [&] -> std::tuple<std::string, std::string, bool> {
    if (argv[1] == std::string("-r")) {
      return std::tuple{argv[2], argv[3], true};
    } else if (argv[1] == std::string("-E")) {
      return std::tuple{argv[1], argv[2], false};
    }
    std::unreachable();
  }();

  if (flag != "-E") {
    std::cerr << "Expected first argument to be '-E'" << std::endl;
    return 1;
  }

  if (argc >= 4) {
    matches_t matches;
    for (int i = recursive ? 4 : 3; i < argc; i++) {
      if (recursive) {
        namespace fs = std::filesystem;
        const std::string& directory = argv[i];
        for (const fs::directory_entry& entry :
             fs::recursive_directory_iterator(directory)) {
          if (fs::is_regular_file(entry.path())) {
            do_matches(entry.path().string(), pattern, matches);
          }
        }
      } else {
        do_matches(argv[i], pattern, matches);
      }
    }
    if (matches.empty()) {
      return 1;
    }
    if (argc == 4) {
      for (const auto& line : matches.front().second) {
        std::cout << line << '\n';
      }
    } else if (argc >= 5) {
      for (const auto& match : matches) {
        for (const auto& line : match.second) {
          std::cout << std::format("{}:{}", match.first, line) << '\n';
        }
      }
    }
    return 0;
  } else {
    std::string input;
    std::getline(std::cin, input);
    return grep(pattern, input);
  }
}
