// this is an assembler for the n2t project

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>

int instructionCounter{};
int lineCounter{};

struct CInstructionContent {
  std::string comp;
  std::string dest;
  std::string jump;
};

class Parser {
public:
  std::string AVal{};
  CInstructionContent CVal{};

  bool isCOperation{};

  Parser(const std::string &line) {
    Type instructionType{getInstructionType(line)};
    std::cout << line << '\n';

    if (instructionType == Type::A_INSTRUCTION) {
      isCOperation = false;
      parseAInstruction(line);
      std::cout << AVal << '\n';
    }

    if (instructionType == Type::C_INSTRUCTION) {
      isCOperation = true;
      parseCInstruction(line);
      std::cout << "\n- " << CVal.dest << "\n- " << CVal.comp << "\n- "
                << CVal.jump << '\n';
    }

    // stript whitespaces for future binary encoding
    AVal.erase(std::remove_if(AVal.begin(), AVal.end(), ::isspace), AVal.end());
    CVal.jump.erase(
        std::remove_if(CVal.jump.begin(), CVal.jump.end(), ::isspace),
        CVal.jump.end());
    CVal.comp.erase(
        std::remove_if(CVal.comp.begin(), CVal.comp.end(), ::isspace),
        CVal.comp.end());
    CVal.dest.erase(
        std::remove_if(CVal.dest.begin(), CVal.dest.end(), ::isspace),
        CVal.dest.end());
  }

private:
  enum class Type {
    COMMENT,
    A_INSTRUCTION,
    C_INSTRUCTION,
    L_INSTRUCTION, // (xxx)
  };

  Type getInstructionType(const std::string &line) {
    for (std::size_t i{}; i < std::size(line); i++) {
      switch (line[i]) {
      case ' ':
      case '\t':
        continue;

      case '/':
        if (line[i + 1] == '/')
          return Type::COMMENT;
      case '@':
        return Type::A_INSTRUCTION;
        break;
      case '(':
        return Type::L_INSTRUCTION;
      }
    }

    return Type::C_INSTRUCTION;
  }

  void parseAInstruction(const std::string &line) {
    for (const char c : line) {
      if (c == ' ' || c == '\t')
        continue;
      if (c == '@')
        continue;
      else
        AVal += c;
    }
  }

  void parseCInstruction(const std::string &line) {

    // if dest empty, = ommited
    bool isDest{false};
    // if jump empty, ; ommited
    bool isJump{false};

    for (const char c : line) {
      switch (c) {
      case '=':
        isDest = true;
        break;
      case ';':
        isJump = true;
        break;
      }
    }

    std::size_t charIndex{0};
    if (isDest) {
      for (const char c : line) {

        ++charIndex;

        if (c == '=')
          break;

        switch (c) {
        case ' ':
        case '\t':
          continue;
        default:
          CVal.dest += c;
        }
      }
    }

    for (std::size_t i{charIndex}; i < std::size(line); i++) {
      ++charIndex;
      if (isJump && line[i] == ';')
        break;
      CVal.comp += line[i];
    }

    if (isJump) {
      for (std::size_t i{charIndex}; i < std::size(line); i++) {
        CVal.jump += line[i];
      }
    }
  }
};

class Code {
public:
  std::string binary;

  Code(const Parser &parsed) {
    if (parsed.isCOperation) {
      encodeCOperation(parsed.CVal);
    } else {
      encodeAOperation(parsed.AVal);
    }

    std::cout << binary << '\n';
    std::cout << "\n---------\n\n";
  };

private:
  void encodeCOperation(const CInstructionContent &content) {
    // decode comp field
    int aComp{0};
    if (content.comp.find('M') != std::string::npos) {
      aComp = 1; // fed the ALU with M if M used
    }

    std::cout << "acomp: " << aComp << '\n';

    // populate binary instruction
    binary += "111";

    std::cout << binary << '\n';

    content.dest.find('A') != std::string::npos ? binary += '1' : binary += '0';
    content.dest.find('D') != std::string::npos ? binary += '1' : binary += '0';
    content.dest.find('M') != std::string::npos ? binary += '1' : binary += '0';

    std::cout << binary << '\n';

    binary += std::to_string(aComp);

    std::cout << binary << '\n';

    std::unordered_map<std::string, std::string> comp{
        {"0", "101010"},   {"1", "111111"},   {"-1", "111010"},
        {"D", "001100"},   {"A", "110000"},   {"M", "110000"},
        {"!D", "001101"},  {"!A", "110001"},  {"!M", "110001"},
        {"-D", "001111"},  {"-A", "110011"},  {"-M", "110011"},
        {"D+1", "011111"}, {"A+1", "110111"}, {"M+1", "110111"},
        {"D-1", "001110"}, {"A-1", "110010"}, {"M-1", "110010"},
        {"D+A", "000010"}, {"D+M", "000010"}, {"D-A", "010011"},
        {"D-M", "010011"}, {"A-D", "000111"}, {"M-D", "000111"},
        {"D&A", "000000"}, {"D&M", "000000"}, {"D|A", "010101"},
        {"D|M", "010101"},
    };

    binary += comp[content.comp];

    std::cout << binary << '\n';

    std::unordered_map<std::string, std::string> jump{
        {"JGT", "001"}, {"JEQ", "010"}, {"JGE", "011"}, {"JLT", "100"},
        {"JNE", "101"}, {"JLE", "110"}, {"JMP", "111"},
    };

    content.jump != "" ? binary += jump[content.jump] : binary += "000";

    std::cout << binary << '\n';
  }

  void encodeAOperation(const std::string &content) {
    binary += '0';
    binary += std::bitset<15>(std::stoi(content)).to_string();
  }
};

class SymbolTable {};

int main(int argc, char *argv[]) {
  std::ifstream file(argv[1]);
  if (!file.is_open()) {
    std::cerr << "Unable to open asm file: " << argv[1];
    return 1;
  }

  std::string line;
  while (std::getline(file, line, '\n')) {
    ++lineCounter;
    if (line.empty())
      continue;
    Parser parsed(line);
    Code code(parsed);
  }
}
