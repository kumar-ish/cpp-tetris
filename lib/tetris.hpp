#ifndef TETRIS
#define TETRIS

#include <algorithm>
#include <expected>
#include <functional>
#include <iostream>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

#include "helper.hpp"

enum class Key { HOLD, SPACE };
enum class Rotation { CLOCKWISE, COUNTER_CLOCKWISE };
enum class Direction { DOWN, LEFT, RIGHT };

struct Coord;
struct Shape;
using Coords = std::vector<Coord>;

// Generic type to be able to get shapes
template <typename T>
concept ShapeFactory = requires(T s) {
  { s.getShape() } -> std::same_as<const Shape>;
  { s.getShapes() } -> std::same_as<const std::vector<const Shape>>;
};

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

  // It's safe to do this as the relative order in the implicitly public members
  // of `Coord` is preserved
  auto operator<=>(const Coord &other) const = default;
  Coord operator+(const Coord &other) const {
    return {x + other.x, y + other.y};
  }
  Coord operator+(Direction direction) const {
    return *this + Coord::directionCoord(direction);
  }
  Coord operator*(int mul) const { return {x * mul, y * mul}; }
};

struct Shape {
  using KickData = std::array<std::array<Coord, 4>, 4>;
  Shape(int _size, const Coords &_coords,
        std::optional<KickData> _kickData = std::nullopt,
        int _rotationIndex = 0)
      : size(_size), coords(_coords), kickData(_kickData),
        rotationIndex(_rotationIndex) {
    if (_size <= 0) {
      throw std::invalid_argument(
          "size cannot be non-positive (must be greater than or equal to 1)");
    }
    if (auto res = std::ranges::find_if(
            _coords, [_size](auto &c) { return not c.inBounds(_size, _size); });
        res != _coords.end()) {
      throw std::invalid_argument(
          std::format("the coord ({}, {}) doesn't fit in the size {}", res->x,
                      res->y, _size));
    }
    if (_rotationIndex > 3 or _rotationIndex < 0) {
      throw std::invalid_argument(std::format(
          "rotation index {} not in bounds (must be between 0 and 3 inclusive)",
          _rotationIndex));
    }
  }

  std::string name;
  int size;
  Coords coords;
  std::optional<KickData> kickData;
  int rotationIndex;

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
    auto clockwiseRotate = [this](const auto &coord) -> Coord {
      return {coord.y, size - 1 - coord.x};
    };
    return {
        size,
        transformCoords(clockwiseRotate),
        kickData,
        (rotationIndex + 1) % 4,
    };
  }

  Shape rotateCounterClockwise() const {
    auto counterClosewiseRotate = [this](const auto &coord) -> Coord {
      return {size - 1 - coord.y, coord.x};
    };
    // C++'s modulo operator can return <0 numbers, so we add 4
    return {size, transformCoords(counterClosewiseRotate), kickData,
            (rotationIndex + 4 - 1) % 4};
  }
};

class StandardShapeFactory {
public:
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
  static inline const Shape I_BLOCK{
      4, {{0, 1}, {1, 1}, {2, 1}, {3, 1}}, StandardShapeFactory::I_KICKDATA};

  static inline const Shape T_BLOCK{3,
                                    {{0, 0}, {0, 1}, {0, 2}, {1, 1}},
                                    StandardShapeFactory::TLJSZ_KICKDATA};
  static inline const Shape L_BLOCK{3,
                                    {{0, 0}, {0, 1}, {0, 2}, {1, 2}},
                                    StandardShapeFactory::TLJSZ_KICKDATA};
  static inline const Shape J_BLOCK{3,
                                    {{1, 0}, {1, 1}, {1, 2}, {0, 2}},
                                    StandardShapeFactory::TLJSZ_KICKDATA};
  static inline const Shape S_BLOCK{3,
                                    {{0, 1}, {0, 2}, {1, 0}, {1, 1}},
                                    StandardShapeFactory::TLJSZ_KICKDATA};
  static inline const Shape Z_BLOCK{3,
                                    {{1, 1}, {1, 2}, {0, 0}, {0, 1}},
                                    StandardShapeFactory::TLJSZ_KICKDATA};

  static inline const Shape O_BLOCK{3, {{1, 1}, {1, 2}, {2, 1}, {2, 2}}};
  static inline const std::vector<const Shape> defaultShapes{
      StandardShapeFactory::I_BLOCK, StandardShapeFactory::O_BLOCK,
      StandardShapeFactory::T_BLOCK, StandardShapeFactory::L_BLOCK,
      StandardShapeFactory::J_BLOCK, StandardShapeFactory::S_BLOCK,
      StandardShapeFactory::Z_BLOCK};

  const std::vector<const Shape> getShapes() const { return defaultShapes; }

  const Shape getShape() const {
    return defaultShapes[rand() % defaultShapes.size()];
  };
};

using Input = std::variant<Direction, Key, Rotation>;

template <ShapeFactory Factory> class Tetris {
private:
  // tetris board
  std::vector<std::vector<bool>> cells;

  const Factory factory;
  Shape currentShape{factory.getShape()};
  Coord shapeLocation;

public:
  const int width;
  const int height;

private:
  std::optional<Shape> holdShape = std::nullopt;
  // If you've held in the turn already
  bool heldInTurn = false;

  int level{1};
  int score{0};
  [[maybe_unused]] double speed{1.0};

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
    return std::ranges::all_of(
        coords, [this](const Coord &c) { return c.inBounds(width, height); });
  }

  bool shapeBlocked(Coord location, const Shape &shape) const {
    if (not shapeInBounds(location, shape)) {
      return true;
    }

    auto coords = absShapeCoords(location, shape);
    return std::ranges::any_of(coords, std::bind_front(&Tetris::cellAt, this));
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

    std::ranges::replace_if(
        removedRange, [](auto) { return true; },
        std::vector<bool>(width, false));

    return (int)removedRange.size();
  }

  // Move the current shape a particular direction
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

    // We've placed the existing shape, so we replace it
    currentShape = factory.getShape();

    // Based on the coordinates of the new shape, we reset its location and
    // reset whether a hold has happened
    resetShapeLocation();
    heldInTurn = false;

    // We clear any lines at the bottom
    clear();
    return true;
  }

  // Rotates the current shape clockwise or anti-clockwise
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
      // If the shape isn't blocked on rotation, we simply rotate
      currentShape = std::move(rotatedShape);
      return;
    } else if (currentShape.kickData->empty()) {
      // If the shape doesn't have any kickdata, and it can't be rotated
      // normally, we do nothing
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

  // Put shape in hold state
  void hold() {
    if (heldInTurn) {
      // If you've held already this turn, a hold operation shouldn't be
      // actionable
      return;
    }
    if (holdShape.has_value()) {
      auto temp = std::move(currentShape);
      resetShape(holdShape.value());
      holdShape = std::move(temp);
    } else {
      holdShape = std::move(currentShape);
      resetShape(factory.getShape());
    }
    heldInTurn = true;
  }

  void handleKey(Key key) {
    switch (key) {
    case Key::HOLD: {
      hold();
      break;
    }
    case Key::SPACE: {
      // keep moving down until materialization
      while (true) {
        auto materialized = move(Direction::DOWN);
        if (materialized)
          break;
      }
    }
    }
  }

  explicit Tetris(int _width, int _height, Factory _factory)
      : factory{std::move(_factory)}, width{_width}, height{_height} {
    resetShapeLocation();
    cells = std::vector(height, std::vector<bool>(width));
  }

public:
  enum class InputError { INVALID_HEIGHT, INVALID_WIDTH };

  static Tetris<StandardShapeFactory> standardTetris() {
    return Tetris(10, 40);
  }

  static std::expected<Tetris, InputError>
  createTetris(int width, int height,
               Factory factory = StandardShapeFactory()) {
    auto breachesLimit = [&](auto var) {
      return std::ranges::any_of(factory.getShapes(),
                                 [var](auto x) { return x.size > var; });
    };
    if (breachesLimit(width)) {
      return std::unexpected(InputError::INVALID_WIDTH);
    } else if (breachesLimit(height)) {
      return std::unexpected(InputError::INVALID_HEIGHT);
    }
    return Tetris(width, height, factory);
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

  std::vector<std::string> outputRows() {
    auto copy = cells;
    for (auto c :
         Tetris<Factory>::absShapeCoords(shapeLocation, currentShape)) {
      copy[height - 1 - c.y][c.x] = true;
    }

    std::vector<std::string> rows;

    for (auto r : std::ranges::drop_view(copy, height - 20)) {
      std::ostringstream out;
      for (auto x : r) {
        out << x << ' ';
      }
      rows.push_back(out.str());
    }
    return rows;
  }
};

template <ShapeFactory Factory>
std::ostream &operator<<(std::ostream &stream, Tetris<Factory> &tetris) {
  auto copy = tetris.cells;
  for (auto c : Tetris<Factory>::absShapeCoords(tetris.shapeLocation,
                                                tetris.currentShape)) {
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

struct TetrisFactory {
  static Tetris<StandardShapeFactory> standardTetris() {
    return Tetris<StandardShapeFactory>::createTetris(10, 40).value();
  }

  static auto defaultTetris(const ShapeFactory auto &t) {
    return Tetris<decltype(t)>::createTetris(10, 40, t).value();
  }
};

#endif