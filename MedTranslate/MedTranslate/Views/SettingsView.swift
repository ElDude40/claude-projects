import SwiftUI

/// Default languages, voice output speed, and a short privacy note.
struct SettingsView: View {
    @Environment(AppSettings.self) private var settings

    var body: some View {
        @Bindable var settings = settings

        NavigationStack {
            Form {
                Section("Default Languages") {
                    LanguagePickerView(title: "I speak", selection: $settings.sourceLanguage)
                    LanguagePickerView(title: "Patient speaks", selection: $settings.targetLanguage)
                }

                Section("Voice Output") {
                    VStack(alignment: .leading) {
                        Text("Speech rate")
                        Slider(value: $settings.speechRate, in: 0.3...0.6)
                    }
                }

                Section("About") {
                    Text("MedTranslate helps healthcare professionals communicate with patients across languages: translate on the spot, explain medical terms in plain language, and hear everything read aloud.")
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                }

                Section("Privacy") {
                    Label("Translation happens on-device using Apple's Translation framework — patient conversations never leave the phone.", systemImage: "lock.shield")
                        .font(.footnote)
                    Label("Voice input requires microphone and speech-recognition permission.", systemImage: "mic")
                        .font(.footnote)
                }
            }
            .navigationTitle("Settings")
        }
    }
}
