#include "hue4cpp/sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/http_client.h"
#include "hue4cpp/json_utils.h"
#include <iostream>

namespace hue4cpp {

	namespace {
		// Helper function to parse button event from string
		ButtonEvent parseButtonEvent(const std::string& event_str) {
			if (event_str == "initial_press") return ButtonEvent::InitialPress;
			if (event_str == "short_release") return ButtonEvent::ShortRelease;
			if (event_str == "long_release") return ButtonEvent::LongRelease;
			if (event_str == "long_press") return ButtonEvent::LongPress;
			if (event_str == "double_short_release") return ButtonEvent::DoubleShortRelease;
			if (event_str == "repeat") return ButtonEvent::Repeat;
			return ButtonEvent::Unknown;
		}

		// Helper function to determine sensor type from API type string
		SensorType parseSensorType(const std::string& type_str) {
			if (type_str == "motion") return SensorType::Motion;
			if (type_str == "temperature") return SensorType::Temperature;
			if (type_str == "light_level") return SensorType::LightLevel;
			if (type_str == "button") return SensorType::Button;
			return SensorType::Unknown;
		}
	}

	class Sensor::Impl {
	public:
		std::string id;
		Bridge* bridge;
		SensorType type;
		bool enabled;

		// Sensor-specific state
		std::optional<MotionState> motion_state;
		std::optional<TemperatureState> temperature_state;
		std::optional<LightLevelState> light_level_state;
		std::optional<ButtonState> button_state;

		Impl() : bridge(nullptr), type(SensorType::Unknown), enabled(false) {}

		Impl(const std::string& sensor_id, Bridge* parent_bridge)
			: id(sensor_id), bridge(parent_bridge), type(SensorType::Unknown), enabled(false) {
		}
	};

	// Sensor implementation
	Sensor::Sensor() : pImpl(std::make_unique<Impl>()) {}

	Sensor::Sensor(const std::string& id, Bridge* bridge)
		: pImpl(std::make_unique<Impl>(id, bridge)) {
	}

	Sensor::~Sensor() = default;

	Sensor::Sensor(const Sensor& other) : pImpl(std::make_unique<Impl>(*other.pImpl)) {}

	Sensor& Sensor::operator=(const Sensor& other) {
		if (this != &other) {
			pImpl = std::make_unique<Impl>(*other.pImpl);
		}
		return *this;
	}

	Sensor::Sensor(Sensor&&) noexcept = default;

	Sensor& Sensor::operator=(Sensor&&) noexcept = default;

	std::string Sensor::getId() const {
		return pImpl->id;
	}

	SensorType Sensor::getType() const {
		return pImpl->type;
	}

	bool Sensor::isEnabled() const {
		return pImpl->enabled;
	}

	std::optional<MotionState> Sensor::getMotionState() const {
		return pImpl->motion_state;
	}

	std::optional<TemperatureState> Sensor::getTemperatureState() const {
		return pImpl->temperature_state;
	}

	std::optional<LightLevelState> Sensor::getLightLevelState() const {
		return pImpl->light_level_state;
	}

	std::optional<ButtonState> Sensor::getButtonState() const {
		return pImpl->button_state;
	}

	Result<void> Sensor::refresh() {
		if (!pImpl->bridge || !pImpl->bridge->isAuthenticated()) {
			return Result<void>(ErrorCode::AuthenticationRequired,
				"Bridge not authenticated");
		}

		const auto& bridge_info = pImpl->bridge->getInfo();
		if (bridge_info.ip_address.empty() || pImpl->id.empty()) {
			return Result<void>(ErrorCode::InvalidParameter,
				"Bridge IP or sensor ID not set");
		}

		try {
			HttpClient client;
			client.setVerifySsl(false);
			client.setTimeout(std::chrono::milliseconds(5000));

			// Determine the resource type endpoint based on sensor type
			std::string resource_type;
			switch (pImpl->type) {
			case SensorType::Motion:
				resource_type = "motion";
				break;
			case SensorType::Temperature:
				resource_type = "temperature";
				break;
			case SensorType::LightLevel:
				resource_type = "light_level";
				break;
			case SensorType::Button:
				resource_type = "button";
				break;
			default:
				return Result<void>(ErrorCode::InvalidParameter,
					"Unknown sensor type");
			}

			std::string url = "https://" + bridge_info.ip_address +
				"/clip/v2/resource/" + resource_type + "/" + pImpl->id;

			std::map<std::string, std::string> headers;
			headers["hue-application-key"] = pImpl->bridge->getAuthenticationKey();

			auto response = client.get(url, headers);

			if (!response.isSuccess()) {
				if (response.status_code == 401 || response.status_code == 403) {
					return Result<void>(ErrorCode::AuthenticationFailed,
						"Authentication failed");
				}
				if (response.status_code == 404) {
					return Result<void>(ErrorCode::ResourceNotFound,
						"Sensor not found");
				}
				return Result<void>(ErrorCode::NetworkError,
					"HTTP request failed: " + response.error_message);
			}

			// Parse response
			auto json_response = json_utils::parse(response.body);

			if (json_response.contains("errors") && json_response["errors"].is_array()) {
				auto errors = json_response["errors"];
				if (!errors.empty()) {
					std::string error_desc = json_utils::getValueOr<std::string>(
						errors[0], "description", "Unknown error");
					return Result<void>(ErrorCode::InvalidRequest, error_desc);
				}
			}

			// Update from response data
			if (json_response.contains("data") && json_response["data"].is_array() &&
				!json_response["data"].empty()) {
				updateFromJson(json_response["data"][0]);
			}

			return Result<void>();

		}
		catch (const JsonParseException& e) {
			return Result<void>(ErrorCode::InvalidRequest,
				std::string("JSON error: ") + e.what());
		}
		catch (const std::exception& e) {
			return Result<void>(ErrorCode::UnknownError,
				std::string("Error: ") + e.what());
		}
	}

	void Sensor::updateFromJson(const nlohmann::json& json) {
		try {
			// Update ID if present
			if (json.contains("id")) {
				pImpl->id = json_utils::getValueOr<std::string>(json, "id", pImpl->id);
			}

			// Update type if present
			if (json.contains("type")) {
				std::string type_str = json_utils::getValueOr<std::string>(json, "type", "");
				pImpl->type = parseSensorType(type_str);
			}

			// Update enabled state
			pImpl->enabled = json_utils::getValueOr<bool>(json, "enabled", true);

			// Parse sensor-specific data based on type
			if (pImpl->type == SensorType::Motion && json.contains("motion")) {
				MotionState state;
				auto motion_obj = json["motion"];
				state.motion = json_utils::getValueOr<bool>(motion_obj, "motion", false);
				state.motion_valid = json_utils::getValueOr<bool>(motion_obj, "motion_valid", false);
				pImpl->motion_state = state;
			}
			else if (pImpl->type == SensorType::Temperature && json.contains("temperature")) {
				TemperatureState state;
				auto temp_obj = json["temperature"];
				// Temperature is in deci-degrees Celsius (divide by 100)
				int temp_raw = json_utils::getValueOr<int>(temp_obj, "temperature", 0);
				state.temperature = temp_raw / 100.0f;
				state.temperature_valid = json_utils::getValueOr<bool>(temp_obj, "temperature_valid", true);
				pImpl->temperature_state = state;
			}
			else if (pImpl->type == SensorType::LightLevel && json.contains("light")) {
				LightLevelState state;
				auto light_obj = json["light"];
				state.light_level = json_utils::getValueOr<uint32_t>(light_obj, "light_level", 0);
				state.light_level_valid = json_utils::getValueOr<bool>(light_obj, "light_level_valid", false);
				pImpl->light_level_state = state;
			}
			else if (pImpl->type == SensorType::Button && json.contains("button")) {
				ButtonState state;
				auto button_obj = json["button"];
				std::string event_str = json_utils::getValueOr<std::string>(button_obj, "last_event", "");
				state.last_event = parseButtonEvent(event_str);
				state.event_sequence = json_utils::getValueOr<uint32_t>(button_obj, "event_sequence", 0);
				// Get button control_id if available (for multi-button devices)
				if (json.contains("metadata") && json["metadata"].contains("control_id")) {
					state.button_id = json_utils::getValueOr<uint32_t>(json["metadata"], "control_id", 0);
				}
				pImpl->button_state = state;
			}

		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing sensor JSON: " << e.what() << std::endl;
		}
	}

} // namespace hue4cpp
