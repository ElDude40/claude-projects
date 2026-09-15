import SwiftUI

/// A searchable, filterable dictionary of medical terms.
struct MedicalTermsListView: View {
    @Environment(MedicalTermsRepository.self) private var repository
    @State private var searchText = ""
    @State private var selectedCategory: MedicalTermCategory?

    private var filteredTerms: [MedicalTerm] {
        let categoryFiltered = repository.terms(in: selectedCategory)
        let trimmed = searchText.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else { return categoryFiltered }
        let lowered = trimmed.lowercased()
        return categoryFiltered.filter {
            $0.term.lowercased().contains(lowered) || $0.definition.lowercased().contains(lowered)
        }
    }

    var body: some View {
        NavigationStack {
            List {
                Picker("Category", selection: $selectedCategory) {
                    Text("All Categories").tag(MedicalTermCategory?.none)
                    ForEach(MedicalTermCategory.allCases) { category in
                        Label(category.rawValue, systemImage: category.systemImage)
                            .tag(Optional(category))
                    }
                }
                .pickerStyle(.menu)

                ForEach(filteredTerms) { term in
                    NavigationLink(value: term) {
                        VStack(alignment: .leading, spacing: 4) {
                            Text(term.term).font(.headline)
                            Text(term.definition)
                                .font(.subheadline)
                                .foregroundStyle(.secondary)
                                .lineLimit(2)
                        }
                    }
                }
            }
            .navigationTitle("Medical Terms")
            .searchable(text: $searchText, prompt: "Search terms")
            .navigationDestination(for: MedicalTerm.self) { term in
                MedicalTermDetailView(term: term)
            }
            .overlay {
                if let error = repository.loadError {
                    ContentUnavailableView(
                        "Couldn't Load Terms",
                        systemImage: "exclamationmark.triangle",
                        description: Text(error)
                    )
                } else if filteredTerms.isEmpty {
                    ContentUnavailableView.search(text: searchText)
                }
            }
        }
    }
}
