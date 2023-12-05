#!/usr/bin/env python3
# TODO License

"""
Extract the ch6 spikes from a Joulescope JLS capture.

API is at https://joulescope.readthedocs.io/en/latest/

"""

from joulescope.data_recorder import DataReader, DataRecorder
import argparse
import numpy as np
import sys
import logging
import statistics
import mad as mad


MAX_SAMPLES = 1000000000 / 5  # limit to 1 GB of RAM
logger = logging.getLogger(__name__)

class ModelAnalysis:
    def __init__(self, analysis, name, index) -> None:
        self.analysis = analysis
        self.name = name
        self.index = index
        self.fieldname = f'current_lsb' if index == 0 else f'voltage_lsb'
        self.spikes_per_repeat = 2 if index == 0 else 1
        self.rising_edges = None
        self.falling_edges = None
        self.spikes = []

    def validate(self, expected_repeats):
        '''Validate the model.'''
        expected_spikes = expected_repeats * self.spikes_per_repeat
        if len(self.rising_edges) != expected_spikes or len(self.falling_edges) != expected_spikes:
            raise ValueError(f'{self.name} has {len(self.rising_edges)} rising edges and {len(self.falling_edges)} falling edges, but expected {expected_spikes} of each')
        
    def filter_pairs(self):
        '''Throw away the 1st spike of each pair (if this model has paired spikes)'''
        if self.spikes_per_repeat == 2:
            self.rising_edges = self.rising_edges[1::2]
            self.falling_edges = self.falling_edges[1::2]

class Spike:
    def __init__(self, model, index, start, end, rise, fall, endleak, included, pairincluded) -> None:
        self.model = model
        self.index = index
        self.start = start
        self.end = end
        self.rise = rise
        self.fall = fall
        self.endleak = endleak
        self.included = included
        self.pairincluded = pairincluded
        self.data = None
        self.columns = []

    def extract(self, reader, saved_fields):
        fields = [self.model.fieldname] + saved_fields
        self.columns = ['time', 'active'] + saved_fields
        # Get the data
        r_start, r_stop = reader.sample_id_range
        use_stop = min(r_stop, self.end)
        raw = reader.samples_get(start=self.start, stop=use_stop, fields=fields)
        # Create the spike array extract
        sample_range = raw['time']['sample_id_range']['value']
        time_column = np.arange(sample_range[0], sample_range[1])
        self.data = np.column_stack([time_column] + [raw['signals'][f]['value'] for f in fields])
        return raw

    def analyse(self):
        '''Analyse the spike.'''
        start, end, rise, fall, endleak = 0, self.end-self.start, self.rise-self.start, self.fall-self.start, self.endleak-self.start
        self.opening_median = np.median(self.data[start:rise, 4])
        self.closing_median = np.median(self.data[endleak:end, 4])
        self.spike_median = np.median(self.data[rise:fall, 4])    
        self.duration = (fall-rise) / self.model.analysis.sample_rate
        self.spike_power = (np.sum(self.data[rise:fall, 4]) 
                           / self.model.analysis.sample_rate)
        self.adjusted_power = (np.sum(self.data[rise:endleak, 4]-self.closing_median)
                               / self.model.analysis.sample_rate) 
        self.peak_power = np.max(self.data[rise:fall, 4])

    def save_text(self, filename):
        '''Save the spike data to a text file.
Columns are time,active,voltage,current,power'''
        np.savetxt(filename, self.data, fmt='%d,%d,%f,%f,%f', delimiter=',', header=','.join(self.columns), comments='')

class Analysis:
    def __init__(self, uuid, expected_repeats, keep_files, data_dir, dmad_threshold, save_text_files) -> None:
        self.uuid = uuid
        self.expected_repeats = expected_repeats
        self.keep_files = keep_files
        self.data_dir = data_dir
        self.dmad_threshold = dmad_threshold
        self.save_text_files = save_text_files

        self.filename = f"{data_dir}\\{self.uuid}.jls"
        self.reader = None
        self.models = [ModelAnalysis(self, 'seq', 0), ModelAnalysis(self, 'coro', 1)]
        self.max_high_interval = 0
        # Allow 1ms (> 657 microseconds - see decay.xlsx) for charge to leak out of the capacitor
        self.charge_leak_allowance = 1000 # running at 1 MHz
        self.saved_fields_common = ['voltage', 'current', 'power']

    def find_edges(self, r, field):
        '''Find the rising and falling edges of the specified field in the specified open DataReader.
r: DataReader
field: str - either "current_lsb" (in0) or "voltage_lsb" (in1)'''
        r_start, r_stop = r.sample_id_range
        r_next = r_start
        last_value = None
        all_rising_indices = np.empty(0, dtype=np.int64)
        all_falling_indices = np.empty(0, dtype=np.int64)
        while r_next < r_stop:
            r_last = r_next
            r_next = min(r_next + MAX_SAMPLES, r_stop)
            logger.debug('load %s[%d:%d]', field, r_last, r_next)
            raw = r.samples_get(start=r_last, stop=r_next, fields=[field])['signals'][field]['value']  # get the raw data
            # Get indices of edges and add the start offset (of this set of loaded data) to each index
            rising_indices = np.argwhere(np.diff(raw) == 1).flatten() + r_last
            falling_indices = np.argwhere(np.diff(raw) == -1).flatten() + r_last
            # Append first edge if needed (i.e. if first loaded value != last loaded value from last tranche)
            if last_value == 0 and raw[0] == 1:
                rising_indices = np.append(r_last - 1, rising_indices)
            if last_value == 1 and raw[0] == 0: # falling edge
                falling_indices = np.append(r_last - 1, falling_indices)
            # Prepare for next tranche
            all_rising_indices = np.append(all_rising_indices, rising_indices)
            all_falling_indices = np.append(all_falling_indices, falling_indices)
            last_value = raw[-1]
        # Return all indices
        return all_rising_indices, all_falling_indices
    
    def collate_stats(self, data):
        return {
            "mean": statistics.mean(data),
            "sd": statistics.stdev(data),
            "median": statistics.median(data),
            "min": min(data),
            "max": max(data),
        }

    def add_statistics(self, root_name, stats):
        for k, v in stats.items():
            self.statistics[f'{root_name}_{k}'] = v

    def add_2_statistics(self, root_name, stats_pair):
        for i in [0, 1]:
            self.add_statistics(f'{root_name}_{self.models[i].name}', stats_pair[i])
        
    def store_results(self):
        # Save results (both spikes and totals) to single structures
        spikes = self.models[0].spikes + self.models[1].spikes
        self.spike_collated_results = {
            "uuid": [self.uuid for s in spikes],
            "model": [s.model.name for s in spikes],
            "index": [s.index for s in spikes],
            "start": [s.start for s in spikes],
            "end": [s.end for s in spikes],
            "rise": [s.rise for s in spikes],
            "fall": [s.fall for s in spikes],
            "endleak": [s.endleak for s in spikes],
            "included": [s.included for s in spikes],
            "pairincluded": [s.pairincluded for s in spikes],
            "duration": [s.duration for s in spikes],
            "opening_median": [s.opening_median for s in spikes],
            "closing_median": [s.closing_median for s in spikes],
            "spike_median": [s.spike_median for s in spikes],
            "spike_power": [s.spike_power for s in spikes],
            "adjusted_power": [s.adjusted_power for s in spikes],
            "peak_power": [s.peak_power for s in spikes]
        }
        # print(self.collated_results)
        duration_deltas = (np.array([s.duration for s in self.models[1].spikes]) - np.array([s.duration for s in self.models[0].spikes])) / np.array([s.duration for s in self.models[0].spikes])
        spike_power_deltas = (np.array([s.spike_power for s in self.models[1].spikes]) - np.array([s.spike_power for s in self.models[0].spikes])) / np.array([s.spike_power for s in self.models[0].spikes])
        adjusted_power_deltas = (np.array([s.adjusted_power for s in self.models[1].spikes]) - np.array([s.adjusted_power for s in self.models[0].spikes])) / np.array([s.adjusted_power for s in self.models[0].spikes])
        self.statistics = { "uuid": self.uuid }
        self.add_2_statistics('duration', [self.collate_stats([s.duration for s in self.models[i].spikes]) for i in [0,1]])
        self.add_2_statistics('spike_power', [self.collate_stats([s.spike_power for s in self.models[i].spikes]) for i in [0,1]])
        self.add_2_statistics('adjusted_power_statistics', [self.collate_stats([s.adjusted_power for s in self.models[i].spikes]) for i in [0,1]])
        self.add_statistics('duration_deltas', self.collate_stats(duration_deltas))
        self.add_statistics('spike_power_deltas', self.collate_stats(spike_power_deltas))
        self.add_statistics('adjusted_power_deltas', self.collate_stats(adjusted_power_deltas))

    # def save_extract(self, spike, model_name, index):
    #     # Save the spike data and surrounding areas
    #     save_file = f'{self.data_dir}\\{self.uuid}_{model_name}_{index}.jls'
    #     save_jls_extract(self.reader, spike.start, spike.end, save_file)

    def analyse(self):
        self.reader = DataReader().open(self.filename)
        self.sample_rate = self.reader.input_sampling_frequency
        _, max_sample_id = self.reader.sample_id_range
        try:     
            # Extract all spike positions
            for model in self.models:
                model.rising_edges, model.falling_edges = self.find_edges(self.reader, model.fieldname)
                model.validate(self.expected_repeats)
            
            # Throw away the 1st spike of each pair (if model has paired spikes) and find the max high interval
            for model in self.models:
                model.filter_pairs()
                # self.max_high_interval = max(self.max_high_interval, np.max(model.falling_edges - model.rising_edges))

            # Filter spikes based on DMAD of duration
            for model in self.models:
                model.included = mad.filter_by_dmad(model.falling_edges - model.rising_edges, self.dmad_threshold)
                logger.info(f'{model.name} included:')
                logger.info(f'{model.included}')
                logger.info(f'{model.name} durations (us):')
                logger.info(f'{model.falling_edges - model.rising_edges}')

            # Filter based on matching spike (both models must be included, or neither)
            self.spike_included = np.logical_and(self.models[0].included, self.models[1].included)

            # Now, find the max high interval
            for model in self.models:
                model.max_high_interval = max(model.falling_edges[self.spike_included] - model.rising_edges[self.spike_included])
                self.max_high_interval = max(self.max_high_interval, model.max_high_interval)

            # Extract all spike data and surrounding areas 
            for model in self.models:
                # Extract the spike data and surrounding areas
                for i in range(len(model.rising_edges)):
                    spike = Spike(model, i, 
                                  model.rising_edges[i] - self.max_high_interval, # start
                                  min(model.falling_edges[i] + self.charge_leak_allowance + self.max_high_interval, max_sample_id), #end
                                  model.rising_edges[i], model.falling_edges[i], # rise, fall
                                  min(model.falling_edges[i] + self.charge_leak_allowance, max_sample_id), # endleak
                                  model.included[i], self.spike_included[i])
                    spike.extract(self.reader, self.saved_fields_common)
                    model.spikes.append(spike)
            
            # Analyse each spike
            # logger.info(f'model,index,duration,opening_median,closing_median,total_power_within_spike,adjusted_power')
            for model in self.models:
                for spike in model.spikes:
                    spike.analyse()

            # Copy results (both spikes and totals) to unified in-memory structures
            self.store_results()
            self.validate()

            # Save results to file (for audit)
            if self.keep_files:
                for model in self.models:
                    for spike in model.spikes:
                        save_jls_extract(self.reader, int(spike.start), int(spike.end), 
                                         f'{self.data_dir}\\{self.uuid}_{spike.model.name}_{spike.index}.jls')
            if self.save_text_files:
                for model in self.models:
                    for spike in model.spikes:
                        spike.save_text(f'{self.data_dir}\\{self.uuid}_{spike.model.name}_{spike.index}.csv')

        finally:
            self.reader.close()
            self.reader = None

        return self.statistics, self.spike_collated_results

    def validate(self):
        pass

      
        

    def dump(self):
        for model in self.models:
            # print(f'{model.name} rising edges: {len(model.rising_edges)}')
            # print(f'{model.name} falling edges: {len(model.falling_edges)}')
            print(f'{model.name} high intervals: ')
            print(model.falling_edges - model.rising_edges)
        print(f'max high interval: {self.max_high_interval}')


def save_jls_extract(reader, start_id, stop_id, save_file_path):
    logger.debug(f'save_jls_extract starts {start_id} ends {stop_id} to {save_file_path}')
    sample_count = stop_id - start_id
    block_size = int(reader.sampling_frequency)
    block_count = (sample_count + block_size - 1) // block_size

    writer = DataRecorder(save_file_path)
    fields = reader.config['reduction_fields']
    try:
        writer.output_sampling_frequency = reader.input_sampling_frequency
        for block in range(block_count):
            offset = start_id + (block * block_size)
            offset_next = offset + block_size
            if offset_next > sample_count:
                offset_next = stop_id
            data = reader.samples_get(offset, offset_next, fields=fields)
            writer.insert(data)
    finally:
        writer.close()

# def copy_data(filename, start, stop, save_file_path):
#     r = DataReader().open(filename)
#     try:
#         save_jls_extract(r, start, stop, save_file_path)
#     except (Exception) as e:
#         logger.error(f"Error during extract: {e}.")
#     finally:
#         r.close()

def analyse(uuid, expected_repeats, keep_files, log_level_passed=None, data_dir='data', 
            dmad_threshold=4.0, save_text_files=False):
    if log_level_passed is not None:
        logger.setLevel(log_level_passed)
    logger.info(f'Analysing {uuid}')
    a = Analysis(uuid, expected_repeats, keep_files, data_dir, dmad_threshold, save_text_files)
    a.analyse()
    return a.statistics, a.spike_collated_results

def cleanup_analysis(uuid):
    pass

if __name__ == '__main__':
    # log_level = os.environ.get('VANET_LOG_LEVEL', 'WARNING').upper()
    log_level = 'INFO'
    logging.basicConfig(format='%(asctime)s %(levelname)-8s %(name)s %(message)s', level='DEBUG')
    logger.setLevel(log_level)
    # copy_data('data\\ffb95ac1-cc53-423c-b330-d36aade28d1e.jls', 1774011, 1850959, 'tmp.jls')
    analyse('c0f5602c-bea1-4002-8676-6a2fc8f4d00a', 32, True, log_level, 'data6', 4.0, True)
