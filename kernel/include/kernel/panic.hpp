namespace kn {
	/**
		\brief kernel panic with a message.
		Used to signal an unrecoverable error.
		Disables interruptions and completely halts
		the computer.
	*/
	void panic(const char *str);
}