#pragma once

#ifndef SOURCES_RESUMABLE_H_
#error This file requires <resumable.h>
#endif

template<typename PREFETCHER_T, typename REFDATA_T, typename RESUMABLE_T = resumable>
class coroutine_runner
{
public:
  typedef RESUMABLE_T (*coro_fn_t)(
    const PREFETCHER_T &prefetcher, 
    REFDATA_T& refdata,
    size_t coroutine_index); 
  
  coroutine_runner(const PREFETCHER_T& prefetcher, REFDATA_T& refdata)
  : prefetcher_(prefetcher), refdata_(refdata)
  {
  }

  void run(size_t coroutine_count, size_t item_count, coro_fn_t coro_fn)
  {
    // A collection of parallel tasks/coroutines
    std::vector<RESUMABLE_T> tasks;
    std::vector<bool> done(coroutine_count, false);
    size_t incomplete = item_count;

    // Create the coroutines, ready to run.
    // There will always be at most coroutine_count of them,
    // and each will be deleted and replaced by a new one 
    // when its item is complete. If there are no further 
    // items to do, it will just be abandoned.
    for (size_t b = 0; b < coroutine_count; b++)
    {
      tasks.push_back(coro_fn(prefetcher_, refdata_, b));
    }

    // Work through all tasks until all are done
    size_t next_item = coroutine_count;
    while (incomplete > 0)
    {
      for (size_t c = 0; c < tasks.size(); c++)
      {
        if (done[c])
        {
          continue;
        }
        RESUMABLE_T &t = tasks[c];
        if (t.is_complete())
        {
          if (next_item < item_count)
          {
            tasks[c] = coro_fn(prefetcher_, refdata_, next_item);
            next_item++;
          }
          else
          {
            done[c] = true;
          }
          incomplete--;
        }
        else
        {
          t.resume();
        }
      }
    }
  }
protected:
  const PREFETCHER_T& prefetcher_;
  REFDATA_T& refdata_;
};
