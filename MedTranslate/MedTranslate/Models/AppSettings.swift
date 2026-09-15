import Foundation

/// App-wide state: the clinician's language, the patient's language, and
/// voice output preferences. Shared across every tab via the environment.
@MainActor
@Observable
final class AppSettings {
    var sourceLanguage: SupportedLanguage
    var targetLanguage: SupportedLanguage
    var speechRate: Float

    init() {
        self.sourceLanguage = SupportedLanguage.all.first { $0.id == "en" } ?? SupportedLanguage.all[0]
        self.targetLanguage = SupportedLanguage.all.first { $0.id == "es" } ?? SupportedLanguage.all[1]
        self.speechRate = 0.5
    }

    func swapLanguages() {
        let temp = sourceLanguage
        sourceLanguage = targetLanguage
        targetLanguage = temp
    }
}
