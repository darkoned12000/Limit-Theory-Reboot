#include "Window.h"

#include "Array.h"
#include "Keyboard.h"
#include "Matrix.h"
#include "Mouse.h"
#include "Pointer.h"
#include "Renderer.h"
#include "String.h"
#include "Texture2D.h"
#include "Viewport.h"

#include "SFML/Graphics.hpp"

namespace {
  Vector<Window>& GetStack() {
    static Vector<Window> stack;
    return stack;
  }

  void ProcessMouseEvent(sf::Mouse::Button button, bool pressed) {
    switch (button) {
      case sf::Mouse::Button::Left:
        Mouse_SetPressed(MouseButton_Left, pressed); break;
      case sf::Mouse::Button::Right:
        Mouse_SetPressed(MouseButton_Right, pressed); break;
      case sf::Mouse::Button::Middle:
        Mouse_SetPressed(MouseButton_Middle, pressed); break;
      case sf::Mouse::Button::Extra1:
        Mouse_SetPressed(MouseButton_X1, pressed); break;
      case sf::Mouse::Button::Extra2:
        Mouse_SetPressed(MouseButton_X2, pressed); break;
      default: break;
    }
  }

  struct WindowImpl : public WindowT {
    sf::RenderWindow impl;
    String title;
    Viewport viewport;
    V2U size;
    uint bpp;
    bool captureMouse;
    bool hasFocus;

    WindowImpl(
        String const& title,
        V2U const& size,
        bool border,
        bool fullscreen) :
      title(title),
      size(size),
      bpp(32),
      captureMouse(false),
      hasFocus(true)
    {
      viewport = Viewport_Create(0, size, 1, true);
      sf::ContextSettings glSettings;
      // Request a 4.6 context (shaders use #version 460). Leave attributeFlags
      // at Default: setting the explicit Core bit makes SFML 2.6 + this Mesa
      // driver crash with a GLX MakeCurrent / oldCtxInfo assertion at first
      // context switch. The engine's draw path is made core-compatible via the
      // VBO fallbacks in Renderer.cpp (DrawQuad / DrawQuadOutline / DrawVertices),
      // so a core 3.3+ context works without client-side vertex arrays.
      glSettings.depthBits = 24;
      glSettings.stencilBits = 8;
      glSettings.majorVersion = 4;
      glSettings.minorVersion = 6;
      glSettings.attributeFlags = sf::ContextSettings::Attribute::Default;
      impl.create(
        sf::VideoMode({size.x, size.y}, bpp),
        title,
        sf::Style::Default,
        fullscreen ? sf::State::Fullscreen : sf::State::Windowed,
        glSettings);
      impl.setMouseCursorVisible(false);
      impl.setView(sf::View(sf::FloatRect({0, 0},
        {static_cast<float>(size.x), static_cast<float>(size.y)})));
      viewport->size = size;
    }

    void Close() override {
      impl.close();
    }

    void Display() override {
      impl.display();
    }

    void* GetImplData() override {
      return &impl;
    }

    V2U GetSize() const override {
      return size;
    }

    bool HasFocus() const override {
      return hasFocus;
    }

    bool IsOpen() const override {
      return impl.isOpen();
    }

    void SetCaptureMouse(bool captureMouse) override {
      this->captureMouse = captureMouse;
    }

    void SetCursorVisible(bool visible) override {
      impl.setMouseCursorVisible(visible);
    }

    void SetFullscreen() override {
      impl.create(
        sf::VideoMode({size.x, size.y}, bpp),
        title,
        sf::State::Fullscreen);
      viewport->size.x = (float)impl.getSize().x;
      viewport->size.y = (float)impl.getSize().y;
      impl.setMouseCursorVisible(false);
    }

    void SetIcon(Texture2D const& icon) override {
      Array<uchar> buf(icon->GetMemory());
      icon->GetData(buf.data());
      impl.setIcon(
        {icon->GetWidth(), icon->GetHeight()},
        reinterpret_cast<std::uint8_t const*>(buf.data()));
    }

    void SetPosition(V2I const& p) override {
      impl.setPosition(sf::Vector2i(p.x, p.y));
    }

    void SetSync(bool sync) override {
      impl.setVerticalSyncEnabled(sync);
    }

    void Update() override {
      while (const auto event = impl.pollEvent()) {
        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
          float w = (float)resized->size.x;
          float h = (float)resized->size.y;
          impl.setView(sf::View(sf::FloatRect({0, 0}, {w, h})));
          size = V2U(resized->size.x, resized->size.y);
          viewport->size = V2(w, h);
        }

        else if (const auto* keyPressed =
                   event->getIf<sf::Event::KeyPressed>())
        {
          if (keyPressed->code != sf::Keyboard::Key::Unknown)
            Keyboard_AddDown(static_cast<int>(keyPressed->code));
        }

        else if (const auto* mouseButton =
                   event->getIf<sf::Event::MouseButtonPressed>())
        {
          ProcessMouseEvent(mouseButton->button, true);
        }

        else if (const auto* mouseButton =
                   event->getIf<sf::Event::MouseButtonReleased>())
        {
          ProcessMouseEvent(mouseButton->button, false);
        }

        else if (const auto* mouseMoved =
                   event->getIf<sf::Event::MouseMoved>())
        {
          V2I p(mouseMoved->position.x, mouseMoved->position.y);

          if (captureMouse) {
            const V2I borderSize = 1;
            V2I s = (V2I)size - borderSize;
            if (p.x < borderSize.x ||
                p.y < borderSize.y ||
                p.x > s.x ||
                p.y > s.y)
            {
              p = Clamp(p, borderSize, s);
              Mouse_SetPos(p);
            }
          }
          Mouse_UpdatePos(p);
        }

        else if (const auto* wheel =
                   event->getIf<sf::Event::MouseWheelScrolled>())
        {
          if (hasFocus)
            Mouse_SetScrollDelta((float)wheel->delta);
        }

        else if (event->is<sf::Event::FocusGained>())
          hasFocus = true;

        else if (event->is<sf::Event::FocusLost>())
          hasFocus = false;

        else if (const auto* textEntered =
                   event->getIf<sf::Event::TextEntered>())
        {
          if (textEntered->unicode >= 32 && textEntered->unicode <= 126)
            Keyboard_AddText(static_cast<char>(textEntered->unicode));
        }
      }
    }
  };
}

Window Window_Create(
  String const& title,
  V2U const& size,
  bool border,
  bool fullscreen)
{
  return new WindowImpl(title, size, border, fullscreen);
}

Window Window_Get() {
  return GetStack().size() ? GetStack().back() : nullptr;
}

void Window_Pop() {
  Viewport_Pop();
  GetStack().pop();
}

void Window_Push(Window const& window) {
  GetStack().push(window);
  Viewport_Push(((WindowImpl*)window.t)->viewport);
}
