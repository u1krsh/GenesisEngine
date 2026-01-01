#pragma once

#include <chrono>

namespace Genesis {

// ============================================================================
// Time - High-resolution time management
// ============================================================================
class Time {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::duration<double>;

    static Time& Instance() {
        static Time instance;
        return instance;
    }

    // Initialize the timer (call once at startup)
    void Initialize() {
        m_startTime = Clock::now();
        m_lastFrameTime = m_startTime;
        m_currentTime = m_startTime;
        m_deltaTime = 0.0;
        m_totalTime = 0.0;
        m_frameCount = 0;
        m_fps = 0.0f;
        m_fpsAccumulator = 0.0;
        m_fpsFrameCount = 0;
    }

    // Update time (call once per frame at the start of the frame)
    void Update() {
        m_lastFrameTime = m_currentTime;
        m_currentTime = Clock::now();

        Duration delta = m_currentTime - m_lastFrameTime;
        m_deltaTime = delta.count();

        Duration total = m_currentTime - m_startTime;
        m_totalTime = total.count();

        m_frameCount++;

        // Calculate FPS
        m_fpsAccumulator += m_deltaTime;
        m_fpsFrameCount++;
        if (m_fpsAccumulator >= 1.0) {
            m_fps = static_cast<float>(m_fpsFrameCount) / static_cast<float>(m_fpsAccumulator);
            m_fpsAccumulator = 0.0;
            m_fpsFrameCount = 0;
        }
    }

    // Getters
    double GetDeltaTime() const { return m_deltaTime; }
    float GetDeltaTimeF() const { return static_cast<float>(m_deltaTime); }
    double GetTotalTime() const { return m_totalTime; }
    float GetTotalTimeF() const { return static_cast<float>(m_totalTime); }
    uint64_t GetFrameCount() const { return m_frameCount; }
    float GetFPS() const { return m_fps; }

    // Fixed timestep helpers
    double GetFixedDeltaTime() const { return m_fixedDeltaTime; }
    void SetFixedDeltaTime(double dt) { m_fixedDeltaTime = dt; }

    // Time scale (for slow-mo effects)
    float GetTimeScale() const { return m_timeScale; }
    void SetTimeScale(float scale) { m_timeScale = scale; }
    double GetScaledDeltaTime() const { return m_deltaTime * m_timeScale; }

private:
    Time() = default;

    TimePoint m_startTime;
    TimePoint m_lastFrameTime;
    TimePoint m_currentTime;

    double m_deltaTime = 0.0;
    double m_totalTime = 0.0;
    double m_fixedDeltaTime = 1.0 / 60.0;  // 60 Hz default
    float m_timeScale = 1.0f;

    uint64_t m_frameCount = 0;
    float m_fps = 0.0f;
    double m_fpsAccumulator = 0.0;
    int m_fpsFrameCount = 0;
};

} // namespace Genesis

