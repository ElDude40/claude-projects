import Foundation

/// A language the app can translate to/from, speak, and listen for.
struct SupportedLanguage: Identifiable, Hashable, Codable {
    /// BCP-47 language identifier (e.g. "en", "es", "zh-Hans").
    let id: String
    let displayName: String

    var locale: Locale.Language {
        Locale.Language(identifier: id)
    }

    /// A region-qualified tag suitable for Speech / AVSpeechSynthesis APIs.
    var speechTag: String {
        speechLocaleOverrides[id] ?? id
    }

    private var speechLocaleOverrides: [String: String] {
        [
            "en": "en-US",
            "es": "es-ES",
            "zh-Hans": "zh-CN",
            "fr": "fr-FR",
            "ar": "ar-SA",
            "ru": "ru-RU",
            "pt-BR": "pt-BR",
            "vi": "vi-VN",
            "ko": "ko-KR",
            "tl": "fil-PH",
            "ht": "fr-HT",
            "de": "de-DE",
            "hi": "hi-IN",
            "ja": "ja-JP",
            "it": "it-IT"
        ]
    }

    static let all: [SupportedLanguage] = [
        SupportedLanguage(id: "en", displayName: "English"),
        SupportedLanguage(id: "es", displayName: "Spanish"),
        SupportedLanguage(id: "zh-Hans", displayName: "Chinese (Simplified)"),
        SupportedLanguage(id: "fr", displayName: "French"),
        SupportedLanguage(id: "ar", displayName: "Arabic"),
        SupportedLanguage(id: "ru", displayName: "Russian"),
        SupportedLanguage(id: "pt-BR", displayName: "Portuguese"),
        SupportedLanguage(id: "vi", displayName: "Vietnamese"),
        SupportedLanguage(id: "ko", displayName: "Korean"),
        SupportedLanguage(id: "tl", displayName: "Tagalog"),
        SupportedLanguage(id: "ht", displayName: "Haitian Creole"),
        SupportedLanguage(id: "de", displayName: "German"),
        SupportedLanguage(id: "hi", displayName: "Hindi"),
        SupportedLanguage(id: "ja", displayName: "Japanese"),
        SupportedLanguage(id: "it", displayName: "Italian")
    ]
}
