import SwiftUI
import SwiftData

/// The main screen: speak or type a phrase, translate it for the patient,
/// hear either side read aloud, generate a "use it in a sentence" example,
/// and save the whole exchange to Favorites.
struct TranslateView: View {
    @Environment(AppSettings.self) private var settings
    @Environment(\.modelContext) private var modelContext
    @Environment(MedicalTermsRepository.self) private var termsRepository

    @State private var speechService = SpeechRecognitionService()
    @State private var synthesisService = SpeechSynthesisService()
    @State private var translationController = TranslationController()
    @State private var sentenceController = TranslationController()

    @State private var inputText: String = ""
    @State private var translatedText: String = ""
    @State private var exampleSentenceSource: String = ""
    @State private var exampleSentenceTranslated: String = ""
    @State private var matchedTerm: MedicalTerm?
    @State private var isTranslating = false
    @State private var savedConfirmation = false
    @State private var showPermissionAlert = false

    var body: some View {
        @Bindable var settings = settings

        NavigationStack {
            Form {
                Section {
                    LanguagePickerView(title: "I speak", selection: $settings.sourceLanguage)
                    Button {
                        settings.swapLanguages()
                        resetResults()
                    } label: {
                        Label("Swap languages", systemImage: "arrow.left.arrow.right")
                    }
                    LanguagePickerView(title: "Patient speaks", selection: $settings.targetLanguage)
                }

                Section("What do you want to say?") {
                    HStack(alignment: .top, spacing: 12) {
                        TextField("Type or speak a phrase…", text: $inputText, axis: .vertical)
                            .lineLimit(2...5)
                        MicButton(isRecording: speechService.isRecording) {
                            toggleRecording()
                        }
                    }

                    if let error = speechService.errorMessage {
                        Text(error).font(.footnote).foregroundStyle(.red)
                    }

                    HStack {
                        Button {
                            synthesisService.speak(inputText, languageTag: settings.sourceLanguage.speechTag, rate: settings.speechRate)
                        } label: {
                            Label("Hear original", systemImage: "speaker.wave.2")
                        }
                        .disabled(inputText.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty)

                        Spacer()

                        Button {
                            translate()
                        } label: {
                            if isTranslating {
                                ProgressView()
                            } else {
                                Label("Translate", systemImage: "character.bubble")
                            }
                        }
                        .buttonStyle(.borderedProminent)
                        .disabled(inputText.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty)
                    }
                }

                if !translatedText.isEmpty {
                    Section("Translation") {
                        Text(translatedText)
                            .font(.title3)
                            .fontWeight(.semibold)

                        if let matchedTerm {
                            Label("Matches medical term: \(matchedTerm.term)", systemImage: "cross.case")
                                .font(.footnote)
                                .foregroundStyle(.secondary)
                        }

                        Button {
                            synthesisService.speak(translatedText, languageTag: settings.targetLanguage.speechTag, rate: settings.speechRate)
                        } label: {
                            Label("Hear translation", systemImage: "speaker.wave.2.fill")
                        }

                        Button {
                            generateSentence()
                        } label: {
                            Label("Use in a sentence", systemImage: "text.quote")
                        }

                        if !exampleSentenceTranslated.isEmpty {
                            VStack(alignment: .leading, spacing: 6) {
                                Text(exampleSentenceSource)
                                    .font(.subheadline)
                                    .foregroundStyle(.secondary)
                                Text(exampleSentenceTranslated)
                                    .font(.subheadline)
                                    .fontWeight(.medium)
                                Button {
                                    synthesisService.speak(exampleSentenceTranslated, languageTag: settings.targetLanguage.speechTag, rate: settings.speechRate)
                                } label: {
                                    Label("Hear example", systemImage: "speaker.wave.2")
                                }
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
            .navigationTitle("Translate")
            .overlay(alignment: .top) {
                if savedConfirmation {
                    Text("Saved to Favorites")
                        .font(.footnote)
                        .padding(.horizontal, 12)
                        .padding(.vertical, 6)
                        .background(.green.opacity(0.9), in: Capsule())
                        .foregroundStyle(.white)
                        .transition(.move(edge: .top).combined(with: .opacity))
                        .padding(.top, 8)
                }
            }
            .translationEngine(translationController)
            .translationEngine(sentenceController)
            .onChange(of: speechService.transcript) { _, newValue in
                inputText = newValue
            }
            .alert("Permission Needed", isPresented: $showPermissionAlert) {
                Button("OK", role: .cancel) {}
            } message: {
                Text("Please allow microphone and speech recognition access in Settings to use voice input.")
            }
        }
    }

    private func toggleRecording() {
        if speechService.isRecording {
            speechService.stopRecording()
            return
        }
        Task {
            let authorized = await speechService.requestAuthorization()
            guard authorized else {
                showPermissionAlert = true
                return
            }
            speechService.startRecording(languageTag: settings.sourceLanguage.speechTag)
        }
    }

    private func translate() {
        isTranslating = true
        matchedTerm = termsRepository.closestMatch(for: inputText)
        translationController.sourceLanguage = settings.sourceLanguage.locale
        translationController.targetLanguage = settings.targetLanguage.locale
        translationController.translate(inputText) { result in
            translatedText = result
            isTranslating = false
        }
    }

    private func generateSentence() {
        let result = SentenceGenerator.sentence(for: inputText, using: termsRepository)
        exampleSentenceSource = result.sentence
        matchedTerm = result.matchedTerm
        sentenceController.sourceLanguage = settings.sourceLanguage.locale
        sentenceController.targetLanguage = settings.targetLanguage.locale
        sentenceController.translate(result.sentence) { translated in
            exampleSentenceTranslated = translated
        }
    }

    private func saveFavorite() {
        let favorite = FavoriteTranslation(
            sourceText: inputText,
            translatedText: translatedText,
            sourceLanguageCode: settings.sourceLanguage.speechTag,
            sourceLanguageName: settings.sourceLanguage.displayName,
            targetLanguageCode: settings.targetLanguage.speechTag,
            targetLanguageName: settings.targetLanguage.displayName,
            medicalTermName: matchedTerm?.term,
            category: matchedTerm?.category.rawValue,
            definition: matchedTerm?.definition,
            exampleSentenceSource: exampleSentenceSource.isEmpty ? nil : exampleSentenceSource,
            exampleSentenceTranslated: exampleSentenceTranslated.isEmpty ? nil : exampleSentenceTranslated
        )
        modelContext.insert(favorite)

        withAnimation { savedConfirmation = true }
        Task {
            try? await Task.sleep(for: .seconds(2))
            withAnimation { savedConfirmation = false }
        }
    }

    private func resetResults() {
        translatedText = ""
        exampleSentenceSource = ""
        exampleSentenceTranslated = ""
        matchedTerm = nil
    }
}
