#include <kernel/drivers/interrupts.hpp>
#include <kernel/datastr/queue.hpp>
#include <kernel/drivers/ps2.hpp>
#include <stdint.h>

enum ButtonEvent : uint8_t {
	BTN_NONE = 0,
	LEFT_CLICK = 1,
	RIGHT_CLICK = 2,
	MIDDLE_CLICK = 4
};

struct MouseEvent {
	int16_t xdisp, ydisp, zdesp;
	uint8_t button_clicked;
};

class Mouse {
	public:
		static Mouse& get();
		void init();
	private:
		[[gnu::interrupt]]
		static void mouse_handler(interrupt_frame*);
		MouseEvent cur_mouse_event;
		StaticQueue<MouseEvent, 128> events_queue;
		bool negx = false, negy = false;

		void try_init_wheel();
		size_t port;
		size_t irqline;
		size_t nbytes;
		PS2Controller::DeviceType type;
		bool initialising = true;
		enum MouseState {
			FIRST_BYTE,
			SECOND_BYTE,
			THIRD_BYTE,
			FOURTH_BYTE
		} state;
		Mouse(){};
};