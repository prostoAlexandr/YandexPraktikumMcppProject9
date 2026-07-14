#include "mandelbrot_sender.hpp"
#include "types_core.hpp"
#include "types_sfml.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <SFML/Config.hpp>
#include <stdexec/__detail/__execution_fwd.hpp>

namespace ex = stdexec;

struct MockCounter {
    MOCK_METHOD(void, value_set, (), (noexcept));
    MOCK_METHOD(void, error_set, (), (noexcept));
};

struct MockReceiver {
    using receiver_concept = ex::receiver_t;
    MockCounter &mock_counter;

    MockReceiver(MockCounter &mc) : mock_counter(mc) {}

    void set_value(FrameBuffer *fb) noexcept { mock_counter.value_set(); }

    template <typename Error>
    void set_error(Error &&error) noexcept {
        mock_counter.error_set();
    }
};

TEST(MandelbrotSenderTest, ReceiverTest) {
    RenderSettings settings{.width = 800, .height = 600, .max_iterations = 200, .escape_radius = 4.0};
    ViewPort viewport = AppState::INITIAL_VIEWPORT;
    auto sender = mandelbrot::MakeComputeSender(settings, viewport);
    MockCounter mc{};
    FrameBuffer fb = FrameBuffer::Make(settings.width, settings.height);
    auto state = ex::connect(ex::just(&fb) | std::move(sender), MockReceiver{mc});
    EXPECT_CALL(mc, value_set).Times(1);
    EXPECT_CALL(mc, error_set).Times(0);
    ex::start(state);
}

TEST(MandelbrotSenderTest, ResultTest) {
    RenderSettings settings{.width = 2, .height = 2, .max_iterations = 200, .escape_radius = 4.0};
    ViewPort viewport = AppState::INITIAL_VIEWPORT;
    auto sender = mandelbrot::MakeComputeSender(settings, viewport);
    FrameBuffer fb = FrameBuffer::Make(settings.width, settings.height);
    auto result = ex::sync_wait(ex::just(&fb) | std::move(sender));
    ASSERT_TRUE(result.has_value());
    auto& fb_result = std::get<0>(result.value());
    EXPECT_EQ(fb_result, &fb);
    EXPECT_EQ(fb_result->height, 2);
    EXPECT_EQ(fb_result->width, 2);
    EXPECT_EQ(fb_result->rgba, std::vector<sf::Uint8>(
                           {0xFF, 0xF, 0x0, 0xFF, 0xFF, 0xF, 0x0, 0xFF, 0xFF, 0x16, 0x0, 0xFF, 0x0, 0x0, 0x0, 0xFF}));
}