#include "ApprovalTests.hpp"
#include "catch2/catch.hpp"
#include <functional>
#include <ranges>
#include <sstream>
#include <tuple>
#include <variant>
#include <vector>

#include "../lib/helper.hpp"
#include "../lib/tetris.hpp"

enum class AdditionalOps { LEFTMOST, RIGHTMOST, SNAP };

std::ostringstream &operator<<(std::vector<std::string> &v,
                               std::ostringstream &stream) {
  for (auto x : v) {
    stream << x;
  }
  return stream;
}

// Zip 2D vectors of strings together
// i.e. [[a, b, c], [d, e, f], [g, h, i]] -> [[a, d, g], [b, e, h], [c, f, i]]
auto zip(std::vector<std::vector<std::string>> sets) {
  std::vector<std::vector<std::string>> ret;

  for (int i = 0; i < (int)sets[0].size(); i++) {
    std::vector<std::string> row;
    row.reserve(sets.size());
    for (int j = 0; j < (int)sets.size(); j++) {
      row.push_back(sets[j][i]);
    }
    ret.push_back(row);
  }

  return ret;
}

// Zips groups of three Tetris string representations together
template <int groupSize>
std::string interleaveRows(const std::string &delimiter,
                           std::vector<std::vector<std::string>> rs) {
  std::ostringstream out;

  int x = 0;
  auto chunkFunc = [&x](auto, auto) { return !(++x % groupSize == 0); };

  for (auto group : rs | std::views::chunk_by(chunkFunc)) {
    for (auto y :
         zip(group |
             std::ranges::to<std::vector<std::vector<std::string>>>())) {
      for (auto x : y) {
        out << x << delimiter;
      }
      out << std::endl;
    }
    out << std::string(group.size() * (group[0].size() + 2) - 1, '-')
        << std::endl;
  }

  return out.str();
}

// Handle basic inputs as handled by Tetris
auto doInputs = []<int groupSize = 3>(auto &&...ts) {
  auto tetris = TetrisFactory::standardTetris();

  std::vector<std::vector<std::string>> res;
  res.push_back(tetris.outputRows());
  for (auto x : {ts...}) {
    tetris.handleInput(x);
    res.push_back(tetris.outputRows());
  }

  std::stringstream os;
  os << interleaveRows<groupSize>("| ", res);
  return std::move(os);
};
auto passStream = [](auto &x, std::ostream &os) { os << x.str(); };

// Handle special shape factory and basic inputs as handled by Tetris, and
// additional ones
auto refinedInputs = []<int groupSize = 3>(auto factory, auto &&...ts) {
  auto tetris = TetrisFactory::defaultTetris(factory);

  std::vector<std::vector<std::string>> res;
  for (auto op : {variant_union<Input, AdditionalOps>::type(ts)...}) {
    std::visit(overloaded{[&](AdditionalOps stuff) {
                            switch (stuff) {
                            case AdditionalOps::SNAP:
                              res.push_back(tetris.outputRows());
                              break;
                            case AdditionalOps::LEFTMOST:
                            case AdditionalOps::RIGHTMOST:
                              auto operation =
                                  (stuff == AdditionalOps::LEFTMOST)
                                      ? Direction::LEFT
                                      : Direction::RIGHT;
                              for (int i = 0; i < tetris.width; i++) {
                                tetris.handleInput(operation);
                              }
                              break;
                            }
                          },
                          [&](auto xd) { tetris.handleInput(xd); }},
               op);
  }

  std::stringstream os;
  os << interleaveRows<groupSize>("| ", res);
  return std::move(os);
};

TEST_CASE("TetrisBasic") {
  SECTION("TwoDowns") {
    ApprovalTests::Approvals::verify(doInputs(Direction::DOWN, Direction::DOWN),
                                     passStream);
  }
  SECTION("FourCWRotates") {
    ApprovalTests::Approvals::verify(
        doInputs.template operator()<4>(
            Rotation::CLOCKWISE, Rotation::CLOCKWISE, Rotation::CLOCKWISE,
            Rotation::CLOCKWISE),
        passStream);
  }
}

TEST_CASE("TetrisClear") {
  SECTION("CanClear1") {
    // Use I blocks since they're the easiest to get a clear with
    struct IBlockFactory {
      const Shape getShape() const { return StandardShapeFactory::I_BLOCK; }

      const std::vector<const Shape> getShapes() const {
        return {StandardShapeFactory::I_BLOCK};
      }
    };

    ApprovalTests::Approvals::verify(
        refinedInputs.template operator()<4>(
            IBlockFactory(), // factory
            AdditionalOps::LEFTMOST, Key::SPACE,
            AdditionalOps::SNAP, // Go leftmost
            AdditionalOps::RIGHTMOST, Direction::LEFT, Direction::LEFT,
            Key::SPACE,
            AdditionalOps::SNAP, // Go two left of rightmost
            Rotation::CLOCKWISE, AdditionalOps::RIGHTMOST, Direction::LEFT,
            Key::SPACE, AdditionalOps::SNAP, Rotation::CLOCKWISE,
            AdditionalOps::RIGHTMOST, Key::SPACE,
            AdditionalOps::SNAP // Put down two vertical bricks
            ),
        // Should clear bottom-most level
        passStream);
  }
}

TEST_CASE("TetrisHold") {
  SECTION("CanHold") {
    // Get a block factory that simply iterates through the standard one, so we
    // can test holding with unique shapes each time
    struct IterateBlockFactory {
      mutable int i = 0;
      const Shape getShape() const {
        return StandardShapeFactory::defaultShapes[i++];
      }

      const std::vector<const Shape> getShapes() const {
        return StandardShapeFactory::defaultShapes;
      }
    };

    ApprovalTests::Approvals::verify(
        refinedInputs.template operator()<5>(
            IterateBlockFactory(),
            AdditionalOps::SNAP,            // Initial
            Key::HOLD, AdditionalOps::SNAP, // New state
            Key::HOLD,
            AdditionalOps::SNAP, // Stay the same, can't hold multiple times
            Key::SPACE, AdditionalOps::SNAP, // New shape
            Key::HOLD,
            AdditionalOps::SNAP // New shape, should be the same as the first
                                // shape
            ),
        passStream);
  }
}
