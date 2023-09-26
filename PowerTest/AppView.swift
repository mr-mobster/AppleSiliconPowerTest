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


enum TestKind: CaseIterable, Identifiable {
  case single, multi
  var id: Self { self }
}

struct ContentView: View {
  @State var samples : Array<Array<benchmark_sample_t>> = [];
  @State var msg = ""
  @State var task : Task<Void, Never>? = .none
  @State var progress = 0.0
  @State var testKind: TestKind = .single
  
  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
        List {
          Section(header: HStack {
            ForEach(["P core", "E core", "Power", "Work/sec"], id: \.self) { Text($0) }
              .frame(minWidth: 0, maxWidth: .infinity, alignment: .leading)
              .font(.footnote)
            
            // results copy button
            Button(action: {
              copyToClipboard(text: samples.toCSV())
            }, label: {
              Image(systemName: "clipboard").buttonBorderShape(.roundedRectangle)
            })
            .disabled(samples.count == 0)
          }) {
            ForEach(0 ..< samples.count, id: \.self) {
              let thread_samples = samples[$0]
              
              // P core frequency (we pick the maximal one among the threads)
              let p_freq = thread_samples
                .map({ $0.p_core_counters.cycles/($0.p_core_counters.time)/1e9})
                .reduce(0, max)
              
              // E core frequency (we pick the maximal one among the threads)
              let e_freq = thread_samples
                .map({ $0.e_core_counters.cycles/($0.e_core_counters.time)/1e9})
                .reduce(0, max)

              // total items processed
              let items = thread_samples.map( { Double($0.items) } ).reduce(0, +)

              // total time elapsed (across all threads)
              let time = thread_samples.map( { $0.p_core_counters.time + $0.e_core_counters.time } ).reduce(0, +)

              // combined power use
              let power = thread_samples
                .map( {
                  ($0.p_core_counters.energy + $0.e_core_counters.energy) /
                  ($0.p_core_counters.time + $0.e_core_counters.time + .ulpOfOne)
                })
                .reduce(0, +)
              
              
              HStack {
                Group {
                  Text(String(format: "%.2f Ghz", p_freq))
                  Text(String(format: "%.2f Ghz", e_freq))
                  Text(String(format: "%.2f W", power))
                  Text(String(format: "%.2f", items/time))
                }
                .lineLimit(1)
                .frame(minWidth: 0, maxWidth: .infinity, alignment: .leading)
                .font(.footnote)
              }
            }
          }
        }
        
        HStack {
          Button(action: {
            guard task == .none else {
              task!.cancel()
              task = .none
              return
            }
            
            samples = []
            progress = 0
            msg = ""
            
            let n_steps = 50
            let step  = 1.0/Double(n_steps + 1)
            let n_threads = testKind == .single ? 1 : ProcessInfo.processInfo.activeProcessorCount
            
            task = Task.detached(priority: .userInitiated) {
              benchmark_start_threads(Int32(n_threads));
              
              var thread_samples = Array(repeating: benchmark_sample_t(), count: n_threads)
            
              for _ in 0 ..< n_steps {
                try? await Task.sleep(for: .milliseconds(1000))
                if Task.isCancelled { break }
                
                benchmark_sample_threads(Int32(n_threads), &thread_samples)
                for i in 0..<thread_samples.count {
                  thread_samples[i].low_power = ProcessInfo.processInfo.isLowPowerModeEnabled
                }
                
                samples.append(thread_samples)
                progress += step;
              }
              
              benchmark_teardown_threads()
              task = .none
              copyToClipboard(text: samples.toCSV())
              msg = "Results have been copied to the clipboard!"
            }
          }, label: {
            Text(task == .none ? "Run test" : "Cancel test")
          })
          
          Picker("", selection: $testKind) {
            Text("Single-core").tag(TestKind.single)
            Text("Multi-core (\(ProcessInfo.processInfo.activeProcessorCount) threads)").tag(TestKind.multi)
          }
          .frame(width: 200)
          .disabled(task != .none)
          
          

          Text(msg).font(.caption)
          if(task != .none) { ProgressView(value: progress).controlSize(.small) }
          
        }
      }
      .padding()
  }
}

#Preview {
    ContentView()
}
