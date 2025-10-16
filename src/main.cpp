#include <algorithm>
#include <cassert>
#include <iostream>
#include <span>
#include <string>

bool is_literal(const char c) {
  // todo - more regex meta characters to add
  return c != '\\' && c != '[';
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
  std::string_view input, int i, std::string_view pattern, int p) {
  if (input[i] == pattern[p]) {
    return {i + 1, p + 1};
  } else {
    // if there are currently no matches, move to the next character
    // otherwise, reset pattern search, and stay on the current character
    return p == 0 ? std::tuple<int, int>{i + 1, p} : std::tuple<int, int>{i, 0};
  }
}

bool match_pattern(
  const std::string_view input_line, const std::string_view pattern) {
  int p = 0;
  for (int i = 0; i < input_line.size() && p < pattern.size();) {
    if (is_literal(pattern[p])) {
      std::tie(i, p) = literal_check(input_line, i, pattern, p);
    } else {
      if (is_escape(pattern[p])) {
        if (is_digit(pattern[p + 1])) {
          std::tie(i, p) = character_class_match(
            input_line, i, p, [](const char c) { return std::isdigit(c); });
        } else if (pattern[p + 1] == 'w') {
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
  }
  return p == pattern.size();
}

int main(int argc, char* argv[]) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  auto r = match_pattern(std::string("4 cats"), std::string("\\d \\w\\w\\ws"));
  auto r2 =
    match_pattern(std::string("sally has 1 orange"), std::string("\\d apple"));
  auto r3 = match_pattern(std::string("orange"), std::string("[^opq]"));
  auto r4 = match_pattern(std::string("e"), std::string("[orange]"));
  auto r5 = match_pattern(
    std::string("sally has 12 apples"), std::string("\\d\\\\d\\\\d apples"));

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
    if (match_pattern(input_line, pattern)) {
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
