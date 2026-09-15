import Foundation

enum MedicalTermCategory: String, Codable, CaseIterable, Identifiable {
    case symptom = "Symptom"
    case condition = "Condition"
    case bodyPart = "Body Part"
    case procedure = "Procedure"
    case medication = "Medication"
    case instruction = "Instruction"

    var id: String { rawValue }

    var systemImage: String {
        switch self {
        case .symptom: return "waveform.path.ecg"
        case .condition: return "heart.text.square"
        case .bodyPart: return "figure.stand"
        case .procedure: return "stethoscope"
        case .medication: return "pills"
        case .instruction: return "list.bullet.clipboard"
        }
    }
}

/// A single medical term with a plain-language explanation and an example
/// sentence a clinician can use when speaking with a patient.
struct MedicalTerm: Identifiable, Codable, Hashable {
    let id: String
    let term: String
    let category: MedicalTermCategory
    let definition: String
    let exampleSentence: String
}
