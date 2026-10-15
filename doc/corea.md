# COREA TPC display

Build: `make -C src/analyzer corea` (also included in the normal build).

```sh
./bin/corea param/corea/analyzer.conf /path/to/HDDAQ-data.dat
# Or use a DAQ endpoint instead of the data file:
./bin/corea param/corea/analyzer.conf geteb1:8901
```

After a data file ends, the last event stays available until Ctrl+C.
Use the dedicated COREA configuration, `param/corea/analyzer.conf`.

Open http://localhost:9091. An optional final argument selects another HTTP port.
For an SSH host, forward the port with `ssh -L 9091:localhost:9091 hanul@HOST`.

The three canvases are TPC_ADC2D (peak minus baseline), TPC_TDC2D
(peak time for signals above 50 ADC), and TDC_FADC (the strongest pad's raw
waveform). Baseline is the mean of the first 100 available samples. Both
170- and 512-sample waveforms are accepted. Data files must be HDDAQ events,
not standalone GET `.graw` frames or merged raw GET files.

Mapping and geometry follow `/home/hanul/share/online-corea/macro/online.C`:
four AsAds, four AGETs each, channels 0..63 mapped to 1024 rectangular pads.
Channels 64..67 are excluded. Pad number is `asad*256 + aget*64 + channel + 1`.
The old HypTPC ring maps in `online-corea/param` are not the map used by that
macro. A dedicated identity channel map preserves GET channel numbering before
applying this rectangular geometry. The example configuration expects CoBo 0
at HDDAQ node 0xcb0 under root node 1; edit channel_map.xml if DAQ node IDs differ.

## Integration test

```sh
python3 tests/create_corea_fixture.py /tmp/corea-fixture.dat
g++ tests/corea_display_test.cc $(root-config --cflags) \
  -Iinclude -I/home/hanul/share/unpacker/include \
  -Llib -Wl,-rpath,"$PWD/lib" -Wl,--no-as-needed \
  -lmycorea -lNoGuiMain -lHistHelper -lK18AnalyzerCore \
  $(/home/hanul/share/unpacker/bin/unpacker-config --libs) \
  $(root-config --glibs) -lRHTTP -o /tmp/corea_display_test
/tmp/corea_display_test /tmp/corea-fixture.dat
```

The test decodes a synthetic HDDAQ event through the installed unpacker and
checks every pad's charge/time, the selected 512-bucket waveform, and exactly
three canvases. It briefly opens HTTP port 19091.
