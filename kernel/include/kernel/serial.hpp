#pragma once
// UART serial transmitter/receiver
class Serial {
	public:
		static Serial& get();
		void init();
		bool is_faulty();
		bool received();
		char read();
		bool is_transmit_empty();
		void write(char a);
		void print(const char *str);
	private:
		bool faulty = false;
};