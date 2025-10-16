#include <algorithm>
#include <iostream>
#include <string>

bool match_literal(const std::string& input_line, const std::string& pattern) {
  return input_line.find(pattern) != std::string::npos;
}

bool match_digit(const std::string& input_line) {
  return std::any_of(
    input_line.begin(), input_line.end(),
    [](const unsigned char c) { return std::isdigit(c); });
}

bool match_word_character(const std::string& input_line) {
  return std::any_of(
    input_line.begin(), input_line.end(),
    [](const unsigned char c) { return std::isalnum(c) || c == '_'; });
}

bool match_pattern(const std::string& input_line, const std::string& pattern) {
  if (pattern.length() == 1) {
    return match_literal(input_line, pattern);
  } else if (pattern == "\\d") {
    return match_digit(input_line);
  } else if (pattern == "\\w") {
    return match_word_character(input_line);
  } else {
    throw std::runtime_error("Unhandled pattern " + pattern);
  }
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
