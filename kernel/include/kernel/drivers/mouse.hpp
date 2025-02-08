#include <kernel/drivers/interrupts.hpp>
#include <kernel/datastr/queue.hpp>
#include <kernel/hal/managers/ps2manager.hpp>
#include <kernel/hal/drvbase/mouse.hpp>
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
class PS2Mouse : public hal::Mouse {
	public:

		/**
			\brief Initialise mouse
		 */
		void init() override;
		void handle_interrupt() override;
	private:
		/**
			\brief Current mouse event being processed
		 */
		MouseEvent cur_mouse_event = {0,0,0,0};
		/**
			\brief Mouse events queue
		 */
		StaticQueue<MouseEvent, 128> events_queue;
		// Whether the sign bit is active
		bool negx = false, negy = false;
		// Try to initialise the scrollwheel
		void try_init_wheel();
		// Port of the mouse
		size_t port = 0;
		// Interrupt Request Line
		size_t irqline = 0;
		// Number of bytes per packet
		size_t nbytes = 0;
		// Type of mouse
		hal::PS2SubsystemManager::DeviceType type = hal::PS2SubsystemManager::DeviceType::NONE;
		// Whether the mouse is being initialised
		// and so we should ignore any interrupts
		bool initialising = true;
		// Which byte am I in
		enum MouseState {
			FIRST_BYTE,
			SECOND_BYTE,
			THIRD_BYTE,
			FOURTH_BYTE
		} state = FIRST_BYTE;
};