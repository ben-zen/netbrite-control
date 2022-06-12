#pragma once

namespace nbx
{
enum class text_color : unsigned char
{
  red = 1,
  green = 2,
  yellow = 3
};

enum class scroll_speed : unsigned char
{
  slow = 1,
  medium = 2,
  fast = 3
};

enum class priority : unsigned char
{
  override = 1,
  interrupt = 2,
  follow = 3,
  yield = 4,
  round_robin = 10
};

enum class text_font : unsigned char
{
  monospace_16 = 0,
  proportional_7 = 1,
  proportional_5 = 2,
  proportional_11 = 3,
  monospace_24 = 4,
  bold_proportional_7 = 5,
  bold_proportional_11 = 6,
  monospace_7 = 7,
  script_16 = 8,
  proportional_9 = 9,
  picture_24 = 10
};
};