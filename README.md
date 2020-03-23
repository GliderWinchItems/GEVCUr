# GEVCUr
This goal of this project is to replace the GEVCU unit that control the DMOC (motor inverter) with our unit (hence the 'r') as a step towards developing the master control for the winch. The open source, source code for the GEVCU was used as a guide for dmoc_control.c.

The project is based on the STM32CubeMX, FreeRTOS, and ST DiscoveryF4 board. It uses the control panel developed for the 'mc' (Master Controller) that can be found in GliderWinchCommons (which was based on bare-metal programs).

Electric winch two motor testing 

Two motors are coupled via a Gates belt.  One motor operates in torque mode and the other acts as a regenerative brake in speed mode.  The battery bank supplies power to make up for losses.  Other instrumentation measures drive belt tension, and inverter currents, etc.

WARNING: Use at your own risk. There is a lot of code in this project,and many routines have not been reviewed with others, so bugs, errors, etc., can be expected even though it may appear to "work."
