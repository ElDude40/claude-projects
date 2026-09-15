import Foundation

/// Loads and searches the bundled medical term dictionary
/// (Resources/MedicalTerms.json).
@MainActor
@Observable
final class MedicalTermsRepository {
    private(set) var terms: [MedicalTerm] = []
    private(set) var loadError: String?

    init() {
        load()
    }

    private func load() {
        guard let url = Bundle.main.url(forResource: "MedicalTerms", withExtension: "json") else {
            loadError = "MedicalTerms.json was not found in the app bundle."
            return
        }
        do {
            let data = try Data(contentsOf: url)
            terms = try JSONDecoder().decode([MedicalTerm].self, from: data)
                .sorted { $0.term.localizedCaseInsensitiveCompare($1.term) == .orderedAscending }
        } catch {
            loadError = "Couldn't load the medical term dictionary: \(error.localizedDescription)"
        }
    }

    func terms(in category: MedicalTermCategory?) -> [MedicalTerm] {
        guard let category else { return terms }
        return terms.filter { $0.category == category }
    }

    func search(_ query: String) -> [MedicalTerm] {
        let trimmed = query.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else { return terms }
        let lowered = trimmed.lowercased()
        return terms.filter {
            $0.term.lowercased().contains(lowered) || $0.definition.lowercased().contains(lowered)
        }
    }

    /// Finds the dictionary term most likely referenced by free-form text,
    /// used to enrich a plain translation with an explanation and example.
    func closestMatch(for text: String) -> MedicalTerm? {
        let lowered = text.lowercased()
        guard !lowered.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else { return nil }
        return terms.first { lowered.contains($0.term.lowercased()) }
    }
}
