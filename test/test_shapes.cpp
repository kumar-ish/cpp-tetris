#include <iterator>

#include "ApprovalTests.hpp"
#include "catch2/catch.hpp"

#include "../lib/tetris.hpp"

std::ostream &operator<<(std::ostream &stream, Shape &shape) {
  std::vector<Coord> coordsCopy{shape.coords};
  std::sort(coordsCopy.begin(), coordsCopy.end());

  auto cIter = coordsCopy.begin();
  for (int i = 0; i < shape.size; i++) {
    for (int j = 0; j < shape.size; j++) {
      char out = '.';
      // If the current coordinate in the shape is what we're looking at, then
      // we make the output character to be 'o' -- a space-taking block
      if (cIter != coordsCopy.end() && cIter->x == i && cIter->y == j) {
        out = 'o';
        cIter++;
      }
      stream << out;
    }
    stream << '\n';
  }
  return stream;
}

TEST_CASE("Shapes Test") {
  auto factory = ShapeFactory{};
  auto printShape = [](auto xd, std::ostream &os) { os << xd; };

  ApprovalTests::Approvals::verifyAll("SHAPES", factory.defaultShapes,
                                      printShape);
}