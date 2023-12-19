#ifndef TETRIS
#define TETRIS

#include <algorithm>
#include <expected>
#include <functional>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "helper.hpp"

enum class Key { HOLD, SPACE };
enum class Rotation { CLOCKWISE, COUNTER_CLOCKWISE };
enum class Direction { DOWN, LEFT, RIGHT };

struct Coord;
using Coords = std::vector<Coord>;

struct Coord {
  int x;
  int y;

  bool inBounds(int width, int height) const {
    return x < width and x >= 0 and y < height and y >= 0;
  }

  static Coord directionCoord(Direction direction) {
    switch (direction) {
    case Direction::DOWN:
      return Coord{0, -1};
    case Direction::LEFT:
      return Coord{-1, 0};
    case Direction::RIGHT:
      return Coord{1, 0};
    default:
      std::unreachable();
    }
  }

  auto operator<=>(const Coord &other) const = default;
  Coord operator+(const Coord &other) const {
    return {x + other.x, y + other.y};
  }
  Coord operator+(Direction direction) const {
    return *this + Coord::directionCoord(direction);
  }
  Coord operator*(int mul) const { return Coord(x * mul, y * mul); }
};

class Shape {
  using KickData = std::array<std::array<Coord, 4>, 4>;

  Shape(int _size, const Coords &_coords, int _rotationIndex = 0,
        std::optional<KickData> _kickData = std::nullopt)
      : size(_size), coords(_coords), rotationIndex(_rotationIndex),
        kickData(_kickData) {
    if (_size <= 0) {
      throw std::invalid_argument(
          "size cannot be non-positive (must be greater than or equal to 1)");
    }
    if (std::ranges::any_of(_coords, [_size](auto &c) {
          return not c.inBounds(_size, _size);
        })) {
      throw std::invalid_argument("coord arguments don't fit with size of "
                                  "matrix (must be between 0 and n inclusive)");
    }
    if (_rotationIndex > 3 or _rotationIndex < 0) {
      throw std::invalid_argument(
          "rotation index not in bounds (must be between 0 and 3 inclusive)");
    }
  }

public:
  friend class ShapeFactory;

  std::string name;
  int size;
  Coords coords;
  int rotationIndex;
  std::optional<KickData> kickData;

  static Coord applyKickRotation(Coord coord, Rotation rotation) {
    switch (rotation) {
    case Rotation::CLOCKWISE:
      return coord;
    case Rotation::COUNTER_CLOCKWISE:
      return coord * -1;
    default:
      std::unreachable();
    }
  }

  Coords transformCoords(const std::function<Coord(const Coord &)> &f) const {
    return std::ranges::transform_view(coords, f) | std::ranges::to<Coords>();
  }
  Shape rotateClockwise() const {
    auto rotate = [this](const auto &coord) -> Coord {
      return {coord.y, size - 1 - coord.x};
    };
    return {size, transformCoords(rotate), (rotationIndex + 1) % 4, kickData};
  }

  Shape rotateCounterClockwise() const {
    auto rotate = [this](const auto &coord) -> Coord {
      return {size - 1 - coord.y, coord.x};
    };
    // C++'s modulo operator can return <0 numbers, so we add 4
    return {size, transformCoords(rotate), (rotationIndex + 4 - 1) % 4,
            kickData};
  }
  Shape(Shape &) = default;
  Shape(const Shape &) = default;
  Shape(Shape &&) = default;
  Shape &operator=(Shape const &) = default;
  Shape &operator=(Shape &) = default;
  Shape &operator=(Shape &&) = default;
};

class ShapeFactory {
private:
  // 0 -> R
  // R -> 2
  // 2 -> L
  // L -> 0
  constexpr static const Shape::KickData TLJSZ_KICKDATA{
      {{Coord{-1, 0}, Coord{-1, 1}, Coord{0, -2}, Coord{-1, -2}},
       {Coord{1, 0}, Coord{1, -1}, Coord{0, 2}, Coord{1, 2}},
       {Coord{1, 0}, Coord{1, 1}, Coord{0, -2}, Coord{1, -2}},
       {Coord{-1, 0}, Coord{-1, -1}, Coord{0, 2}, Coord{-1, 2}}}};
  constexpr static const Shape::KickData I_KICKDATA{
      {{Coord{-2, 0}, Coord{1, 0}, Coord{-2, -1}, Coord{1, 2}},
       {Coord{-1, 0}, Coord{2, 0}, Coord{-1, 2}, Coord{2, -1}},
       {Coord{2, 0}, Coord{-1, 1}, Coord{2, 1}, Coord{-1, -2}},
       {Coord{1, 0}, Coord{-2, -1}, Coord{1, -2}, Coord{-2, 1}}}};

public:
  // tetriminoes
  const Shape I_BLOCK{4, {{0, 1}, {1, 1}, {2, 1}, {3, 1}}, 0, I_KICKDATA};

  const Shape T_BLOCK{3, {{0, 0}, {0, 1}, {0, 2}, {1, 1}}, 0, TLJSZ_KICKDATA};
  const Shape L_BLOCK{3, {{0, 0}, {0, 1}, {0, 2}, {1, 2}}, 0, TLJSZ_KICKDATA};
  const Shape J_BLOCK{3, {{1, 0}, {1, 1}, {1, 2}, {0, 2}}, 0, TLJSZ_KICKDATA};
  const Shape S_BLOCK{3, {{0, 1}, {0, 2}, {1, 0}, {1, 1}}, 0, TLJSZ_KICKDATA};
  const Shape Z_BLOCK{3, {{1, 1}, {1, 2}, {0, 0}, {0, 1}}, 0, TLJSZ_KICKDATA};

  const Shape O_BLOCK{3, {{1, 1}, {1, 2}, {2, 1}, {2, 2}}};

  const std::vector<Shape> defaultShapes{I_BLOCK, O_BLOCK, T_BLOCK, L_BLOCK,
                                         J_BLOCK, S_BLOCK, Z_BLOCK};

  const std::vector<Shape> &getShapes() const { return defaultShapes; }

  Shape getRandomShape() const {
    return defaultShapes[rand() % defaultShapes.size()];
  };
};

class Tetris {
private:
  const int width;
  const int height;

  // tetris board
  std::vector<std::vector<bool>> cells;

  const ShapeFactory factory = ShapeFactory();
  Shape currentShape = factory.getRandomShape();
  Coord shapeLocation;

  std::optional<Shape> holdShape = std::nullopt;
  // If you've held in the turn already
  bool heldInTurn = false;

  int level = 1;
  int score = 0;
  [[maybe_unused]] double speed = 1.0;

  void setCellAt(Coord c, bool b) { cells[height - 1 - c.y][c.x] = b; }
  bool cellAt(Coord c) const { return cells[height - 1 - c.y][c.x]; }

  void resetShapeLocation() {
    shapeLocation = {width / 2 - currentShape.size / 2,
                     height / 2 - currentShape.size};
  }

  static std::vector<Coord> absShapeCoords(const Coord &location,
                                           const Shape &shape) {
    auto addLocation = [&location](auto offset) { return location + offset; };

    return shape.transformCoords(addLocation);
  }

  bool shapeInBounds(Coord &location, const Shape &shape) const {
    auto coords = absShapeCoords(location, shape);

    return std::all_of(coords.cbegin(), coords.cend(),
                       [this](auto c) { return c.inBounds(width, height); });
  }

  bool shapeBlocked(Coord location, const Shape &shape) const {
    if (not shapeInBounds(location, shape)) {
      return true;
    }

    auto coords = absShapeCoords(location, shape);
    return std::any_of(coords.cbegin(), coords.cend(),
                       [this](auto c) { return cellAt(c); });
  }

  void resetShape(const Shape &shape) {
    currentShape = shape;
    resetShapeLocation();
  }

  int clear() {
    // Remove from the bottom row to the top
    auto removedRange = std::ranges::remove_if(
        std::ranges::reverse_view(cells),
        [](auto const &c) { return std::ranges::all_of(c, std::identity()); });

    int numCleared = (int)removedRange.size();
    for (auto &iter : removedRange) {
      iter = std::vector<bool>(width, false);
    }

    return numCleared;
  }

  // Returns whether the move leads to a crystallisation of the shape
  bool move(Direction direction) {
    auto movedLocation = shapeLocation + direction;

    if (not shapeBlocked(movedLocation,
                         currentShape)) { // flowing through air -- let it flow
      shapeLocation = movedLocation;
      return false;
    }

    if (direction != Direction::DOWN) {
      return false;
    }

    // blocked + going down means that shape has to be placed
    for (auto c : absShapeCoords(shapeLocation, currentShape)) {
      setCellAt(c, true);
    }

    currentShape = factory.getRandomShape();

    resetShapeLocation();
    heldInTurn = false;

    clear();
    return true;
  }

  void rotate(Rotation rotation) {
    auto rotatedShape = std::invoke([rotation, this] {
      switch (rotation) {
      case Rotation::CLOCKWISE:
        return currentShape.rotateClockwise();
      case Rotation::COUNTER_CLOCKWISE:
        return currentShape.rotateCounterClockwise();
      default:
        std::unreachable();
      }
    });

    if (not shapeBlocked(shapeLocation, rotatedShape)) {
      currentShape = rotatedShape;
      return;
    } else if (currentShape.kickData->empty()) {
      return;
    }

    // We have kickdata, so we have to visit all of our options there
    auto kickData = rotatedShape.kickData->data()[rotatedShape.rotationIndex];
    for (auto &kickOffset : kickData) {
      Coord newLocation =
          Shape::applyKickRotation(kickOffset, rotation) + shapeLocation;

      if (not shapeBlocked(newLocation, rotatedShape)) {
        shapeLocation = newLocation;
        currentShape = rotatedShape;
        return;
      }
    }
  }

  void hold() {
    if (heldInTurn) {
      // If you've held already this turn, you shouldn't be able to hold again
      return;
    }
    if (holdShape.has_value()) {
      auto temp = std::move(currentShape);
      resetShape(holdShape.value());
      holdShape = std::move(temp);
    } else {
      holdShape = std::move(currentShape);
      resetShape(factory.getRandomShape());
    }
    heldInTurn = true;
  }

  void handleKey(Key key) {
    switch (key) {
    case Key::HOLD: {
      hold();
    }
    case Key::SPACE: {
      // keep moving down until materialization
      while (not move(Direction::DOWN)) {
      }
    }
    }
  }

  explicit Tetris(int _width, int _height) : width(_width), height(_height) {
    resetShapeLocation();
    cells = std::vector(height, std::vector<bool>(width));
  }

public:
  using Input = std::variant<Direction, Key, Rotation>;
  enum class InputError { INVALID_HEIGHT, INVALID_WIDTH };

  static Tetris standardTetris() { return Tetris(10, 40); }

  static std::expected<Tetris, InputError>
  createTetris(int width, int height, ShapeFactory factory = ShapeFactory()) {
    auto breachesLimit = [&](auto var) {
      return std::ranges::any_of(factory.defaultShapes,
                                 [var](auto x) { return x.size > var; });
    };
    if (breachesLimit(width)) {
      return std::unexpected(InputError::INVALID_WIDTH);
    } else if (breachesLimit(height)) {
      return std::unexpected(InputError::INVALID_HEIGHT);
    }
    return Tetris(width, height);
  }

  auto getLevel() const { return level; }
  auto getScore() const { return score; }

  void handleInput(Input input) {
    std::visit(overloaded{[this](Direction direction) { move(direction); },
                          [this](Key key) { handleKey(key); },
                          [this](Rotation rotation) { rotate(rotation); }},
               input);
  }

  friend std::ostream &operator<<(std::ostream &stream, Tetris &tetris);
  friend std::vector<std::string> outputRows(Tetris &tetris);
};

std::vector<std::string> outputRows(Tetris &tetris) {
  auto copy = tetris.cells;
  for (auto c :
       Tetris::absShapeCoords(tetris.shapeLocation, tetris.currentShape)) {
    copy[tetris.height - 1 - c.y][c.x] = true;
  }

  std::vector<std::string> rows;

  for (auto r : std::ranges::drop_view(copy, tetris.height - 20)) {
    std::ostringstream out;
    for (auto x : r) {
      out << x << ' ';
    }
    rows.push_back(out.str());
  }
  return rows;
}

std::ostream &operator<<(std::ostream &stream, Tetris &tetris) {
  auto copy = tetris.cells;
  for (auto c :
       Tetris::absShapeCoords(tetris.shapeLocation, tetris.currentShape)) {
    copy[tetris.height - 1 - c.y][c.x] = true;
  }
  for (auto r : std::ranges::drop_view(copy, tetris.height - 20)) {
    for (bool x : r) {
      stream << x << " ";
    }
    stream << std::endl;
  }
  return stream;
}

#endif