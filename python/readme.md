# Python setup

On Windows 11

In `environment.yml` there are instructions for creating a named conda environment with Joulescope's dependencies and Joulescope. Note that the latter is installed via pip., e.g.:

```yaml
name: jls2
dependencies:
  - python=3.9
  - numpy
  - pandas
  - pip
  - pip:
    - joulescope
```


```bash
conda env create -f environment.yml
conda activate jls2
```

### Debug the Joulescope

To control trace, run e.g.:

```
set VANET_LOG_LEVEL=INFO
```

or in PowerShell:

```PS
$env:VANET_LOG_LEVEL='INFO'
```

### Test the hardware setup

Ensure that the JS UI is not monitoring the Joulescope, and run the following in the conda shell:

```bash
cd \repos\vanet\python
python trigger2.py --record --start rising --end falling --count 1 --jls_version 1 --display_stats --event_count 6
```

Start another standard command line shell and run:

```bash
ssh 10.0.0.123 ~/repos/vanet/Version6/scripts/run_infer_l_sync.sh -i -v 1 -s 20 -d 1024 -t 2 -a 3 -c 50
```

The `trigger2.py` script is waiting for a total of 6 falling edges from `in0`. The `infer7` command line provides 3 repeats, each of which contains two `in0` rise/fall pairs.

The IP address should be that of the Raspberry Pi 4.

## Tests

```bash
python .\run_infer7.py -t 2 -s 50 -n 50 -d 1024 -r 32 -i 10.0.0.123 -m 3 -o data3
```
