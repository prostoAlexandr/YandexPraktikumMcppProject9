#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <exception>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/__detail/__receivers.hpp>
#include <stdexec/execution.hpp>

#include "types_core.hpp"
#include "types_sfml.hpp"

namespace ex = stdexec;

class SfmlEventHandler {
public:

    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        sf::RenderWindow &window_;
        RenderSettings render_settings_;
        AppState &state_;

        static constexpr float ZOOM_INTERVAL_MS = 100.0f;
        using operation_state_concept = stdexec::operation_state_t;

        template <typename R>
        explicit OperationState(R &&r, sf::RenderWindow &window, RenderSettings render_settings, AppState &state)
            : receiver_{std::forward<R>(r)}, window_{window}, render_settings_{render_settings}, state_{state} {}

        /* Ваш код метода start() здесь */
        void start() noexcept {
            try {
                HandleEvents();
                if (state_.should_exit)
                {
                    ex::set_stopped(std::move(receiver_));
                }
                ex::set_value(std::move(receiver_));
            } catch (...) {
                ex::set_error(std::move(receiver_), std::current_exception());
            }
        }

    private:
        void HandleEvents() {
            sf::Event event;
            while (window_.pollEvent(event)) {
                switch (event.type) {
                case sf::Event::Closed:
                state_.should_exit = true;
                break;

                /* Ваш код здесь  */
                case sf::Event::KeyPressed:
                    HandleKeyPress(event.key);
                    break;
                case sf::Event::MouseButtonPressed:
                    HandleMousePress(event.mouseButton);
                    break;
                case sf::Event::MouseButtonReleased:
                    HandleMouseRelease(event.mouseButton);
                default:
                    break;
                }
            }
            HandleAutoZoom();
        }

        void HandleKeyPress(const sf::Event::KeyEvent &key) {
            /* Ваш код здесь */
            switch (key.code) {
            case sf::Keyboard::X:
                if (!state_.auto_zoom_enabled) {
                    state_.auto_zoom_enabled = true;
                }else{
                    state_.auto_zoom_enabled = false;
                }
            case sf::Keyboard::C:
                state_.viewport = AppState::INITIAL_VIEWPORT;
                state_.auto_zoom_enabled = false;
            }
        }

        void HandleMousePress(const sf::Event::MouseButtonEvent &mouse) {
            /* Ваш код здесь */
            if (mouse.button == sf::Mouse::Left && !state_.left_mouse_pressed) {
                state_.left_mouse_pressed = true;
                ZoomToPoint(mouse.x, mouse.y, true);
            } else if (mouse.button == sf::Mouse::Right && !state_.right_mouse_pressed) {
                state_.right_mouse_pressed = true;
                ZoomToPoint(mouse.x, mouse.y, false);
            }
        }

        void HandleMouseRelease(const sf::Event::MouseButtonEvent &mouse) {
            if (mouse.button == sf::Mouse::Left) {
                state_.left_mouse_pressed = false;
            } else if (mouse.button == sf::Mouse::Right) {
                state_.right_mouse_pressed = false;
            }
        }

        void HandleAutoZoom() {
            /* Ваш код здесь */
            if (state_.auto_zoom_enabled)
            {
                ZoomToPoint(AppState::AUTO_ZOOM_TARGET_X, AppState::AUTO_ZOOM_TARGET_Y, true);
            }
        }

        void ZoomToPoint(int pixel_x, int pixel_y, bool zoom_in, double factor = 0.8) {
            const double target_x = state_.viewport.x_min +
                                    (static_cast<double>(pixel_x) / render_settings_.width) * state_.viewport.width();
            const double target_y = state_.viewport.y_min +
                                    (static_cast<double>(pixel_y) / render_settings_.height) * state_.viewport.height();

            const double zoom_factor = zoom_in ? factor : (1.0 / factor);
            const double new_width = state_.viewport.width() * zoom_factor;
            const double new_height = state_.viewport.height() * zoom_factor;
            
            /* Ваш код обновления state_ здесь  */
            const double x_min = target_x - new_width / 2;
            const double y_min = target_y - new_height / 2;
            state_.viewport =
            ViewPort{.x_min = x_min, .x_max = x_min + new_width, .y_min = y_min, .y_max = y_min + new_height};
            state_.need_rerender = true;
        }
    };

    sf::RenderWindow &window_;
    RenderSettings render_settings_;
    AppState &state_;
    using sender_concept = stdexec::sender_t;

    SfmlEventHandler(sf::RenderWindow &window, RenderSettings render_settings, AppState &state)
        : window_{window}, render_settings_{render_settings}, state_{state} {}

    /* Ваш код методов connect() и get_completion_signatures() здесь */
    template <typename Env>
    auto get_completion_signatures(Env &&) const {
        return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr),
                                         ex::set_stopped_t()>{};
    }

    template <typename Receiver>
    auto connect(Receiver &&receiver) {
        return OperationState<std::decay_t<Receiver>>(std::forward<Receiver>(receiver), window_, render_settings_,
                                                      state_);
    }
};
