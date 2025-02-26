#pragma once

#include "BgrColor.hpp"
#include <array>

static constexpr auto NumberOfFeedbackLeds = 12;

/// array which should be filled by user/animation class with data
using LedSegmentArray = std::array<BgrColor, NumberOfFeedbackLeds>;