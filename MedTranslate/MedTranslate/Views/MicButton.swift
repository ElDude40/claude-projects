import SwiftUI

/// A round microphone button that pulses while actively recording.
struct MicButton: View {
    let isRecording: Bool
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            Image(systemName: isRecording ? "mic.fill" : "mic")
                .font(.title2)
                .foregroundStyle(.white)
                .padding(14)
                .background(isRecording ? Color.red : Color.accentColor)
                .clipShape(Circle())
                .scaleEffect(isRecording ? 1.08 : 1.0)
                .animation(
                    isRecording ? .easeInOut(duration: 0.5).repeatForever(autoreverses: true) : .default,
                    value: isRecording
                )
        }
        .buttonStyle(.plain)
        .accessibilityLabel(isRecording ? "Stop recording" : "Start voice input")
    }
}

#Preview {
    HStack(spacing: 24) {
        MicButton(isRecording: false) {}
        MicButton(isRecording: true) {}
    }
    .padding()
}
