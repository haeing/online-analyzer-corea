# GET full readout

`hddaq-corea/Frontend/get_node/src/EventReader.cc` stores both partial
(FrameType 1, 32-bit items) and full (FrameType 2, 16-bit items) GET payloads
inside HDDAQ AsAd blocks without swapping the payload bytes.

The installed unpacker at `/home/hanul/share/unpacker` decodes full frames in
`src/unpacker/src/GetAsAd.cc`. Full samples are big-endian 16-bit values:
bits 15:14 identify the AGET, bits 11:0 contain the ADC. Each AGET has its own
implicit channel counter cycling through 0..67; successive cycles are time
buckets. Item size and available payload length are checked before decoding.
Partial frames keep their existing decoding path.

`bin/e72_tpc` and `bin/e72_tpc_ps` now select the maximum waveform regardless
of whether it has 170 or 512 samples, and display the available samples.
TPC waveform and time histograms have capacity for 512 buckets. Existing
physical time windows and drift-position conversions still need calibration
for the acquisition settings used; waveform support does not recalibrate them.

Example with an HDDAQ stream (or replace the last argument with an HDDAQ data file):

```sh
cd /home/hanul/online-analyzer
./bin/e72_tpc /home/hanul/share/param/conf/analyzer_e72_online.conf geteb1:8901
```

The default web port for `e72_tpc` is 9090. Standalone `.graw` files are GET
frames, not HDDAQ event files; the decoder test below can read their first frame.

## Decoder regression test

```sh
g++ -std=c++17 -I/home/hanul/share/unpacker/include \
  tests/unpacker_full_read_test.cc \
  -L/home/hanul/share/unpacker/lib \
  -Wl,-rpath,/home/hanul/share/unpacker/lib -Wl,--no-as-needed \
  -lGetAsAd -lHDDAQUnpacker -lUnpackerImpl -o /tmp/test_get_asad_full
/tmp/test_get_asad_full
# Optionally add a full-readout .graw file path to test its first frame.
```

Checks cover all channels and ADC samples for 170/512 buckets with four
interleaved AGETs, invalid item size, truncated payload, odd sample count,
empty payload, and a partial-readout regression sample. The two invalid-input
cases intentionally print warnings.
