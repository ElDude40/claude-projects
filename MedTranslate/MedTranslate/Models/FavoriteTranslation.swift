import Foundation
import SwiftData

/// A saved translation, with every piece of context tied to it so a
/// clinician can revisit exactly what was said, how it was explained,
/// and how to say it, without re-doing the work.
@Model
final class FavoriteTranslation {
    var id: UUID
    var createdAt: Date

    var sourceText: String
    var translatedText: String

    var sourceLanguageCode: String
    var sourceLanguageName: String
    var targetLanguageCode: String
    var targetLanguageName: String

    var medicalTermName: String?
    var category: String?
    var definition: String?
    var exampleSentenceSource: String?
    var exampleSentenceTranslated: String?

    var notes: String

    init(
        sourceText: String,
        translatedText: String,
        sourceLanguageCode: String,
        sourceLanguageName: String,
        targetLanguageCode: String,
        targetLanguageName: String,
        medicalTermName: String? = nil,
        category: String? = nil,
        definition: String? = nil,
        exampleSentenceSource: String? = nil,
        exampleSentenceTranslated: String? = nil,
        notes: String = ""
    ) {
        self.id = UUID()
        self.createdAt = Date()
        self.sourceText = sourceText
        self.translatedText = translatedText
        self.sourceLanguageCode = sourceLanguageCode
        self.sourceLanguageName = sourceLanguageName
        self.targetLanguageCode = targetLanguageCode
        self.targetLanguageName = targetLanguageName
        self.medicalTermName = medicalTermName
        self.category = category
        self.definition = definition
        self.exampleSentenceSource = exampleSentenceSource
        self.exampleSentenceTranslated = exampleSentenceTranslated
        self.notes = notes
    }
}
