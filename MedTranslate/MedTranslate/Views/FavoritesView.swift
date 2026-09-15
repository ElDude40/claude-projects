import SwiftUI
import SwiftData

/// Lists every saved translation, each carrying its original text,
/// translation, languages, matched medical term, explanation, and example
/// sentence — everything needed to reuse it without retranslating.
struct FavoritesView: View {
    @Environment(\.modelContext) private var modelContext
    @Query(sort: \FavoriteTranslation.createdAt, order: .reverse) private var favorites: [FavoriteTranslation]
    @State private var searchText = ""

    private var filtered: [FavoriteTranslation] {
        let trimmed = searchText.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else { return favorites }
        let lowered = trimmed.lowercased()
        return favorites.filter {
            $0.sourceText.lowercased().contains(lowered) ||
            $0.translatedText.lowercased().contains(lowered) ||
            ($0.medicalTermName?.lowercased().contains(lowered) ?? false)
        }
    }

    var body: some View {
        NavigationStack {
            List {
                ForEach(filtered) { favorite in
                    NavigationLink {
                        FavoriteDetailView(favorite: favorite)
                    } label: {
                        VStack(alignment: .leading, spacing: 4) {
                            Text(favorite.sourceText).font(.headline)
                            Text(favorite.translatedText)
                                .font(.subheadline)
                                .foregroundStyle(.secondary)
                            Text("\(favorite.sourceLanguageName) → \(favorite.targetLanguageName)")
                                .font(.caption)
                                .foregroundStyle(.tertiary)
                        }
                    }
                }
                .onDelete(perform: delete)
            }
            .navigationTitle("Favorites")
            .searchable(text: $searchText, prompt: "Search favorites")
            .overlay {
                if favorites.isEmpty {
                    ContentUnavailableView(
                        "No Favorites Yet",
                        systemImage: "star",
                        description: Text("Save translations from the Translate or Medical Terms tabs to find them here.")
                    )
                }
            }
        }
    }

    private func delete(at offsets: IndexSet) {
        for index in offsets {
            modelContext.delete(filtered[index])
        }
    }
}
