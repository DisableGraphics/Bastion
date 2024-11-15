class PS2Controller {
	public:
		static PS2Controller &get();
		void init();
	private:
		PS2Controller() {}
};
