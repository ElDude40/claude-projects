import SwiftUI

/// Full detail for one saved favorite: original text, translation,
/// matched medical term with its explanation, and the example sentence —
/// each with its own "hear it" button.
struct FavoriteDetailView: View {
    let favorite: FavoriteTranslation

    @Environment(AppSettings.self) private var settings
    @State private var synthesisService = SpeechSynthesisService()

    var body: some View {
        Form {
            Section(favorite.sourceLanguageName) {
                Text(favorite.sourceText)
                Button {
                    synthesisService.speak(favorite.sourceText, languageTag: favorite.sourceLanguageCode, rate: settings.speechRate)
                } label: {
                    Label("Hear original", systemImage: "speaker.wave.2")
                }
            }

            Section(favorite.targetLanguageName) {
                Text(favorite.translatedText)
                Button {
                    synthesisService.speak(favorite.translatedText, languageTag: favorite.targetLanguageCode, rate: settings.speechRate)
                } label: {
                    Label("Hear translation", systemImage: "speaker.wave.2.fill")
                }
            }

            if let termName = favorite.medicalTermName {
                Section("Medical Term") {
                    LabeledContent("Term", value: termName)
                    if let category = favorite.category {
                        LabeledContent("Category", value: category)
                    }
                    if let definition = favorite.definition {
                        Text(definition).font(.subheadline)
                    }
                }
            }

            if let source = favorite.exampleSentenceSource, let translated = favorite.exampleSentenceTranslated {
                Section("Example Sentence") {
                    Text(source).foregroundStyle(.secondary)
                    Text(translated).fontWeight(.medium)
                    Button {
                        synthesisService.speak(translated, languageTag: favorite.targetLanguageCode, rate: settings.speechRate)
                    } label: {
                        Label("Hear example", systemImage: "speaker.wave.2")
                    }
                }
            }

            Section {
                LabeledContent("Saved", value: favorite.createdAt.formatted(date: .abbreviated, time: .shortened))
            }
        }
        .navigationTitle(favorite.medicalTermName ?? "Favorite")
        .navigationBarTitleDisplayMode(.inline)
    }
}
