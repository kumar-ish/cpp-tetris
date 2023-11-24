#include "tetris.hpp"

#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75
#define KEY_RIGHT 77

std::unordered_map<char, Tetris::Input> mapping{
    {' ', Key::SPACE},
    {'c', Key::HOLD},
    {'q', Rotation::COUNTER_CLOCKWISE},
    {'e', Rotation::CLOCKWISE},
    {KEY_DOWN, Direction::DOWN},
    {KEY_RIGHT, Direction::RIGHT},
    {KEY_LEFT, Direction::LEFT}};

int main() {
  Tetris a = Tetris::standardTetris();

  char c;
  while (std::cin.get(c)) {
    if (mapping.contains(c)) {
      auto value = mapping[c];
      a.handleInput(value);
      a.print();
    }
    a.handleInput(Direction::LEFT);
    std::cout.flush();
  }
  a.handleInput(Direction::DOWN);
  a.handleInput(Direction::DOWN);
  a.handleInput(Direction::DOWN);

  return 0;
}
