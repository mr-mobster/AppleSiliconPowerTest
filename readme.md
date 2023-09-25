# CPU frequency/power tester tool

## About

This is a simple tool for evaluating the CPU frequency and power consumption on Apple Silicon 
computers (Mac, iPhone, iPad). It runs a workload designed to saturate a CPU core while 
sampling performance counters. The results can be copied to a CSV file for subsequent evaluation.

## How to use

1. Download the project and open it in Xcode
2. Select your development team in the project signing panel (required to run on iOS)
3. Select the device to run the test on in the Xcode tool bar (you might need to connect your iPhone to your Mac using a cable)
4. Click the "Run tests" button 
5. The app will freeze, wait 10-30 seconds for it to do it's thing
6. The results will be copied to the clipboard (you can press the icon on top to do it again)


Consider submitting your result (you can use GitHub issues for that)!

## Limitations

The current test uses a brute-force algorithm to find prime numbers and only stresses a few 
CPU subsystems. I recommend to run the app in the debug mode, since optimizing makes the 
code smarter and reduces the CPU load. It would be good to improve this by providing a more 
demanging int and fp workloads. 

The test is done on the main thread, so this will freeze the app. One should start an utility 
thread to do this (for now I didn't bother). It would also enable doign multithreaded tests. 

## Terms of use

Permission is granted to download, build, and modify this software for evaluation purposes only. 

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.



