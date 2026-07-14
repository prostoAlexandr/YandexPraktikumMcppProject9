#pragma once

#include "mandelbrot_fractal_utils.hpp"
#include "types_sfml.hpp"
#include <print>

#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>

using namespace std::chrono_literals;
namespace ex = stdexec;

namespace mandelbrot {

static auto MakeComputeSender(RenderSettings settings, ViewPort viewport) {
    static AvrTimeCounter time_counter;

    return ex::then([](FrameBuffer *fb) {
               time_counter.Start();
               return fb;
           }) |
           ex::bulk(ex::par, settings.height,
                    [settings, viewport](uint32_t idx, FrameBuffer *fb) {
                        for (uint32_t x = 0; x < settings.width; ++x) {
                            auto comp = mandelbrot::Pixel2DToComplex(x, idx, viewport, settings.width, settings.height);
                            auto iterations = mandelbrot::CalculateIterationsForPoint(comp, settings.max_iterations,
                                                                                      settings.escape_radius);
                            auto color = mandelbrot::IterationsToColor(iterations, settings.max_iterations);
                            const size_t index = (idx * settings.width + x) * 4;

                            fb->rgba[index] = color.r;
                            fb->rgba[index + 1] = color.g;
                            fb->rgba[index + 2] = color.b;
                            fb->rgba[index + 3] = 0xff;
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
