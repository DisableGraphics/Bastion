#pragma once
#include <kernel/drivers/interrupts.hpp>
#include <stdint.h>
#include <string.h>
#include <kernel/hal/drvbase/timer.hpp>
#ifdef __i386
#include "../arch/i386/defs/pit/pit.hpp"
#endif

// Number of max kernel countdowns available
constexpr uint32_t K_N_COUNTDOWNS = 32;
/**
	\brief Programmable Interrupt Timer driver
	Implemented as a singleton

	Not a high resolution timer.
*/
class PIT final : public hal::Timer {
	public:
		PIT();
		virtual ~PIT();
		void init() override;
		void start(uint32_t ms) override;
		void stop() override;
		size_t ellapsed() override;
		/**
			\brief Get elapsed milliseconds.

			Not the millis since boot, but the millis since PIT was initialised.
		 */
		uint32_t millis_accum();
		/**
			\brief Read the PIT count
		 */
		uint16_t read_count();
		/**
			\brief Set the PIT count
		 */
		void set_count(uint16_t count);
		/**
			\brief Sleep for millis milliseconds
			\param millis The number of milliseconds to sleep
		 */
		void sleep(uint32_t millis);
		/**
			\brief Call a function callback after millis milliseconds
			\return The timer handle. Do not forget to deallocate the handle with clear_callback().
			\param handle The timer handle. If it is UINT32_MAX, a new handle is allocated
			\param millis Milliseconds to wait until callback is called
			\param fn Function to call after millis milliseconds have passed
			\param arg Arguments
		 */
		uint32_t timer_callback(uint32_t handle, uint32_t millis, void (*fn)(void*), void *arg);
		/**
			\brief Deallocate the handle allocated with timer_callback()
			\param handle Handle of the timer
		 */
		void clear_callback(uint32_t handle);

		/**
			\brief Allocate timer. Usage of this function is not recommended.
			\return Handle of the allocated timer.
		 */
		uint32_t alloc_timer();
		/**
			\brief Deallocate handle of timer allocated with alloc_timer()
			\param handle Handle of the timer allocated with alloc_timer()
		 */
		void dealloc_timer(uint32_t handle);
		
		void handle_interrupt() override;
	private:
		/**
			\brief Initialise timer
			\param freq Frequency of the timer in Hertz (Hz)
		 */
		void init(int freq);
		/**
			\brief system_timer_fractions = Cumulative sum of all the fractions of milliseconds while the OS has been running.
			\brief system_timer_ms = Cumulative sum of all the milliseconds while the OS has been running. 
		 */
		uint32_t system_timer_fractions = 0, system_timer_ms = 0;
		/**
			\brief IRQ0_fractions = How many fractions of milliseconds have elapsed between interrupts
			\brief IRQ0_ms = How many milliseconds have elapsed between interrupts
		 */
		volatile uint32_t IRQ0_fractions = 0, IRQ0_ms = 0;
		/**
			\brief IRQ0_frecuency = Frecuency of the timer in hertz
		 */
		uint32_t IRQ0_frequency = 0;
		/**
			\brief Reload value for the PIT
		 */
		uint16_t PIT_reload_value = 0;
		/**
			\brief kernel_countdowns. Array of millisecond countdowns.
		 */
		volatile int kernel_countdowns[K_N_COUNTDOWNS];
		/**
			\brief callbacks: Array of callbacks to be called after calling timer_callback
		 */
		void (*callbacks[K_N_COUNTDOWNS])(void*);
		/**
			\brief callback_args: Array of callback arguments
		 */
		void *callback_args[K_N_COUNTDOWNS];
		/**
			\brief Bitmap for all allocated timers.
		 */
		uint32_t allocated = 0;
		
};