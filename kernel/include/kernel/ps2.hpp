class PS2Controller {
	public:
		static PS2Controller &get();
		void init();
	private:
		void configure_first_port();
		void configure_second_port();
		void self_test();
		bool two_channels();
		bool test();
		void enable_devices();

		bool has_two_channels;
		PS2Controller() {}
};
