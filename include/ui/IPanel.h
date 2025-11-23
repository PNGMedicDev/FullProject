#pragma once

#include <string>

namespace DiagIDE {
namespace UI {

// Base interface for all UI panels
class IPanel {
public:
    virtual ~IPanel() = default;

    // Panel lifecycle
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

    // Rendering
    virtual void Render() = 0;
    virtual void Update(float deltaTime) = 0;

    // Panel info
    virtual const char* GetName() const = 0;
    virtual bool IsVisible() const { return m_visible; }
    virtual void SetVisible(bool visible) { m_visible = visible; }

protected:
    bool m_visible = true;
};

} // namespace UI
} // namespace DiagIDE
