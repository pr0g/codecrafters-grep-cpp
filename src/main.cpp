#include <algorithm>
#include <iostream>
#include <string>

bool match_literal(
  const std::string_view input_line, const std::string_view pattern) {
  return input_line.find(pattern) != std::string_view::npos;
}

bool match_digit(const std::string_view input_line) {
  return std::any_of(
    input_line.begin(), input_line.end(),
    [](const unsigned char c) { return std::isdigit(c); });
}

bool match_word_character(const std::string_view input_line) {
  return std::any_of(
    input_line.begin(), input_line.end(),
    [](const unsigned char c) { return std::isalnum(c) || c == '_'; });
}

bool match_positive_characters(
  const std::string_view input_line, const std::string_view pattern) {
  const auto end = pattern.find(']');
  const auto characters = pattern.substr(1, end - 1);
  return std::any_of(
    characters.begin(), characters.end(), [&input_line](const unsigned char c) {
      return input_line.find(c) != std::string::npos;
    });
}

bool match_negative_characters(
  const std::string_view input_line, const std::string_view pattern) {
  const auto end = pattern.find(']');
  const auto characters = pattern.substr(2, end - 2);
  return std::any_of(
    input_line.begin(), input_line.end(), [&characters](const unsigned char c) {
      return characters.find(c) == std::string::npos;
    });
}

bool match_pattern(
  const std::string_view input_line, const std::string_view pattern) {
  int p = 0;
  int i = 0;
  for (; i < input_line.size() && p < pattern.size();) {
    if (pattern[p] != '\\' && pattern[p] != '[') {
      if (input_line[i] == pattern[p]) {
        i++;
        p++;
      } else {
        // don't skip character
        if (p == 0) {
          i++;
        }
        p = 0;
      }
    } else {
      if (pattern[p] == '\\') {
        if (pattern[p + 1] == 'd') {
          if (std::isdigit(input_line[i])) {
            p += 2;
            i++;
            // return true;
          } else {
            i++;
          }
        } else if (pattern[p + 1] == 'w') {
          if (std::isalnum(input_line[i]) || input_line[i] == '_') {
            p += 2;
            i++;
            // return true;
          } else {
            i++;
          }
        }
      } else if (pattern[p] == '[') {
        if (pattern[p + 1] == '^') {
          const auto end = pattern.find(']', p + 2);
          const auto characters = pattern.substr(p + 2, end - (p + 2));
          auto not_found = std::any_of(
            input_line.begin(), input_line.end(),
            [&characters](const unsigned char c) {
              return characters.find(c) == std::string::npos;
            });
          if (not_found) {
            // return true;
            p += (end - p) + 1;
            i++;
          } else {
            // p += end - p;
            i += input_line.size() - i;
          }
        } else {
          const auto end = pattern.find(']', p + 1);
          const auto characters = pattern.substr(p + 1, end - (p + 1));
          auto exists = std::any_of(
            characters.begin(), characters.end(),
            [&input_line, i](const unsigned char c) {
              return input_line.find(c) != std::string::npos;
            });
          if (exists) {
            // return true;
            p += (end - p) + 1;
            i++;
          } else {
            // p += end - p;
            i += input_line.size() - i;
          }
        }
      }
    }
  }
  return p == pattern.size();

  // if (pattern.length() == 1) {
  //   return match_literal(input_line, pattern);
  // } else if (pattern == "\\d") {
  //   return match_digit(input_line);
  // } else if (pattern == "\\w") {
  //   return match_word_character(input_line);
  // } else if (pattern[0] == '[') {
  //   if (pattern[1] == '^') {
  //     return match_negative_characters(input_line, pattern);
  //   }
  //   return match_positive_characters(input_line, pattern);
  // } else {
  //   throw std::runtime_error("Unhandled pattern " + std::string(pattern));
  // }
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
