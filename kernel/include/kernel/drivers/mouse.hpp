#include <kernel/drivers/interrupts.hpp>
#include <kernel/datastr/queue.hpp>
#include <kernel/drivers/ps2.hpp>
#include <stdint.h>
/**
	\brief Which button has been clicked
 */
enum ButtonEvent : uint8_t {
	BTN_NONE = 0,
	LEFT_CLICK = 1,
	RIGHT_CLICK = 2,
	MIDDLE_CLICK = 4
};
/**
	\brief Mouse event
	xdisp: Displacement in x
	ydisp: Displacement in y
	zdesp: Wheel's displacement
	button_clicked: Bitmap of which buttons have been clicked
 */
struct MouseEvent {
	int16_t xdisp, ydisp, zdesp;
	uint8_t button_clicked;
};

/**
	\brief PS/2 mouse driver.
	Implemented as a singleton.
 */
class Mouse {
	public:
		/**
			\brief Get singleton instance
		 */
		static Mouse& get();
		/**
			\brief Initialise mouse
		 */
		void init();
	private:
		/**
			\brief Mouse interrupt handler
		 */
		[[gnu::interrupt]]
		static void mouse_handler(interrupt_frame*);
		/**
			\brief Current mouse event being processed
		 */
		MouseEvent cur_mouse_event;
		/**
			\brief Mouse events queue
		 */
		StaticQueue<MouseEvent, 128> events_queue;
		// Whether the sign bit is active
		bool negx = false, negy = false;
		// Try to initialise the scrollwheel
		void try_init_wheel();
		// Port of the mouse
		size_t port;
		// Interrupt Request Line
		size_t irqline;
		// Number of bytes per packet
		size_t nbytes;
		// Type of mouse
		PS2Controller::DeviceType type;
		// Whether the mouse is being initialised
		// and so we should ignore any interrupts
		bool initialising = true;
		// Which byte am I in
		enum MouseState {
			FIRST_BYTE,
			SECOND_BYTE,
			THIRD_BYTE,
			FOURTH_BYTE
		} state;
		Mouse(){};
};