# Running COREA

## View live data

Run these commands on the computer hosting online-analyzer. The data distributor
on axis must be running at `192.168.10.2:8901`.

```bash
cd /home/hanul/online-analyzer
./bin/corea param/corea/analyzer.conf 192.168.10.2:8901
```

Open **http://localhost:9091** in a browser on the same computer.

The display contains three canvases: `TPC_ADC2D`, `TPC_TDC2D`, and `TDC_FADC`.
Press `Ctrl+C` in the terminal to stop COREA.

## View a recorded data file

```bash
cd /home/hanul/online-analyzer
./bin/corea param/corea/analyzer.conf /misc/hddaq/corea/run00027.dat
```

Open **http://localhost:9091**.
When the file ends, you may see `EventReader::read() no data`.
The last event remains available in the browser until you press `Ctrl+C`.

## Open the display from another computer

On your own computer, open an SSH tunnel to the computer running online-analyzer.
Replace `ANALYSIS_HOST` with that computer's IP address or hostname.

```bash
ssh -N -L 9091:localhost:9091 hanul@ANALYSIS_HOST
```

Keep this terminal open and visit **http://localhost:9091** in your local browser.

## Connect to axis through an SSH tunnel

If the analysis computer cannot reach the data port directly but can connect
to axis over SSH, run this command in a separate terminal on the analysis computer.
Replace `AXIS_USER` with your SSH username on axis.

```bash
ssh -N -L 18901:localhost:8901 AXIS_USER@192.168.10.2
```

Keep the tunnel open, then start COREA in another terminal on the analysis computer:

```bash
cd /home/hanul/online-analyzer
./bin/corea param/corea/analyzer.conf localhost:18901
```

## Build or rebuild COREA

```bash
cd /home/hanul/online-analyzer
make -C src/analyzer corea -j2
```

Use the dedicated configuration, `param/corea/analyzer.conf`.

## Use a different web port

Specify the web port as the final argument:

```bash
./bin/corea param/corea/analyzer.conf 192.168.10.2:8901 9092
```

Then open **http://localhost:9092**. If using SSH port forwarding, update the
forwarded web port to match.
