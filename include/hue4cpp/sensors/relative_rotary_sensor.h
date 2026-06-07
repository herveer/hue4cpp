#pragma once

#include "sensor_base.h"
#include <ReactiveLitepp/ObservableObject.h>

/**
 * @file relative_rotary_sensor.h
 * @brief Relative rotary sensor class (dial/knob)
 */

namespace hue4cpp {

	/**
	 * @brief Represents a relative rotary sensor (dial/knob)
	 */
	class RelativeRotarySensor : public Sensor {
	public:
		/**
		 * @brief Construct a RelativeRotarySensor
		 * @param id Sensor unique identifier
		 * @param bridge Pointer to parent bridge
		 */
		RelativeRotarySensor(const std::string& id, Bridge* bridge);

		/**
		 * @brief Get the sensor type
		 * @return SensorType::RelativeRotary
		 */
		SensorType getType() const override;

    /**
     * @brief Initialize relative rotary sensor state from JSON data
     * @param json JSON object containing sensor data from API
     */
    void initFromJson(const nlohmann::json& json) override;

    /** @brief Number of rotation steps in the last event (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<int32_t> Steps{
        [this]() { return _steps; }
    };

		/** @brief Direction of the last rotation (reactive, read-only) */
		ReactiveLitepp::ReadonlyProperty<RotationDirection> Direction{
			[this]() { return _direction; }
		};

		/** @brief Rotation action associated with the last rotation event (reactive, read-only) */
		ReactiveLitepp::ReadonlyProperty<RotationAction> Action{
			[this]() { return _rotationAction; }
		};

		/** @brief Duration of the rotation event in milliseconds (reactive, read-only) */
		ReactiveLitepp::ReadonlyProperty<int32_t> Duration{
			[this]() { return _duration; }
		};


	private:
		int32_t           _steps = 0;
		int32_t           _duration = 0;
		RotationDirection _direction = RotationDirection::Unknown;
		RotationAction    _rotationAction = RotationAction::Unknown;

		void notifyStateProperties(const nlohmann::json& delta) override;

		friend class Bridge;
	};

} // namespace hue4cpp
