#include <cstring>
#include <iostream>

void push(char**& stack, int64_t& length, int64_t& max_length) {
  int64_t value_size;
  value_size = 1;
  int64_t size;
  size = 1;
  char* value = new char[value_size];
  int symbol;
  symbol = getchar();
  symbol = getchar();
  value[0] = static_cast<char>(symbol);
  while (symbol != '\0' && !isspace(symbol)) {
    if (size == value_size) {
      char* value_copy = new char[size];
      for (int i = 0; i < size; ++i) {
        value_copy[i] = value[i];
      }
      delete[] value;
      value_size = 2 * size;
      value = new char[value_size];
      for (int i = 0; i < size; ++i) {
        value[i] = value_copy[i];
      }
      delete[] value_copy;
    }
    symbol = getchar();
    value[size] = static_cast<char>(symbol);
    ++size;
  }
  stack[length] = new char[size];
  for (int i = 0; i < size - 1; ++i) {
    stack[length][i] = value[i];
  }
  stack[length][size - 1] = '\0';
  ++length;
  if (length == max_length) {
    char** stack_copy = new char*[length];
    for (int i = 0; i < length; ++i) {
      stack_copy[i] = new char[strlen(stack[i]) + 1];
      for (int j = 0; j < strlen(stack[i]) + 1; ++j) {
        stack_copy[i][j] = stack[i][j];
      }
      delete[] stack[i];
    }
    delete[] stack;
    max_length = 2 * length;
    stack = new char*[max_length];
    for (int i = 0; i < length; ++i) {
      stack[i] = new char[strlen(stack_copy[i]) + 1];
      for (int j = 0; j < strlen(stack_copy[i]) + 1; ++j) {
        stack[i][j] = stack_copy[i][j];
      }
      delete[] stack_copy[i];
    }
    delete[] stack_copy;
  }
  std::cout << "ok" << "\n";
  delete[] value;
}

void pop(char** stack, int64_t& length) {
  if (length == 0) {
    std::cout << "error" << "\n";
  } else {
    std::cout << stack[length - 1] << "\n";
    delete[] stack[--length];
  }
}

void back(char** stack, int64_t& length) {
  if (length == 0) {
    std::cout << "error" << "\n";
  } else {
    std::cout << stack[length - 1] << "\n";
  }
}

void size(int64_t& length) {
  std::cout << length << "\n";
}

void clear(char**& stack, int64_t& length, int64_t& max_length) {
  for (int i = 0; i < length; ++i) {
    delete[] stack[i];
  }
  delete[] stack;
  max_length = 1;
  length = 0;
  stack = new char* [max_length];
  std::cout << "ok" << "\n";
}

int main() {
  const int kCommandSize = 6;
  int64_t length;
  int64_t max_length;
  length = 0;
  max_length = 1;
  char** stack = new char*[max_length];
  char command[kCommandSize];
  while (true) {
    std::cin >> command;
    if (memcmp(command, "push", 4) == 0) {
      push(stack, length, max_length);
    } else if (memcmp(command, "pop", 3) == 0) {
      pop(stack, length);
    } else if (memcmp(command, "back", 4) == 0) {
      back(stack, length);
    } else if (memcmp(command, "size", 4) == 0) {
      size(length);
    } else if (memcmp(command, "clear", 5) == 0) {
      clear(stack, length, max_length);
    } else if (memcmp(command, "exit", 4) == 0) {
      std::cout << "bye" << "\n";
      break;
    }
  }
  for (int i = 0; i < length; ++i) {
    delete[] stack[i];
  }
  delete[] stack;
  return 0;
}

