import Foundation

/// Produces a "use it in a sentence" example for arbitrary translated
/// text: the matched dictionary term's example if one is found, otherwise
/// a generic conversational fallback.
enum SentenceGenerator {
    static func genericSentence(for text: String) -> String {
        let trimmed = text.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else { return "" }
        return "Can you tell me more about \"\(trimmed)\"?"
    }

    static func sentence(for text: String, using repository: MedicalTermsRepository) -> (sentence: String, matchedTerm: MedicalTerm?) {
        if let match = repository.closestMatch(for: text) {
            return (match.exampleSentence, match)
        }
        return (genericSentence(for: text), nil)
    }
}
