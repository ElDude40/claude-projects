import SwiftUI
import Translation

/// Drives Apple's on-device Translation framework for a single piece of
/// text at a time. Translation happens entirely on-device, which matters
/// for patient conversations. Use one controller per independently
/// translated piece of text (don't share a controller between two texts
/// that might be translated back-to-back).
@MainActor
@Observable
final class TranslationController {
    var sourceLanguage: Locale.Language
    var targetLanguage: Locale.Language
    private(set) var isTranslating = false
    private(set) var lastError: String?

    fileprivate var configuration: TranslationSession.Configuration?
    private var pendingText: String = ""
    private var completion: ((String) -> Void)?

    init(
        sourceLanguage: Locale.Language = Locale.Language(identifier: "en"),
        targetLanguage: Locale.Language = Locale.Language(identifier: "es")
    ) {
        self.sourceLanguage = sourceLanguage
        self.targetLanguage = targetLanguage
    }

    /// Requests a translation of `text` using the controller's current
    /// source/target languages. `completion` is called with the translated
    /// text, or an empty string if translation failed.
    func translate(_ text: String, completion: @escaping (String) -> Void) {
        let trimmed = text.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else {
            completion("")
            return
        }

        pendingText = text
        self.completion = completion
        isTranslating = true
        lastError = nil

        if let existing = configuration,
           existing.source?.minimalIdentifier == sourceLanguage.minimalIdentifier,
           existing.target?.minimalIdentifier == targetLanguage.minimalIdentifier {
            existing.invalidate()
        } else {
            configuration = TranslationSession.Configuration(source: sourceLanguage, target: targetLanguage)
        }
    }

    fileprivate func handle(session: TranslationSession) async {
        guard !pendingText.isEmpty, let completion else { return }
        do {
            let response = try await session.translate(pendingText)
            isTranslating = false
            completion(response.targetText)
        } catch {
            isTranslating = false
            lastError = error.localizedDescription
            completion("")
        }
    }
}

private struct TranslationHost: ViewModifier {
    @Bindable var controller: TranslationController

    func body(content: Content) -> some View {
        content.translationTask(controller.configuration) { session in
            await controller.handle(session: session)
        }
    }
}

extension View {
    /// Attaches a `TranslationController` to this view so it can perform
    /// on-device translations. A view may attach more than one controller
    /// if it needs to translate more than one independent piece of text.
    func translationEngine(_ controller: TranslationController) -> some View {
        modifier(TranslationHost(controller: controller))
    }
}
