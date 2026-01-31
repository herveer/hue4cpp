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

		Impl() : bridge(nullptr), type(SensorType::Unknown) {}

		Impl(const std::string& sensor_id, Bridge* parent_bridge)
			: id(sensor_id), bridge(parent_bridge), type(SensorType::Unknown) {
		}

		// Helper to get the resource type string from SensorType enum
		std::string getResourceTypeString() const {
			switch (type) {
			case SensorType::Motion:
				return "motion";
			case SensorType::Temperature:
				return "temperature";
			case SensorType::LightLevel:
				return "light_level";
			case SensorType::Button:
				return "button";
			default:
				return "";
			}
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
		// Extract enabled state from cached JSON
		std::function extractEnabled = [](const std::string& state_json) -> std::optional<bool> {
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("enabled") && state["enabled"].is_boolean()) {
						return state["enabled"].template get<bool>();
					}
				}
				catch (...) {
					// Fall through to refresh cache or return default
				}
			}
			return std::nullopt;
		};

		if (!pImpl->bridge) {
			// No bridge - cannot get state, return false
			return false;
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = pImpl->bridge->getSensorState(pImpl->id, pImpl->getResourceTypeString(), false);
		auto enabled_opt = extractEnabled(state_json);
		if (enabled_opt.has_value()) {
			return enabled_opt.value();
		}

		// Try refreshing cache
		state_json = pImpl->bridge->getSensorState(pImpl->id, pImpl->getResourceTypeString(), true);
		enabled_opt = extractEnabled(state_json);
		return enabled_opt.value_or(false);
	}

	std::optional<MotionState> Sensor::getMotionState() const {
		// Extract motion state from cached JSON
		std::function extractMotion = [](const std::string& state_json) -> std::optional<MotionState> {
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("motion") && state["motion"].is_object()) {
						auto motion_obj = state["motion"];
						MotionState motion_state;
						motion_state.motion = json_utils::getValueOr<bool>(motion_obj, "motion", false);
						motion_state.motion_valid = json_utils::getValueOr<bool>(motion_obj, "motion_valid", false);
						return motion_state;
					}
				}
				catch (...) {
					// Fall through to refresh cache or return nullopt
				}
			}
			return std::nullopt;
		};

		if (!pImpl->bridge || pImpl->type != SensorType::Motion) {
			return std::nullopt;
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = pImpl->bridge->getSensorState(pImpl->id, pImpl->getResourceTypeString(), false);
		auto motion_opt = extractMotion(state_json);
		if (motion_opt.has_value()) {
			return motion_opt;
		}

		// Try refreshing cache
		state_json = pImpl->bridge->getSensorState(pImpl->id, pImpl->getResourceTypeString(), true);
		return extractMotion(state_json);
	}

	std::optional<TemperatureState> Sensor::getTemperatureState() const {
		// Extract temperature state from cached JSON
		std::function extractTemperature = [](const std::string& state_json) -> std::optional<TemperatureState> {
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("temperature") && state["temperature"].is_object()) {
						auto temp_obj = state["temperature"];
						TemperatureState temp_state;
						// Temperature is in deci-degrees Celsius (divide by 100)
						int temp_raw = json_utils::getValueOr<int>(temp_obj, "temperature", 0);
						temp_state.temperature = temp_raw / 100.0f;
						temp_state.temperature_valid = json_utils::getValueOr<bool>(temp_obj, "temperature_valid", true);
						return temp_state;
					}
				}
				catch (...) {
					// Fall through to refresh cache or return nullopt
				}
			}
			return std::nullopt;
		};

		if (!pImpl->bridge || pImpl->type != SensorType::Temperature) {
			return std::nullopt;
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = pImpl->bridge->getSensorState(pImpl->id, pImpl->getResourceTypeString(), false);
		auto temp_opt = extractTemperature(state_json);
		if (temp_opt.has_value()) {
			return temp_opt;
		}

		// Try refreshing cache
		state_json = pImpl->bridge->getSensorState(pImpl->id, pImpl->getResourceTypeString(), true);
		return extractTemperature(state_json);
	}

	std::optional<LightLevelState> Sensor::getLightLevelState() const {
		// Extract light level state from cached JSON
		std::function extractLightLevel = [](const std::string& state_json) -> std::optional<LightLevelState> {
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("light") && state["light"].is_object()) {
						auto light_obj = state["light"];
						LightLevelState light_state;
						light_state.light_level = json_utils::getValueOr<uint32_t>(light_obj, "light_level", 0);
						light_state.light_level_valid = json_utils::getValueOr<bool>(light_obj, "light_level_valid", false);
						return light_state;
					}
				}
				catch (...) {
					// Fall through to refresh cache or return nullopt
				}
			}
			return std::nullopt;
		};

		if (!pImpl->bridge || pImpl->type != SensorType::LightLevel) {
			return std::nullopt;
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = pImpl->bridge->getSensorState(pImpl->id, pImpl->getResourceTypeString(), false);
		auto light_opt = extractLightLevel(state_json);
		if (light_opt.has_value()) {
			return light_opt;
		}

		// Try refreshing cache
		state_json = pImpl->bridge->getSensorState(pImpl->id, pImpl->getResourceTypeString(), true);
		return extractLightLevel(state_json);
	}

	std::optional<ButtonState> Sensor::getButtonState() const {
		// Extract button state from cached JSON
		std::function extractButton = [](const std::string& state_json) -> std::optional<ButtonState> {
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("button") && state["button"].is_object()) {
						auto button_obj = state["button"];
						ButtonState button_state;
						std::string event_str = json_utils::getValueOr<std::string>(button_obj, "last_event", "");
						button_state.last_event = parseButtonEvent(event_str);
						button_state.event_sequence = json_utils::getValueOr<uint32_t>(button_obj, "event_sequence", 0);
						// Get button control_id if available (for multi-button devices)
						if (state.contains("metadata") && state["metadata"].contains("control_id")) {
							button_state.button_id = json_utils::getValueOr<uint32_t>(state["metadata"], "control_id", 0);
						}
						return button_state;
					}
				}
				catch (...) {
					// Fall through to refresh cache or return nullopt
				}
			}
			return std::nullopt;
		};

		if (!pImpl->bridge || pImpl->type != SensorType::Button) {
			return std::nullopt;
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = pImpl->bridge->getSensorState(pImpl->id, pImpl->getResourceTypeString(), false);
		auto button_opt = extractButton(state_json);
		if (button_opt.has_value()) {
			return button_opt;
		}

		// Try refreshing cache
		state_json = pImpl->bridge->getSensorState(pImpl->id, pImpl->getResourceTypeString(), true);
		return extractButton(state_json);
	}

	Result<void> Sensor::refresh() {
		if (!pImpl->bridge) {
			return Result<void>(ErrorCode::InvalidParameter, "No bridge associated with sensor");
		}

		// Force a cache refresh by calling getSensorState with refreshCache=true
		std::string state = pImpl->bridge->getSensorState(pImpl->id, pImpl->getResourceTypeString(), true);
		
		if (state.empty()) {
			return Result<void>(ErrorCode::NetworkError, "Failed to refresh sensor state");
		}

		return Result<void>();
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

			// Cache the full JSON state in StateManager if we have a bridge
			// This allows the getters to retrieve the state from cache
			if (pImpl->bridge && !pImpl->id.empty()) {
				pImpl->bridge->getStateManager().setResourceState(pImpl->id, json.dump());
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing sensor JSON: " << e.what() << std::endl;
		}
	}

} // namespace hue4cpp
