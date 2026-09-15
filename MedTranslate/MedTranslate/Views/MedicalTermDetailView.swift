import SwiftUI
import SwiftData

/// Shows one medical term with its plain-language explanation and example
/// sentence, translated on demand for the patient, with speech output and
/// a way to save it to Favorites.
struct MedicalTermDetailView: View {
    let term: MedicalTerm

    @Environment(AppSettings.self) private var settings
    @Environment(\.modelContext) private var modelContext

    @State private var termController = TranslationController()
    @State private var definitionController = TranslationController()
    @State private var sentenceController = TranslationController()
    @State private var synthesisService = SpeechSynthesisService()

    @State private var translatedTerm: String = ""
    @State private var translatedDefinition: String = ""
    @State private var translatedSentence: String = ""
    @State private var isTranslating = false
    @State private var savedConfirmation = false

    var body: some View {
        Form {
            Section("Term") {
                Text(term.term).font(.title3).fontWeight(.semibold)
                Label(term.category.rawValue, systemImage: term.category.systemImage)
                    .font(.footnote)
                    .foregroundStyle(.secondary)
            }

            Section("Explanation") {
                Text(term.definition)
            }

            Section("Use in a sentence") {
                Text(term.exampleSentence)
                    .font(.subheadline)
            }

            Section {
                Button {
                    translateAll()
                } label: {
                    if isTranslating {
                        ProgressView()
                    } else {
                        Label("Translate for patient", systemImage: "character.bubble")
                    }
                }
            }

            if !translatedTerm.isEmpty {
                Section("Translation") {
                    LabeledContent("Term", value: translatedTerm)
                    LabeledContent("Explanation", value: translatedDefinition)
                    if !translatedSentence.isEmpty {
                        LabeledContent("Example", value: translatedSentence)
                    }

                    HStack {
                        Button {
                            synthesisService.speak(term.term, languageTag: settings.sourceLanguage.speechTag, rate: settings.speechRate)
                        } label: {
                            Label("Original", systemImage: "speaker.wave.2")
                        }
                        Spacer()
                        Button {
                            synthesisService.speak(translatedTerm, languageTag: settings.targetLanguage.speechTag, rate: settings.speechRate)
                        } label: {
                            Label("Translated", systemImage: "speaker.wave.2.fill")
                        }
                    }

                    Button {
                        saveFavorite()
                    } label: {
                        Label("Save to Favorites", systemImage: "star")
                    }
                }
            }
        }
        .navigationTitle(term.term)
        .navigationBarTitleDisplayMode(.inline)
        .translationEngine(termController)
        .translationEngine(definitionController)
        .translationEngine(sentenceController)
        .overlay(alignment: .top) {
            if savedConfirmation {
                Text("Saved to Favorites")
                    .font(.footnote)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 6)
                    .background(.green.opacity(0.9), in: Capsule())
                    .foregroundStyle(.white)
                    .padding(.top, 8)
            }
        }
    }

    private func translateAll() {
        isTranslating = true

        for controller in [termController, definitionController, sentenceController] {
            controller.sourceLanguage = settings.sourceLanguage.locale
            controller.targetLanguage = settings.targetLanguage.locale
        }

        termController.translate(term.term) { result in
            translatedTerm = result
            isTranslating = false
        }
        definitionController.translate(term.definition) { result in
            translatedDefinition = result
        }
        sentenceController.translate(term.exampleSentence) { result in
            translatedSentence = result
        }
    }

    private func saveFavorite() {
        let favorite = FavoriteTranslation(
            sourceText: term.term,
            translatedText: translatedTerm,
            sourceLanguageCode: settings.sourceLanguage.speechTag,
            sourceLanguageName: settings.sourceLanguage.displayName,
            targetLanguageCode: settings.targetLanguage.speechTag,
            targetLanguageName: settings.targetLanguage.displayName,
            medicalTermName: term.term,
            category: term.category.rawValue,
            definition: translatedDefinition.isEmpty ? term.definition : translatedDefinition,
            exampleSentenceSource: term.exampleSentence,
            exampleSentenceTranslated: translatedSentence.isEmpty ? nil : translatedSentence
        )
        modelContext.insert(favorite)

        withAnimation { savedConfirmation = true }
        Task {
            try? await Task.sleep(for: .seconds(2))
            withAnimation { savedConfirmation = false }
        }
    }
}
