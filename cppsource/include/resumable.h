/*
 * resumable.h
 *
 *  Created on: 25 Jul 2018
 *      Author: belsonba
 */

#ifndef SOURCES_RESUMABLE_H_
#define SOURCES_RESUMABLE_H_

#if defined(_MSC_VER)
#include <experimental/coroutine>
using namespace std::experimental;
#elif defined(__clang__)
#include <experimental/coroutine>
using namespace std::experimental;
#else
#include <coroutine>
#endif
#include <exception>

using namespace std;

namespace std {

struct resumable {
	struct promise_type {
    bool is_complete;
    promise_type() : is_complete(false) {}
		resumable get_return_object() {
			return resumable(
					coroutine_handle < promise_type > ::from_promise(*this));
		}
		auto initial_suspend() noexcept {
			return suspend_always { };
		}
		auto final_suspend() noexcept {
			return suspend_always { };
		}
		void return_void() {
      is_complete = true;
		}
		void unhandled_exception() {
			std::terminate();
		}
	};
	coroutine_handle<promise_type> _coroutine; // = nullptr;
	resumable() = default;
	resumable(resumable const&) = delete;
	resumable& operator=(resumable const&) = delete;
	resumable(resumable&& other) noexcept :
			_coroutine(other._coroutine) {
		other._coroutine = nullptr;
	}
	resumable& operator =(resumable&& other) noexcept {
		if (&other != this) {
			_coroutine = other._coroutine;
			other._coroutine = nullptr;
		}
		return *this;
	}
	explicit resumable(coroutine_handle<promise_type> coroutine) :
			_coroutine(coroutine) {
	}
	~resumable() {
		if (_coroutine) {
			_coroutine.destroy();
		}
	}
	bool resume() {
		if (!_coroutine.done())
			_coroutine.resume();
		return !_coroutine.done();
	}
  bool is_complete() {
    return _coroutine.promise().is_complete;
  }
};

} // std

#endif /* SOURCES_RESUMABLE_H_ */
