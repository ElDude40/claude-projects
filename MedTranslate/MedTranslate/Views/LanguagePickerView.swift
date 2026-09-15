import SwiftUI

/// A labeled picker for choosing one of the app's supported languages.
struct LanguagePickerView: View {
    let title: String
    @Binding var selection: SupportedLanguage

    var body: some View {
        Picker(title, selection: $selection) {
            ForEach(SupportedLanguage.all) { language in
                Text(language.displayName).tag(language)
            }
        }
    }
}
