import SwiftUI


func copyToClipboard(text: String) {
  #if os(iOS)
    UIPasteboard.general.string = text
  #elseif os(macOS)
    let pasteboard = NSPasteboard.general
    pasteboard.declareTypes([.string], owner: nil)
    pasteboard.setString(text, forType: .string)
  #endif
}


struct ContentView: View {
  @State var samples : Array<benchmark_sample_t> = [];
  @State var msg = "the app will be unresponsive for several seconds while the tests are running"
  
  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
        List {
          Section(header: HStack {
            ForEach(["P core", "E core", "Power", "Time", "Primes/sec"], id: \.self) { Text($0) }
              .frame(minWidth: 0, maxWidth: .infinity, alignment: .leading)
            
            // results copy button
            Button(action: {
              copyToClipboard(text: samples.toCSV())
            }, label: {
              Image(systemName: "clipboard").buttonBorderShape(.roundedRectangle)
            })
            .disabled(samples.count == 0)
          }) {
            ForEach(0..<samples.count, id: \.self) {
              let sample = samples[$0]
              let time   = sample.p_core_counters.time + sample.e_core_counters.time
              
              HStack {
                Group {
                  Text(String(format: "%.3f Ghz", sample.p_core_counters.cycles/time/1e9))
                  Text(String(format: "%.3f Ghz", sample.e_core_counters.cycles/time/1e9))
                  Text(String(format: "%.3f W", (sample.p_core_counters.energy + sample.e_core_counters.energy)/time))
                  Text(String(format: "%.3f sec", time))
                  Text(String(format: "%.2f sec", Double(sample.primes)/time))
                }
                .lineLimit(1)
                .frame(minWidth: 0, maxWidth: .infinity, alignment: .leading)
              }
            }
          }
        }
        
        HStack {
          Button(action: {
            samples = []
            
            for _ in 0..<50 {
              var sample = run_benchmark()
              sample.low_power = ProcessInfo.processInfo.isLowPowerModeEnabled
              
              samples.append(sample)
            }
            
            copyToClipboard(text: samples.toCSV())
            msg = "Done, results have been copied to the clipboard"
          }, label: {
            Text("Run tests")
          })
          
          Text(msg).font(.subheadline)
        }
      }
      .padding()
  }
}

#Preview {
    ContentView()
}
