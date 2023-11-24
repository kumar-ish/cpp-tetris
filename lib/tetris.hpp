#include <expected>
#include <iostream>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "helper.hpp"

#include "all_headers.h"

enum class Key { HOLD, SPACE };
enum class Rotation { CLOCKWISE, COUNTER_CLOCKWISE };
enum class Direction { DOWN, LEFT, RIGHT };

struct Coord {
  int x;
  int y;

  Coord() = default;
  constexpr Coord(int _x, int _y) : x(_x), y(_y) {}

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

  bool operator<(const Coord &other) {
    // We sort by x, then y
    return this->x == other.x ? this->y < other.y : this->x < other.x;
  }
  Coord moveDirection(Direction direction) const {
    return *this + Coord::directionCoord(direction);
  }

  Coord operator+(const Coord other) const {
    return Coord(x + other.x, y + other.y);
  }
  Coord operator*(int mul) const { return Coord(x * mul, y * mul); }
};

class Shape {
  using KickData = std::array<std::array<Coord, 4>, 4>;

  Shape(int _size, std::vector<Coord> _coords, int _rotationIndex = 0,
        std::optional<KickData> _kickData = std::nullopt)
      : size(_size), coords(_coords), rotationIndex(_rotationIndex),
        kickData(_kickData) {
    if (_size <= 0) {
      throw std::invalid_argument(
          "size cannot be non-positive (must be greater than or equal to 1)");
    }
    if (std::any_of(_coords.cbegin(), _coords.cend(), [_size](auto &c) {
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
  std::vector<Coord> coords;
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

  std::vector<Coord>
  transformCoords(std::function<Coord(const Coord &)> f) const {
    std::vector<Coord> newCoords;
    newCoords.reserve(coords.size());

    std::transform(coords.cbegin(), coords.cend(),
                   std::back_inserter(newCoords), f);
    return newCoords;
    // return std::ranges::transform_view(coords, f) |
    //        std::ranges::to<std::vector<Coord>>();
    // ^ doesn't work because c++23 support lacking
  }
  Shape rotateClockwise() const {
    auto rotate = [this](const auto &coord) {
      return Coord({coord.y, size - 1 - coord.x});
    };
    return Shape(size, transformCoords(rotate), (rotationIndex + 1) % 4,
                 kickData);
  }

  Shape rotateCounterClockwise() const {
    auto rotate = [this](const auto &coord) {
      return Coord({size - 1 - coord.y, coord.x});
    };
    // C++'s modulo operator can return <0 numbers, so we add 4
    return Shape(size, transformCoords(rotate), (rotationIndex + 4 - 1) % 4,
                 kickData);
  }
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

  const std::vector<Shape> getShapes() const { return defaultShapes; }

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
                     height - currentShape.size};
  }

  std::vector<Coord> absShapeCoords(const Coord &location,
                                    const Shape &shape) const {
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
    auto lastIter =
        std::remove_if(cells.rbegin(), cells.rend(), [](auto const &c) {
          return std::all_of(c.begin(), c.end(), std::identity());
        });
    int numCleared = 0;
    while (lastIter != cells.rend()) {
      *lastIter = std::vector<bool>(width, false);
      numCleared++;
      lastIter++;
    }

    return numCleared;
  }

  bool move(Direction direction) {
    auto movedLocation = shapeLocation.moveDirection(direction);

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
    resetShapeLocation();
    currentShape = factory.getRandomShape();
    heldInTurn = false;
    clear();
    return true;
  }

  void rotate(Rotation rotation) {
    auto rotatedShape = [rotation, this]() {
      switch (rotation) {
      case Rotation::CLOCKWISE:
        return currentShape.rotateClockwise();
      case Rotation::COUNTER_CLOCKWISE:
        return currentShape.rotateCounterClockwise();
      default:
        std::unreachable();
      }
    }();

    if (not shapeBlocked(shapeLocation, rotatedShape)) {
      currentShape = rotatedShape;
      return;
    } else if (currentShape.kickData->empty()) {
      return;
    }

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

public:
  explicit Tetris(int _width, int _height) : width(_width), height(_height) {
    resetShapeLocation();
    cells = std::vector<std::vector<bool>>(height, std::vector<bool>(width));
  }

  static Tetris standardTetris() { return Tetris(10, 40); }

  auto getLevel() const { return level; }
  auto getScore() const { return score; }

  using Input = std::variant<Direction, Key, Rotation>;
  void handleInput(Input input) {
    std::visit(overloaded{[this](Direction direction) { move(direction); },
                          [this](Key key) { handleKey(key); },
                          [this](Rotation rotation) { rotate(rotation); }},
               input);
  }

  void print() const {
    auto copy = cells;
    for (auto c : absShapeCoords(shapeLocation, currentShape)) {
      copy[height - 1 - c.y][c.x] = true;
    }
    for (auto r : std::ranges::drop_view(copy, height - 20)) {
      for (bool x : r) {
        std::cout << x << " ";
      }
      std::cout << std::endl;
    }
    std::cout << "---------------" << std::endl;
  }
};