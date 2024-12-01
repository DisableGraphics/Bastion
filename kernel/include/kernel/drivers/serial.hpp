#pragma once
/**
	\brief UART serial transmitter/receiver.

	Implemented as a singleton
*/
class Serial {
	public:
		/**
			\brief Get singleton instance
		 */
		static Serial& get();
		/**
			\brief Initialize the serial
		 */
		void init();
		/**
			\brief Wether the serial is faulty or not.

			If this returns true you *really* shouldn't use the serial.
			Unless you don't care your data is sent to the shadow.
			realm or corrupted
		 */
		bool is_faulty();
		/**
			\brief Whether the serial has received data or not
		 */
		bool received();
		/**
			\brief Read a character from serial.

			This operation blocks the thread of execution 
			until it receives all the data.
		 */
		char read();
		/**
			\brief Whether the transmit buffer is empty or not
		 */
		bool is_transmit_empty();
		/**
			\brief Write a byte to the serial.
			
			This operation blocks the calling thread of execution.
		 */
		void write(char a);
		/**
			\brief Write an array of bytes to the serial.

			This operation blocks the calling thread of execution.
			Depending on the size of the array this will take an eternity,
			so use this sporadically at best.
		 */
		void print(const char *str);
	private:
		bool faulty = false;
};