#include "ApprovalTests.hpp"
#include "catch2/catch.hpp"
#include <ranges>
#include <sstream>
#include <tuple>

#include "../lib/tetris.hpp"
#include <vector>

std::ostringstream &operator<<(std::vector<std::string> &v,
                               std::ostringstream &stream) {
  for (auto x : v) {
    stream << x;
  }
  return stream;
}

auto zip(auto sets) {
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

std::string interleaveRows(const std::string &delimiter,
                           std::vector<std::vector<std::string>> rs) {
  std::ostringstream out;

  int x = 0;
  auto chunkFunc = [&x](auto, auto) {
    return !(++x % 3 == 0);
  };

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

TEST_CASE("TetrisBasic") {
  auto doInputs = [](auto &&...ts) {
    auto tetris = Tetris::standardTetris();

    std::vector<std::vector<std::string>> res;
    res.push_back(outputRows(tetris));
    for (auto x : {ts...}) {
      tetris.handleInput(x);
      res.push_back(outputRows(tetris));
    }

    std::stringstream os;
    os << interleaveRows("| ", res);
    return std::move(os);
  };
  auto passStream = [](auto &x, std::ostream &os) { os << x.str(); };

  SECTION("TwoDowns") {
    ApprovalTests::Approvals::verify(doInputs(Direction::DOWN, Direction::DOWN),
                                     passStream);
  }
  SECTION("FourCWRotates") {
    ApprovalTests::Approvals::verify(
        doInputs(Rotation::CLOCKWISE, Rotation::CLOCKWISE, Rotation::CLOCKWISE,
                 Rotation::CLOCKWISE),
        passStream);
  }
}
