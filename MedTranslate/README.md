# MedTranslate

An iOS app that helps healthcare professionals communicate with patients
across languages. Built with SwiftUI, targeting **iOS 17.4+**.

## Features

- **Translate** — type or dictate a phrase, translate it on-device for the
  patient, hear either side read aloud, and save the exchange.
- **Medical Terms dictionary** — 50 common terms (symptoms, conditions, body
  parts, procedures, medications, instructions), each with a plain-language
  explanation and an example sentence, translatable and speakable on demand.
- **"Use in a sentence"** — every term, and any free-typed/spoken phrase that
  matches one, comes with an example sentence a clinician can say to a
  patient, translated into the patient's language.
- **Voice input and output** — dictate with the microphone (Speech
  framework) and listen to translations, terms, and example sentences read
  aloud (AVSpeechSynthesizer), in either language.
- **Favorites** — save any translation (original text, translation,
  languages, matched medical term, explanation, and example sentence) for
  instant reuse later, with search.

## Why on-device translation

Translation runs through Apple's **Translation** framework, which performs
translation entirely on-device. Patient conversations, which can include
sensitive health information, never leave the phone to reach a translation
server.

## Project structure

```
MedTranslate/
  MedTranslateApp.swift        App entry point, injects shared state
  ContentView.swift            Tab bar: Translate / Medical Terms / Favorites / Settings
  Models/
    Language.swift             Supported languages (BCP-47 + speech tags)
    MedicalTerm.swift          Dictionary entry model + categories
    FavoriteTranslation.swift  SwiftData model for saved translations
    AppSettings.swift          Shared source/target language + speech rate
  Services/
    TranslationEngine.swift        Wraps the Translation framework
    SpeechRecognitionService.swift Wraps Speech + AVAudioEngine (voice input)
    SpeechSynthesisService.swift   Wraps AVSpeechSynthesizer (voice output)
    MedicalTermsRepository.swift   Loads/searches the term dictionary
    SentenceGenerator.swift        "Use in a sentence" logic
  Views/
    TranslateView.swift
    MedicalTermsListView.swift
    MedicalTermDetailView.swift
    FavoritesView.swift
    FavoriteDetailView.swift
    SettingsView.swift
    LanguagePickerView.swift, MicButton.swift  (shared components)
  Resources/
    MedicalTerms.json          The bundled term dictionary
```

## Opening the project

1. Open `MedTranslate.xcodeproj` in Xcode 15.4 or later.
2. Select the `MedTranslate` scheme and a simulator/device running iOS 17.4+.
3. Build and run (⌘R).

On first launch, each language you translate into may need its language
pack downloaded by iOS (Settings → General → Translation Languages) the
first time it's used, and Speech Recognition needs its corresponding
language installed on-device for voice input in that language.

### Before shipping

- Change `PRODUCT_BUNDLE_IDENTIFIER` (currently
  `com.medtranslate.healthcare`) to your own bundle ID and set your team for
  code signing.
- Add real app icon images to `Assets.xcassets/AppIcon.appiconset`.
- Review `Info.plist`-equivalent build settings
  (`INFOPLIST_KEY_NSMicrophoneUsageDescription` and
  `INFOPLIST_KEY_NSSpeechRecognitionUsageDescription` in the target's build
  settings) and adjust the wording if needed — these power the permission
  prompts for voice input.

## Extending the dictionary

Add entries to `MedTranslate/Resources/MedicalTerms.json`. Each entry needs:

```json
{
  "id": "unique-id",
  "term": "Term",
  "category": "Symptom | Condition | Body Part | Procedure | Medication | Instruction",
  "definition": "Plain-language explanation for a patient.",
  "exampleSentence": "A sentence a clinician might say using the term."
}
```
