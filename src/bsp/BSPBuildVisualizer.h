#pragma once

#include "bsp/BSPTypes.h"
#include "math/Math.h"
#include <vector>
#include <chrono>

namespace Genesis {

// ============================================================================
// BSPBuildStep - One step in the BSP build process
// ============================================================================
struct BSPBuildStep {
    enum class Type {
        SplitPlane,     // A split plane was chosen
        CreateNode,     // A node was created
        CreateLeaf,     // A leaf was created
        SplitFace       // A face was split
    };

    Type type;
    Vec3 planePoint;    // Point on the split plane
    Vec3 planeNormal;   // Normal of the split plane
    Vec3 boundsMin;     // Bounds of the region being split
    Vec3 boundsMax;
    uint32_t nodeIndex; // Index of node/leaf created
    bool isFront;       // Front or back side
};

// ============================================================================
// BSPBuildVisualizer - Records and replays BSP build steps
// ============================================================================
class BSPBuildVisualizer {
public:
    static BSPBuildVisualizer& Instance() {
        static BSPBuildVisualizer instance;
        return instance;
    }

    // Recording
    void StartRecording();
    void StopRecording();
    void AddStep(const BSPBuildStep& step);
    bool IsRecording() const { return m_recording; }

    // Playback
    void Play();
    void Pause();
    void TogglePlayback();
    void Update(float deltaTime);
    bool IsPlaying() const { return m_playing; }
    void SetPlaybackSpeed(float speed) { m_playbackSpeed = speed; }

    // Get current state for rendering
    const std::vector<BSPBuildStep>& GetSteps() const { return m_steps; }
    size_t GetCurrentStep() const { return m_currentStep; }
    size_t GetTotalSteps() const { return m_steps.size(); }

    // World bounds (for mapping to screen)
    void SetWorldBounds(const Vec3& min, const Vec3& max);
    Vec3 GetWorldMin() const { return m_worldMin; }
    Vec3 GetWorldMax() const { return m_worldMax; }

    // Static geometry (for visualization background)
    struct Line {
        Vec3 start;
        Vec3 end;
    };
    void AddStaticLine(const Vec3& start, const Vec3& end) {
        if (m_recording) m_staticLines.push_back({start, end});
    }
    const std::vector<Line>& GetStaticLines() const { return m_staticLines; }

    // Clear all recorded data
    void Clear();

private:
    BSPBuildVisualizer() = default;

    bool m_recording = false;
    bool m_playing = false;
    float m_playbackSpeed = 5.0f;   // Slower default speed (5 steps/sec)
    float m_stepAccumulator = 0.0f;
    size_t m_currentStep = 0;

    std::vector<BSPBuildStep> m_steps;
    std::vector<Line> m_staticLines;
    Vec3 m_worldMin{0.0f};
    Vec3 m_worldMax{0.0f};
};

} // namespace Genesis
