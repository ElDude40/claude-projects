import SwiftUI

struct ContentView: View {
    var body: some View {
        TabView {
            TranslateView()
                .tabItem { Label("Translate", systemImage: "bubble.left.and.bubble.right") }

            MedicalTermsListView()
                .tabItem { Label("Medical Terms", systemImage: "cross.case") }

            FavoritesView()
                .tabItem { Label("Favorites", systemImage: "star") }

            SettingsView()
                .tabItem { Label("Settings", systemImage: "gearshape") }
        }
    }
}

#Preview {
    ContentView()
        .environment(AppSettings())
        .environment(MedicalTermsRepository())
        .modelContainer(for: FavoriteTranslation.self, inMemory: true)
}
