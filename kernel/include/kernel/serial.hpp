#pragma once

class Serial {
	public:
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

extern Serial serial;