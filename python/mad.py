#!/usr/bin/env python3

import numpy as np

CONSISTENCY_CONSTANT = 1.4826

def filter_by_mad(seq, sd_limit):
  """Locate outliers in a sequence using the median absolute deviation."""
  ad_ = np.abs(seq - np.median(seq))
  mad_ = np.median(ad_)
  return (ad_ < sd_limit * CONSISTENCY_CONSTANT * mad_)


def filter_by_dmad(seq, sd_limit):
  """Locate outliers in a sequence using the median absolute deviation.
  See https://eurekastatistics.com/using-the-median-absolute-deviation-to-find-outliers/"""
  ad_ = np.abs(seq - np.median(seq))
  mad_ = np.median(ad_)
  lmad_ = np.median(ad_[seq <= np.median(seq)])
  rmad_ = np.median(ad_[seq >= np.median(seq)])
  lfence_ = np.median(seq) - (sd_limit * CONSISTENCY_CONSTANT * lmad_)
  rfence_ = np.median(seq) + (sd_limit * CONSISTENCY_CONSTANT * rmad_)
  return (seq <= rfence_) & (seq >= lfence_)

if __name__ == '__main__':
  import sys

  limit = float(sys.argv[1]) if len(sys.argv) >= 2 else 3.0
  print('limit: ', limit)

  seq_dur = np.array([6594,6601,6612,6608,6602,6605,6634,6582,6582,6606])
  coro_dur = np.array([18541,6209,6239,6232,6205,6203,6208,6227,6233,6273])

  seq_filt = filter_by_mad(seq_dur, limit)
  print('seq   mad: ', seq_filt)
  seq_dfilt = filter_by_dmad(seq_dur, limit)
  print('seq  dmad: ', seq_dfilt)

  coro_filt = filter_by_mad(coro_dur, limit)
  print('coro  mad: ', coro_filt)
  coro_dfilt = filter_by_dmad(coro_dur, limit)
  print('coro dmad: ', coro_dfilt)



