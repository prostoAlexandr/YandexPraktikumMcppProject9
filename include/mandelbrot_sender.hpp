#pragma once

#include "mandelbrot_fractal_utils.hpp"
#include "types_sfml.hpp"
#include <print>

#include <stdexec/execution.hpp>

using namespace std::chrono_literals;
namespace ex = stdexec;

namespace mandelbrot {

static auto MakeComputeSender(RenderSettings settings, ViewPort viewport) {
    static AvrTimeCounter time_counter;
    static constexpr double R = 2.0;
    static constexpr uint32_t n = 100;

    return ex::then([&](FrameBuffer *fb) {
               time_counter.Start();
               for (uint32_t x = 0; x < fb->width; ++x) {
                   for (uint32_t y = 0; y < fb->height; ++y) {
                       auto comp = mandelbrot::Pixel2DToComplex(x, y, viewport, fb->width, fb->height);
                       auto iterations = mandelbrot::CalculateIterationsForPoint(comp, n, R);
                       if (iterations < n) {
                           continue;
                       }

                       const size_t index = (y * fb->width + x) * 4;
                       fb->rgba[index] = 0;
                       fb->rgba[index + 1] = 0;
                       fb->rgba[index + 2] = 0;
                       fb->rgba[index + 3] = 0;
                   }
               }
               return fb;
           }) |
           ex::then([](FrameBuffer *fb) {
               time_counter.End();
               if (time_counter.Count() % 10 == 0) {
                   std::println("\nAverage compute time: {} ms over {} frames", time_counter.GetAvr(),
                                time_counter.Count());
               }
               return fb;
           });
}

}  // namespace mandelbrot
