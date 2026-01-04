#include "BSPBuildVisualizer.h"

namespace Genesis {

void BSPBuildVisualizer::StartRecording() {
    Clear();
    m_recording = true;
}

void BSPBuildVisualizer::StopRecording() {
    m_recording = false;
}

void BSPBuildVisualizer::AddStep(const BSPBuildStep& step) {
    if (m_recording) {
        m_steps.push_back(step);
    }
}

void BSPBuildVisualizer::Play() {
    if (m_steps.empty()) return;

    // If finished, restart. Otherwise resume.
    if (m_currentStep >= m_steps.size()) {
        m_currentStep = 0;
        m_stepAccumulator = 0.0f;
    }
    m_playing = true;
}

void BSPBuildVisualizer::Pause() {
    m_playing = false;
}

void BSPBuildVisualizer::TogglePlayback() {
    if (m_playing) {
        Pause();
    } else {
        Play();
    }
}

void BSPBuildVisualizer::Update(float deltaTime) {
    if (!m_playing || m_steps.empty()) return;

    m_stepAccumulator += deltaTime * m_playbackSpeed;

    while (m_stepAccumulator >= 1.0f && m_currentStep < m_steps.size()) {
        m_currentStep++;
        m_stepAccumulator -= 1.0f;
    }

    // Stop at end
    if (m_currentStep >= m_steps.size()) {
        m_currentStep = m_steps.size();
        m_playing = false;
    }
}

void BSPBuildVisualizer::SetWorldBounds(const Vec3& min, const Vec3& max) {
    m_worldMin = min;
    m_worldMax = max;
}

void BSPBuildVisualizer::Clear() {
    m_steps.clear();
    m_staticLines.clear();
    m_currentStep = 0;
    m_stepAccumulator = 0.0f;
    m_playing = false;
    m_recording = false;
}

} // namespace Genesis
