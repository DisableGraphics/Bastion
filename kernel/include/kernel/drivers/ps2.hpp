#pragma once
#include <stdint.h>
namespace hal {
	/**
		\brief PS/2 Driver.
		Implemented as a singleton.
	*/
	class PS2SubsystemController {
		public:
			enum DeviceType {
				MOUSE = 0x00,
				MOUSE_SCROLLWHEEL = 0x03,
				MOUSE_5BUTTON = 0x05,
				MF2_KEYBOARD,
				SHORT_KEYBOARD,
				NCD_122_KEYBOARD,
				KEYBOARD_122_KEY,
				JP_G_KEYBOARD,
				JP_P_KEYBOARD,
				JP_A_KEYBOARD,
				NCD_SUN_KEYBOARD,
				NONE
			};
			/**
				\brief Get singleton instance
			*/
			static PS2SubsystemController &get();
			/**
				\brief Initialise PS/2 Controller
			*/
			void init();
			/**
				\brief Get which port is the keyboard's.
				\return the keyboard port or -1 if there isn't
				a keyboard connected.
			*/
			int get_keyboard_port();
			/**
				\brief Get which port is the mouse's
				\return the mouse port or -1 if there isn't a mouse
			*/
			int get_mouse_port();
			/**
				\brief Get device type connected to the port port
			*/
			DeviceType get_device_type(int port);
			/**
				\brief Write to port port the command command
			*/
			void write_to_port(uint8_t port, uint8_t command);
		private:
			/**
				\brief Configure first port
			*/
			void configure_first_port();
			/**
				\brief Configure second port
			*/
			void configure_second_port();
			/**
				\brief check if the PS/2 controller is correct
			*/
			void self_test();
			/**
				\brief Whether this PS/2 Controller has two channels or not
			*/
			bool two_channels();
			/**
				\brief Tests whether both (or one if we have only one port) ports are ok 
			*/
			bool test();
			/**
				\brief Enable devices connected to the port
			*/
			void enable_devices();
			/**
				\brief Reset devices and detect which types are they
			*/
			void reset_and_detect_devices();

			/**
				\brief handler for when the timeout expires
			*/
			static void on_timeout_expire(volatile void* arg);

			/**
				\brief Whether this PS/2 Controller has two ports
			*/
			bool has_two_channels = false;
			/**
				\brief Whether the timeout has expired or not
			*/
			volatile bool timeout_expired = false;

			/**
				\brief Device type
			*/
			DeviceType first_port_device = DeviceType::NONE, second_port_device = DeviceType::NONE;		
			/**
				\brief Set device type for port port
			*/
			void set_device_type(int port, DeviceType type);

			PS2SubsystemController() {}
	};
}