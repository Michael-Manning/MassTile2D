#pragma once

#include <chrono>
#include <thread>  
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stack>
#include <set>


/*
	Simple one time profiling. Use for startup or benchmarking.
	Not meant for use with repeated functions, use tracy for that.

	USAGE: 
	
	Use PROFILE_START(something_unique) and PROFILE_END(something_unique) to output the time for a section

	Use PROFILE_SCOPE; to profile a scope labled by function name, or PROFILE_SCOPEN(some_label) to profile a scope with a custom label

	Note: use PROFILE_SCOPE for nested scope statistics
*/


#define _PROFILE_concat_impl(arg1, arg2) arg1 ## arg2
#define _PROFILE_concat(arg1, arg2) _PROFILE_concat_impl(arg1, arg2)

#if defined(_DEBUG) || defined(PROFILE_IN_RELEASE)

static std::string autoTimeScale(unsigned long long microseconds) {
	double value = static_cast<double>(microseconds);
	std::string unit = "us"; // default to microseconds

	// Check for conversion to milliseconds
	if (value >= 1000) {
		value /= 1000;
		unit = "ms";
		// Check for conversion to seconds
		if (value >= 1000) {
			value /= 1000;
			unit = "s";
		}
	}

	// Format the output with up to 1 decimal place for milliseconds and seconds
	std::ostringstream out;
	if (unit == "ms" || unit == "s") {
		out << std::fixed << std::setprecision(2) << value;
	}
	else {
		out << microseconds; // keep as integer for microseconds
	}

	return out.str() + unit;
}


#define PROFILE_START(s) auto _profileInstance_##s = std::chrono::high_resolution_clock::now();

#define PROFILE_END(s) auto _PROFILE_concat(end,__LINE__) = std::chrono::high_resolution_clock::now(); \
					   auto _PROFILE_concat(duration,__LINE__) = std::chrono::duration_cast<std::chrono::microseconds>(_PROFILE_concat(end,__LINE__) - _profileInstance_##s); \
					   std::cout << "Duration " << #s << ": " << autoTimeScale(_PROFILE_concat(duration,__LINE__).count()) << "\n";

#define PROFILE_SCOPEN(s) _ProfileScope _profileScopeInstance_##s(#s);

#define PROFILE_SCOPE _ProfileScope _PROFILE_concat(_profileScopeInstance_, __LINE__)(__func__);

#define PROFILING_IMPL uint32_t _ProfileScope::profiling_depth = 0;\
					   std::stack<_ProfileScope::profileResult> _ProfileScope::profileStack = {};\
					   std::stack<_ProfileScope::profilePrintout> _ProfileScope::printStack = {};

class _ProfileScope {
public:
	_ProfileScope(const char* name) : name_(name), start_(std::chrono::high_resolution_clock::now()) {
		profiling_depth++;
	}

	~_ProfileScope() {
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();

		profiling_depth--;

		if (profiling_depth > 0) {
			profileStack.push(profileResult{
				.duration = static_cast<uint64_t>(duration),
				.scopeName = std::string(name_),
				.depth = profiling_depth
				});
		}
		else {
			uint64_t outerDuration = static_cast<uint64_t>(duration);
			std::stack<uint64_t> durationStack;

			while (!profileStack.empty()) {
				profileResult res = profileStack.top();
				profileStack.pop();


				printStack.push(profilePrintout{
					.duration = res.duration,
					.scopePercentage = (static_cast<double>(res.duration) / outerDuration) * 100,
					.name = res.scopeName,
					.depth = res.depth
					});

				if (!profileStack.empty()) {

					if (profileStack.top().depth > res.depth) {
						durationStack.push(outerDuration);
						outerDuration = res.duration;
					}
					else if (profileStack.top().depth < res.depth) {
						outerDuration = durationStack.top();
						durationStack.pop();
					}
				}
			}

			uint64_t nodeTotal = 0;
			std::stack<uint64_t> selfTimeStack;

			const int colA = 37;
			const int colB = 8;

			std::set<uint32_t> nestedNodes;

			while (!printStack.empty()) {

				profilePrintout res = printStack.top();
				printStack.pop();

				{
					std::stringstream str;
					str << "Duration ";
					for (uint32_t i = 0; i < res.depth - 1; i++)
					{
						if(nestedNodes.contains(i + 1))
							str << "|  ";
						else 
							str << "   ";
					}
					str << "|--" << res.name << " ";
					std::cout << std::left << std::setw(colA + res.depth * 3) << str.str() << std::setw(colB) << autoTimeScale(res.duration)
						<< " (" << std::fixed << std::setprecision(1) << res.scopePercentage << "%)" << "\n";
				}

				nodeTotal += res.duration;

				if (!printStack.empty()) {
					auto next = printStack.top();
					if (next.depth < res.depth) {
						nestedNodes.erase(res.depth);

						uint64_t selfTime = next.duration - nodeTotal;

						std::stringstream str;
						str << "Duration ";
						for (uint32_t i = 0; i < res.depth - 1; i++)
						{
							if (nestedNodes.contains(i + 1))
								str << "|  ";
							else
								str << "   ";
						}
						str << "|--" << "SELF" << " ";

						std::cout << std::left << std::setw(colA + res.depth * 3) << str.str() << std::setw(colB) << autoTimeScale(selfTime)
							<< " (" << std::fixed << std::setprecision(1) << ((static_cast<double>(selfTime) / next.duration) * 100) << "%)" << "\n";

						nodeTotal = 0;
						if (!selfTimeStack.empty()) {
							nodeTotal = selfTimeStack.top();
							selfTimeStack.pop();
						}
					}
					else if (next.depth > res.depth) {
						nestedNodes.insert(res.depth);
						for (size_t i = 0; i < (next.depth - res.depth); i++) {
							selfTimeStack.push(nodeTotal);
							nodeTotal = 0;
						}
					}
				}
				else if (nodeTotal > 0) {
					uint64_t selfTime = duration - nodeTotal;


					std::stringstream str;
					str << "Duration " << "|--SELF" << " ";
					std::cout << std::left << std::setw(colA + 3) << str.str() << std::setw(colB) << autoTimeScale(selfTime)
						<< " (" << std::fixed << std::setprecision(1) << ((static_cast<double>(selfTime) / duration) * 100) << "%)" << "\n";
				}
			}
			std::cout << "Duration " << name_ << ": " << autoTimeScale(duration) << "\n\n";
		}
	}

private:
	const char* name_;
	std::chrono::high_resolution_clock::time_point start_;

	struct profilePrintout {
		size_t duration;
		double scopePercentage;
		std::string name;
		uint32_t depth;
	};
	static std::stack<profilePrintout> printStack;

	static uint32_t profiling_depth;

	struct profileResult {
		uint64_t duration; // microseconds
		std::string scopeName;
		uint32_t depth;
	};

	static  std::stack<profileResult> profileStack;
};

#else

#define PROFILE_START(s)

#define PROFILE_END(s)

#define PROFILE_SCOPE(s)

#endif
