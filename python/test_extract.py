#!/usr/bin/env python3

import extract_spikes

if __name__ == '__main__':
    # extract_spikes.copy_data('data\\ffb95ac1-cc53-423c-b330-d36aade28d1e.jls', 1774011, 1850959, 'tmp.jls')
    # extract_spikes.analyse('ffb95ac1-cc53-423c-b330-d36aade28d1e', expected_repeats=10, keep_files=True, log_level_passed='DEBUG')
    extract_spikes.analyse('ad837acc-3f9d-4a37-ae4a-230187cfbc97', expected_repeats=10, keep_files=True, log_level_passed='DEBUG')
  