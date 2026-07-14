#include <chrono>
#include <memory>
#include <print>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <system_error>
#include <thread>
#include <utility>

#include <SFML/Graphics.hpp>

#include <exec/any_sender_of.hpp>
#include <exec/repeat_until.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "mandelbrot_sender.hpp"
#include "sfml_display_sender.hpp"
#include "sfml_events_handler.hpp"
#include "types_sfml.hpp"

using namespace std::chrono_literals;
namespace ex = stdexec;

class WaitForFPS {
public:
    static constexpr float TARGET_FPS = 60.0f;
    static constexpr float FRAME_TIME_MS = 1000.0f / TARGET_FPS;

    explicit WaitForFPS(FrameClock &frame_clock, unsigned int target_fps)
        : frame_clock_(frame_clock), frame_time_(1'000'000us / target_fps) {}

    void operator()() {
        auto cur_frame_duration = frame_clock_.GetFrameTime();

        if (cur_frame_duration < frame_time_) {
            std::this_thread::sleep_for(frame_time_ - cur_frame_duration);
        }
        frame_clock_.Reset();
    }

private:
    FrameClock &frame_clock_;
    const std::chrono::microseconds frame_time_ = 1ms;
};

template <class... CompletionSigs>
using any_sender_of =
  exec::any_sender<exec::any_receiver<stdexec::completion_signatures<CompletionSigs...>>>;

class MandelbrotApp {
public:
    MandelbrotApp() : compute_pool_{std::max(1u, std::thread::hardware_concurrency())}, sfml_thread_{1} {
        std::println("hardware_concurrency: {}\n", std::thread::hardware_concurrency());
    }

    void Run() {
        auto compute_sched = compute_pool_.get_scheduler();
        auto sfml_sched = sfml_thread_.get_scheduler();

        auto initialize =
            ex::schedule(sfml_sched) | ex::then([this]() {
                state_ = std::make_unique<SfmlState>(  //
                    RenderSettings{.width = 800, .height = 600, .max_iterations = 200, .escape_radius = 4.0});
            });
        ex::sync_wait(std::move(initialize));

        using conditional_sender =
        any_sender_of<ex::set_value_t(FrameBuffer *), ex::set_error_t(std::error_code)>;
        // clang-format off
        auto process_frame =
            ex::schedule(sfml_sched) |
            ex::let_value([this] {
                return SfmlEventHandler(state_->window, state_->render_settings, state_->app_state);
            }) |
            ex::let_value([this] -> conditional_sender {
                if (state_->app_state.need_rerender) {
                    state_->app_state.need_rerender = false;
                    return ex::just(&state_->fb);
                }
                // Use error to go to the pipeline end to keep the structure flat
                return ex::just_error(std::make_error_code(std::errc::broken_pipe));
            }) |
            ex::continues_on(compute_sched) |
            ex::let_value([this](FrameBuffer *buf) {
                return ex::just(buf) |
                    // Get updated viewport on each cycle
                    mandelbrot::MakeComputeSender(state_->render_settings, state_->app_state.viewport);
            }) |
            ex::continues_on(sfml_sched) |
            render::MakeSfmlDisplaySender(*state_) |
            ex::upon_error([](auto err) {}) |
            ex::then([this] { WaitForFPS{state_->frame_clock, static_cast<unsigned int>(WaitForFPS::TARGET_FPS)}(); });

            auto repeated_pipeline = std::move(process_frame) |
                ex::then([this] { return state_->app_state.should_exit; }) |
                exec::repeat_until();
        // clang-format on
        ex::sync_wait(std::move(repeated_pipeline));

        auto deinitialize =
            ex::schedule(sfml_sched) | ex::then([this]() {
                state_->window.close();
                state_.reset();
            });
        ex::sync_wait(std::move(deinitialize));
    }

private:
    std::unique_ptr<SfmlState> state_;

    exec::static_thread_pool compute_pool_;
    exec::static_thread_pool sfml_thread_;
};

int main() {
    std::println("=== Mandelbrot Fractal Renderer ===\n");
    std::println("Controls:");
    std::println("  Left Mouse Button  - Zoom In");
    std::println("  Right Mouse Button - Zoom Out");
    std::println("  X                  - Toggle Auto Zoom (infinite zoom to 'Seahorse Valley' point)");
    std::println("  C                  - Reset to Initial View");
    std::println("  Close Window       - Exit\n");

    try {
        MandelbrotApp app;
        app.Run();
    } catch (const std::exception &e) {
        std::println("Error: {}", e.what());
        return 1;
    }
    return 0;
}