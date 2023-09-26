import Foundation


#if os(macOS)
import IOKit

func getModelIdentifier() -> String {
  let service = IOServiceGetMatchingService(kIOMainPortDefault,
                                            IOServiceMatching("IOPlatformExpertDevice"))
    var modelIdentifier: String?
    if let modelData = IORegistryEntryCreateCFProperty(service, "model" as CFString, kCFAllocatorDefault, 0).takeRetainedValue() as? Data {
        modelIdentifier = String(data: modelData, encoding: .utf8)?.trimmingCharacters(in: .controlCharacters)
    }

    IOObjectRelease(service)
    return modelIdentifier ?? "unknown"
}

#elseif os(iOS)
import UIKit

func getModelIdentifier() -> String {
  var systemInfo = utsname()
  uname(&systemInfo)
  let machineMirror = Mirror(reflecting: systemInfo.machine)
  let identifier = machineMirror.children.reduce("") { identifier, element in
              guard let value = element.value as? Int8, value != 0 else { return identifier }
              return identifier + String(UnicodeScalar(UInt8(value)))
          }
  
  return identifier
}
#endif


extension Array<Array<benchmark_sample_t>> {
  func toCSV(withModelIdentifier model: String = getModelIdentifier()) -> String {
    var out = "sample, thread_id, device, powermode, p_cycles, p_time, p_energy, e_cycles, e_time, e_energy, items\n"
    for (sample_idx, thread_samples) in self.enumerated() {
      for (thread_id, sample) in thread_samples.enumerated() {
        out += String(sample_idx) + ","
        out += String(thread_id) + ","
        out += "\"\(model)\","
        out += (sample.low_power ? "low" : "high") + ","
        out += String(sample.p_core_counters.cycles) + ","
        out += String(sample.p_core_counters.time) + ","
        out += String(sample.p_core_counters.energy) + ","
        out += String(sample.e_core_counters.cycles) + ","
        out += String(sample.e_core_counters.time) + ","
        out += String(sample.e_core_counters.energy) + ","
        out += String(sample.items) + "\n"
      }
    }
    return out
  }
}
