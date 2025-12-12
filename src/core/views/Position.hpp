#ifndef POSITION_HPP
#define POSITION_HPP

namespace core
{
  namespace view
  {

    struct Position
    {
      float x = 0.0f;
      float y = 0.0f;

      Position() = default;
      Position(float x, float y) : x(x), y(y) {}
    };

    struct Size
    {
      float x = 0.0f;
      float y = 0.0f;

      Size() = default;
      Size(float width, float height) : x(width), y(height) {}
    };

    struct Scroll
    {
      float x = 0.0f;
      float y = 0.0f;
      Position to;

      Scroll() = default;
    };

  } // namespace view
} // namespace core

#endif // POSITION_HPP
