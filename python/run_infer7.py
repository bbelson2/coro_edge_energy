#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from joulescope import scan_require_one
from joulescope.data_recorder import DataRecorder
import argparse
# import datetime
import logging
import numpy as np
import os
import signal
from datetime import datetime
import time
import uuid
from ipaddress import ip_address
import subprocess
import sys
from extract_spikes import analyse, cleanup_analysis
import pandas as pd
from pathlib import Path

logger = logging.getLogger(__name__)

def get_parser():
    # Using -defikmnorstwx
    p = argparse.ArgumentParser(
        description='Run entire test process for chapter 6.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    p.add_argument('--tasks', '-t', nargs='+',
                   type=int, required=False, default=[2],
                   help='The range of task counts to test: min [max [step]].')
    p.add_argument('--sensors', '-s', nargs='+',
                   type=int, required=True,
                   help='The range of sensors to test: min [max [step]].')
    p.add_argument('--samples', '-n', nargs='+',
                   type=int, required=True,
                   help='The range of sample sizes to test: min [max [step]].')
    p.add_argument('--datagram', '-d', nargs='+',
                   type=int, required=True,
                   help='The range of datagram sizes to test: min [max [step]].')
    p.add_argument('--repeats', '-r', 
                   type=int, required=True,
                   default=1,
                   help='The number of times to repeat each test.')
    p.add_argument('--delay', '-w', 
                   type=int, required=False,
                   default=0,
                   help='Delay at start of test (millisecs).')
    p.add_argument('--ip', '-i', 
                   type=ip_address, required=False,
                   default='10.0.0.123',
                   help='IP address of Raspberry Pi.')
    p.add_argument('--keep', '-k', 
                   action='store_true',
                   help='Keep (do not delete) imtermediate files.')
    p.add_argument('--output_files_folder', '-o',
                   default='data',
                   help='Folder for output files.')
    p.add_argument('--max_retries', '-m',
                   type=int, required=False, default=3,
                   help='Maximum number of retries for each experiment.')
    p.add_argument('--dmad_threshold', '-f',
                   type=int, required=False, default=4,
                   help='Threshold for dMAD filter, used to calculate fences.')
    p.add_argument('--text_files', '-x',
                   action='store_true',
                   help='Create csv text files for spikes.')
    p.add_argument('--backfill', '-b',
                   action='store_true',
                   help='Only run test if no record exists.')  
    p.add_argument('--save_error_data', '-e',
                   action='store_true',
                   help='Save data from failed experiments.')  
    return p

def make_range(inp):
    '''Make a range from the input. The result can be passed to a for loop as a range object,
e.g. for x in range(*make_range(inp))'''
    if len(inp) == 1:
        inp = [inp[0], inp[0]]
    if len(inp) == 2:
        inp = [inp[0], inp[1], 1]
    if len(inp) > 3:
        inp = inp[:3]
    inp = [int(x) for x in inp]
    inp[1] = inp[1] + 1
    return inp  
    
def count(rng):
    '''Count the number of values in a range.'''
    return len(range(*rng))

class TestRun:
    def __init__(self, args) -> None:
        self.args = args
        self.runid = uuid.uuid4()
    
    def experiments_path(self):
        '''Return the path to the experiments file.'''
        return Path(self.args.output_files_folder) / 'experiments.csv'
    
    def spikes_path(self):
        '''Return the path to the spikes file.'''
        return Path(self.args.output_files_folder) / 'spikes.csv'
    
    def make_experiment(self, tasks, sensors, samples, datagram):
        '''Make an experiment object.'''
        return Experiment(self, tasks, sensors, samples, datagram, self.args)

    def make_experiments(self):
        '''Make a list of experiments.'''
        experiments = []
        for tasks in range(*self.args.tasks):
            for sensors in range(*self.args.sensors):
                for samples in range(*self.args.samples):
                    for datagram in range(*self.args.datagram):
                        experiments.append(self.make_experiment(tasks, sensors, samples, datagram))
        return experiments
    
    def experiment_done(self, e):
        '''Return True if the experiment has already been done.'''
        if not Path.exists(self.experiments_path()):
            return False
        df = pd.read_csv(self.experiments_path())
        df2 = df.query(f'(tasks=={e.tasks} & (sensors=={e.sensors}) & (samples=={e.samples}) & (datagram=={e.datagram}))')
        return len(df2) > 0

class Experiment:
    def __init__(self, testrun, tasks, sensors, samples, datagram, args):
        self.start_time = datetime.now()
        self.testrun = testrun
        self.tasks = tasks
        self.sensors = sensors
        self.samples = samples
        self.datagram = datagram
        self.args = args
        self.experimentid = uuid.uuid4()
        self.winscriptfolder = os.path.dirname(os.path.realpath(__file__))
        self.pirootfolder = '~/repos/coro_edge_energy/cppsource/'
        self.script = self.pirootfolder + 'scripts/run_infer_l_sync.sh'
        self.reportfile = self.pirootfolder + f'results/{self.experimentid}_report.csv'
        self.perffile = self.pirootfolder + f'results/{self.experimentid}_perf.csv'
        self.perffilecopy = f"{self.args.output_files_folder}\\{self.experimentid}_perf.csv"
        self.jlsfile = f"{self.args.output_files_folder}\\{self.experimentid}.jls"
        self.dmad_threshold = args.dmad_threshold
        self.data = None
        self.results = None

    def append_results(results):
        '''Append results to the results CSV file. This is a stub.'''
        pass

    def create_ssh_cmd_line(self):
        '''Create the command line for the SSH call.'''
        cmd = (f"ssh {self.args.ip} {self.script} -i -v 1 " +
            f"-b {self.args.delay} -a {self.args.repeats} " +
            f"-s {self.sensors} -c {self.samples} -d {self.datagram} -t {self.tasks} " +
            f"-y {self.reportfile} -f {self.perffile}")
        return cmd

    def run_ssh(self, cmd):
        '''Start the SSH server on the Raspberry Pi. See https://stackoverflow.com/a/57439663'''
        logger.info(f"Running SSH server with command: {cmd}")
        stdout, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()

    def create_scp_cmd_line(self):
        '''Create the command line for the scp call.'''
        cmd = f"scp {self.args.ip}:{self.perffile} {self.perffilecopy}"
        return cmd

    def run_scp(self, cmd):
        '''Start the SSH server on the Raspberry Pi. See https://stackoverflow.com/a/57439663'''
        logger.info(f"Running scp with command: {cmd}")
        stdout, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()

    def remove_via_ssh(self, target):
        cmd = f"ssh {self.args.ip} rm target"
        logger.info(f"Running SSH server with command: {cmd}")
        stdout, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()

    def import_perf_file(self):
        '''Import the performance file.'''
        logger.info(f"Importing per file {self.perffilecopy}")
        dfp = pd.read_csv(self.perffilecopy, index_col=False)
        dfp = dfp.drop(dfp[dfp.step == 0].index) # Drop first of each seq pair
        # spike_data arrays are ordered by model, then repeat
        dfp = dfp.sort_values(by=['model', 'repeat'])
        for name in ["cpu_cycles", "instructions", "d_cache_reads" ,"d_cache_misses"]:
            self.spike_data[name] = dfp[name].tolist();

    def summary(self):
        return f"Tasks: {self.tasks}, Sensors: {self.sensors}, Samples: {self.samples}, Datagram: {self.datagram}, DeltaT: {self.statistics['duration_deltas_mean']*100:.2f}%, DeltaP: {self.statistics['spike_power_deltas_mean']*100:.2f}%, DeltaPAdj: {self.statistics['adjusted_power_deltas_mean']*100:.2f}%"

    def process_joulescope_file(self):
        self.statistics, self.spike_data = analyse(self.experimentid, self.args.repeats, self.args.keep, 
                                                   log_level_passed=log_level, 
                                                   data_dir=self.args.output_files_folder,
                                                   dmad_threshold=self.dmad_threshold,
                                                   save_text_files=self.args.text_files)
        self.end_time = datetime.now()

        # Enrich main statistical data with arguments
        tmp_summary = {
            'runid': self.testrun.runid,
            'experimentid': self.experimentid,
            'tasks': self.tasks,
            'sensors': self.sensors,
            'samples': self.samples,
            'datagram': self.datagram,
            'repeats': self.args.repeats,
            'start_time': self.start_time,
            'end_time': self.end_time
        }
        tmp_summary.update(self.statistics)
        tmp_summary.pop('uuid') # Duplicate to Experiment.experimentid
        self.statistics = tmp_summary

    def save_results(self):
        # Save to CSV files
        dfs = pd.DataFrame(self.spike_data)
        spike_output_file = f'{self.args.output_files_folder}\\spikes.csv'
        dfs.to_csv(spike_output_file, mode='a', header=not os.path.exists(spike_output_file), index=False)

        # Save to experiment file last, so that there is no record if save to spike fails
        df = pd.DataFrame(self.statistics, index=[0])
        main_output_file = f'{self.args.output_files_folder}\\experiments.csv'
        df.to_csv(main_output_file, mode='a', header=not os.path.exists(main_output_file), index=False)

    def run(self):
        '''Run the experiment. This is the main function.'''  
        logger.info(f"Running experiment {self.experimentid} with {self.tasks} tasks, {self.sensors} sensors, {self.samples} samples, {self.datagram} datagram.")
        self.start_time = datetime.now()
        
        # quit_ = False
        # def do_quit(*args, **kwargs):
        #     nonlocal quit_
        #     quit_ = 'quit from SIGINT'
        # signal.signal(signal.SIGINT, do_quit)

        ssh_cmd = self.create_ssh_cmd_line()

        device = scan_require_one(config='auto')
        with device:
            recorder = DataRecorder(self.jlsfile)#, calibration=device.calibration)
            try:
                device.stream_process_register(recorder)
                logger.debug('Device starting')
                device.start()
                logger.debug('Device started')
                self.run_ssh(ssh_cmd)
                time.sleep(0.1) # wait for end of last spike before stopping
                device.stop()
                logger.debug('Device stopped')
            except Exception as ex:
                logger.error(f'Exception: {ex}')
                raise ex
            finally:
                recorder.close()
                device.stream_process_unregister(recorder)
                recorder = None
                logger.debug('Recorder closed')

        self.process_joulescope_file()
        self.run_scp(self.create_scp_cmd_line())
        self.import_perf_file()

        self.save_results()
        logger.info(f"End of experiment {self.experimentid}.")

    def remove_with_delay(self, filename, delay = 0.1):
        '''Remove a file and try again after  a delay.'''
        if not os.path.isfile(filename):
            return
        max_attempts = 3
        attempts = 0
        while attempts < max_attempts:
            try:
                attempts = attempts + 1
                os.remove(filename)
                break
            except PermissionError as e:
                if attempts > max_attempts:
                    raise e
                else:
                    time.sleep(delay)

    def cleanup(self, always_delete=None, always_keep=None):
        '''Cleanup after the experiment.'''
        logger.info(f"Cleaning up experiment {self.experimentid}. keep={self.args.keep} always_delete={always_delete} always_keep={always_keep}.")
        keep_used = (self.args.keep and not always_delete) or always_keep
        # Delete raw data file
        if not(keep_used):
            self.remove_with_delay(self.jlsfile)
        # Delete spike files
        if not(keep_used):
            cleanup_analysis(self.experimentid)
        if not(keep_used):
            self.remove_with_delay(self.perffilecopy)
        if not(keep_used):
            self.remove_via_ssh(self.perffile)

def run_all(override_args=None):
    args = get_parser().parse_args(override_args)
    args.tasks = make_range(args.tasks)
    args.sensors = make_range(args.sensors)
    args.samples = make_range(args.samples)
    args.datagram = make_range(args.datagram)

    testrun = TestRun(args)
    print(f'Starting experiments. runid = {testrun.runid}.')

    logger.info(f'Starting experiments. runid = {testrun.runid}.')
    logger.info('Arguments:')
    logger.info(args)
    logger.info(f'Python version: {sys.version}')
    logger.info(f'Loop counts:- Tasks: {count(args.tasks)}; Sensors: {count(args.sensors)}; Samples: {count(args.samples)}; Datagram: {count(args.datagram)}; Total: {count(args.tasks)*count(args.sensors)*count(args.samples)*count(args.datagram)} experiments.')

    Path(args.output_files_folder).mkdir(parents=True, exist_ok=True)

    all_experiments = testrun.make_experiments()
    for experiment in all_experiments:
        skip = False
        if args.backfill:
            if testrun.experiment_done(experiment):
                skip = True
        if skip:
            logger.info(f"Skipping tasks={experiment.tasks}, sensors={experiment.sensors}, samples={experiment.samples}, datagram={experiment.datagram} (experiment {experiment.experimentid}).")
        if not skip:
            retries = 0
            done = False
            always_delete = False
            while (not done) and (retries < args.max_retries):
                try:
                    always_delete = False
                    experiment.run()
                    print(experiment.summary())
                    done = True
                except Exception as e:
                    logger.error(f"Error in attempt {retries + 1} of experiment {experiment.experimentid}: {e}.")
                    retries = retries + 1;
                    always_delete = True
                finally:
                    experiment.cleanup(always_delete, args.save_error_data) # always_delete is True if there was an error
            # If failure was repeated, give up now on the whole set of experiments
            if retries >= args.max_retries:
                logger.fatal(f"Run {testrun.runid}: too many failures in experiment {experiment.experimentid}. Giving up.")
                exit(-1)

if __name__ == '__main__':
    ## Logging
    # Set local log level to WARNING, unless overridden by environment variable
    log_level = os.environ.get('VANET_LOG_LEVEL', 'WARNING').upper()
    logger.setLevel(log_level)
    # Set global log level to WARNING
    logging.basicConfig(format='%(asctime)s %(levelname)-8s %(name)s %(message)s', level='WARNING')
    # Attach a file logger
    fh = logging.FileHandler('logs\\vanet.log')
    fh.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(name)s %(message)s'))
    logger.addHandler(fh)

    ## Go
    run_all()
