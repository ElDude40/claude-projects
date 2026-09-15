import SwiftUI
import SwiftData

@main
struct MedTranslateApp: App {
    @State private var appSettings = AppSettings()
    @State private var termsRepository = MedicalTermsRepository()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environment(appSettings)
                .environment(termsRepository)
        }
        .modelContainer(for: FavoriteTranslation.self)
    }
}
