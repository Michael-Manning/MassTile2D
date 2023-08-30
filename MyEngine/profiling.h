#pragma once

#include <chrono>
#include <thread>  
#include <iostream>

#define _PROFILE_concat_impl(arg1, arg2) arg1 ## arg2
#define _PROFILE_concat(arg1, arg2) _PROFILE_concat_impl(arg1, arg2)

#define PROFILE_START(s) auto s = std::chrono::high_resolution_clock::now()

#define PROFILE_END(s) auto _PROFILE_concat(end,__LINE__) = std::chrono::high_resolution_clock::now(); \
					   auto _PROFILE_concat(duration,__LINE__) = std::chrono::duration_cast<std::chrono::microseconds>(_PROFILE_concat(end,__LINE__) - s); \
					   std::cout << "Profile " << #s << ": " << _PROFILE_concat(duration,__LINE__) << std::endl