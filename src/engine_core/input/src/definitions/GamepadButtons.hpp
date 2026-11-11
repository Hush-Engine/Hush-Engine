#pragma once
#include "HushBindings.hpp"

// Standardized buttons for gamepads, no confusion

namespace Hush
{
	/// @brief Maps directly to the SDL definition of Axis
	enum class [[hush::export]] EGamepadAxis
	{
		Invalid = -1,
		LeftX,
		LeftY,
		RightX,
		RightY,
		LeftTrigger,
		RightTrigger,
		Max
	};

	enum class [[hush::export]] EGamepadButton
	{
		Invalid = -1,
		South, /**< Bottom face button (e.g. Xbox A button) */
		East,  /**< Right face button (e.g. Xbox B button) */
		West,  /**< Left face button (e.g. Xbox X button) */
		North, /**< Top face button (e.g. Xbox Y button) */
		Back,
		Guide,
		Start,
		LeftStick,
		RightStick,
		LeftShoulder,
		RightShoulder,
		DpadUp,
		DpadDown,
		DpadLeft,
		DpadRight,
		Misc1, /**< Additional button (e.g. Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro
				  capture button, Amazon Luna microphone button, Google Stadia capture button) */
		RightPaddle1, /**< Upper or primary paddle, under your right hand (e.g. Xbox Elite paddle P1, DualSense Edge RB
						 button, Right Joy-Con SR button) */
		LeftPaddle1,  /**< Upper or primary paddle, under your left hand (e.g. Xbox Elite paddle P3, DualSense Edge LB
						 button, Left Joy-Con SL button) */
		RightPaddle2, /**< Lower or secondary paddle, under your right hand (e.g. Xbox Elite paddle P2, DualSense Edge
						 right Fn button, Right Joy-Con SL button) */
		LeftPaddle2,  /**< Lower or secondary paddle, under your left hand (e.g. Xbox Elite paddle P4, DualSense Edge
						 left Fn button, Left Joy-Con SR button) */
		Touchpad,	  /**< PS4/PS5 touchpad button */
		Misc2,		  /**< Additional button */
		Misc3,		  /**< Additional button (e.g. Nintendo GameCube left trigger click) */
		Misc4,		  /**< Additional button (e.g. Nintendo GameCube right trigger click) */
		Misc5,		  /**< Additional button */
		Misc6,		  /**< Additional button */
		Max,

		// Platform specific for convinience
		XboxA = South,
		XboxY = North,
		XboxB = East,
		XboxX = West,
		XboxLB = LeftShoulder,
		XboxRB = RightShoulder,

		PlaystationCross = South,
		PlaystationTriangle = North,
		PlaystationCircle = East,
		PlaystationSquare = West,
		PlaystationL1 = LeftShoulder,
		PlaystationR1 = RightShoulder,

		SwitchB = South,
		SwitchX = North,
		SwitchA = East,
		SwitchY = West,
		SwitchL = LeftShoulder,
		SwitchR = RightShoulder
	};
	// HACK: Do not use inheritance on the enum but this alias for its sizing due to limitations of the hush::export
	// tool
	using EGamepadButton_t = int8_t;
} // namespace Hush
