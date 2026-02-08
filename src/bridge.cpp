#include "hue4cpp/bridge.h"
#include "hue4cpp/light.h"
#include "hue4cpp/sensors.h"
#include "hue4cpp/state.h"
#include "hue4cpp/http_client.h"
#include "hue4cpp/json_utils.h"
#include "hue4cpp/exceptions.h"
#include <chrono>
#include <thread>

namespace hue4cpp {

	namespace {
		// Constants for authentication
		constexpr std::chrono::milliseconds AUTH_TIMEOUT{ 5000 };  // 5 seconds timeout for auth requests
		constexpr int MAX_AUTH_RETRIES = 30;  // Maximum number of retries (30 seconds with 1 second interval)
		constexpr std::chrono::milliseconds AUTH_RETRY_INTERVAL{ 1000 };  // 1 second between retries
	}

	// Bridge implementation
	Bridge::Bridge() : state_manager_(std::make_unique<StateManager>(this)) {}

	Bridge::Bridge(const BridgeInfo& info)
		: info_(info), state_manager_(std::make_unique<StateManager>(this)) {
	}

	Bridge::~Bridge() {
		// Stop SSE connection when bridge is destroyed
		if (state_manager_ && state_manager_->isRunning()) {
			state_manager_->stop();
		}
	}


	Bridge::Bridge(Bridge&& other) noexcept
		: info_(std::move(other.info_))
		, auth_key_(std::move(other.auth_key_))
		, state_manager_(std::move(other.state_manager_))
	{
		if (state_manager_) {
			state_manager_->setBridge(this);
		}
	}

	Bridge& Bridge::operator=(Bridge&& other) noexcept {
		if (this != &other) {
			info_ = std::move(other.info_);
			auth_key_ = std::move(other.auth_key_);
			state_manager_ = std::move(other.state_manager_);
			if (state_manager_) {
				state_manager_->setBridge(this);
			}
		}
		return *this;
	}

	// Template method implementation - must be in source file to access sensor types
	template<typename SensorType>
	std::vector<std::unique_ptr<SensorType>> Bridge::fetchSensorsByType(const std::string& resource_type) {
		if (auth_key_.empty() || info_.ip_address.empty()) {
			return {};
		}

		try {
			HttpClient client;
			client.setVerifySsl(false);
			client.setTimeout(std::chrono::milliseconds(5000));

			std::string url = "https://" + info_.ip_address + "/clip/v2/resource/" + resource_type;

			std::map<std::string, std::string> headers;
			headers["hue-application-key"] = auth_key_;

			auto response = client.get(url, headers);

			if (!response.isSuccess()) {
				return {};
			}

			auto json_response = json_utils::parse(response.body);

			// Check if response contains errors
			if (json_response.contains("errors") && json_response["errors"].is_array()) {
				auto errors = json_response["errors"];
				if (!errors.empty()) {
					return {};
				}
			}

			// Extract sensors from the data array
			if (!json_response.contains("data") || !json_response["data"].is_array()) {
				return {};
			}

			std::vector<std::unique_ptr<SensorType>> sensors;
			auto data = json_response["data"];

			for (const auto& sensor_data : data) {
				std::string id = json_utils::getValueOr<std::string>(sensor_data, "id", "");
				if (!id.empty()) {
					auto sensor = std::make_unique<SensorType>(id, this);
					sensor->updateFromJson(sensor_data);
					sensors.push_back(std::move(sensor));
				}
			}

			return sensors;

		}
		catch (const std::exception&) {
			return {};
		}
	}


	// Discovery methods are implemented in discovery.cpp

	Result<std::string> Bridge::authenticate(const std::string& app_name,
		const std::string& device_name) {
		if (info_.ip_address.empty()) {
			return Result<std::string>(ErrorCode::InvalidParameter,
				"Bridge IP address is not set");
		}

		if (app_name.empty()) {
			return Result<std::string>(ErrorCode::InvalidParameter,
				"Application name cannot be empty");
		}

		// Construct device type string
		std::string devicetype = app_name;
		if (!device_name.empty()) {
			devicetype += "#" + device_name;
		}

		// Create HTTP client
		HttpClient client;
		client.setVerifySsl(false);  // Hue bridges use self-signed certificates
		client.setTimeout(AUTH_TIMEOUT);

		// Construct the URL for authentication
		std::string url = "https://" + info_.ip_address + "/api";

		// Create JSON request body
		nlohmann::json request_body;
		request_body["devicetype"] = devicetype;
		std::string json_str = json_utils::toString(request_body);

		// Try to authenticate with retries
		for (int retry = 0; retry < MAX_AUTH_RETRIES; ++retry) {
			auto response = client.post(url, json_str);

			if (!response.isSuccess()) {
				// Network error
				return Result<std::string>(ErrorCode::NetworkError,
					"Failed to connect to bridge: " + response.error_message);
			}

			try {
				auto json_response = json_utils::parse(response.body);

				// The response is an array of status objects
				if (!json_response.is_array() || json_response.empty()) {
					return Result<std::string>(ErrorCode::InvalidRequest,
						"Invalid response from bridge");
				}

				auto first_item = json_response[0];

				// Check for success
				if (first_item.contains("success")) {
					auto success = first_item["success"];
					if (success.contains("username")) {
						std::string username = success["username"].get<std::string>();
						auth_key_ = username;
						// Note: SSE connection must be manually started via getStateManager().start()
						return Result<std::string>(username);
					}
				}

				// Check for error
				if (first_item.contains("error")) {
					auto error = first_item["error"];
					int error_type = json_utils::getValueOr<int>(error, "type", 0);
					std::string error_desc = json_utils::getValueOr<std::string>(
						error, "description", "Unknown error");

					// Error type 101 means link button not pressed
					if (error_type == 101) {
						// Continue retrying
						if (retry < MAX_AUTH_RETRIES - 1) {
							std::this_thread::sleep_for(AUTH_RETRY_INTERVAL);
							continue;
						}
						else {
							return Result<std::string>(ErrorCode::AuthenticationFailed,
								"Link button not pressed within timeout period. " + error_desc);
						}
					}

					// Other errors are fatal
					return Result<std::string>(ErrorCode::AuthenticationFailed, error_desc);
				}

				// Unknown response format
				return Result<std::string>(ErrorCode::InvalidRequest,
					"Unexpected response from bridge");

			}
			catch (const JsonParseException& e) {
				return Result<std::string>(ErrorCode::InvalidRequest,
					std::string("Failed to parse bridge response: ") + e.what());
			}
			catch (const std::exception& e) {
				return Result<std::string>(ErrorCode::UnknownError,
					std::string("Authentication error: ") + e.what());
			}
		}

		// Should not reach here, but just in case
		return Result<std::string>(ErrorCode::TimeoutError,
			"Authentication timed out");
	}

	void Bridge::setAuthenticationKey(const std::string& key) {
		auth_key_ = key;
		state_manager_->stop(); // Stop any existing SSE connection if auth key changes
	}

	std::string Bridge::getAuthenticationKey() const {
		return auth_key_;
	}

	bool Bridge::isAuthenticated() const {
		return !auth_key_.empty();
	}

	Result<void> Bridge::validateAuthentication() const {
		if (auth_key_.empty()) {
			return Result<void>(ErrorCode::AuthenticationRequired,
				"No authentication key set");
		}

		if (info_.ip_address.empty()) {
			return Result<void>(ErrorCode::InvalidParameter,
				"Bridge IP address is not set");
		}

		try {
			HttpClient client;
			client.setVerifySsl(false);  // Hue bridges use self-signed certificates
			client.setTimeout(AUTH_TIMEOUT);

			// Try to access the config endpoint with the authentication key
			std::string url = "https://" + info_.ip_address +
				"/clip/v2/resource/bridge";

			std::map<std::string, std::string> headers;
			headers["hue-application-key"] = auth_key_;

			auto response = client.get(url, headers);

			if (!response.isSuccess()) {
				if (response.status_code == 401 || response.status_code == 403) {
					return Result<void>(ErrorCode::AuthenticationFailed,
						"Invalid authentication key");
				}
				return Result<void>(ErrorCode::NetworkError,
					"Failed to validate authentication: " + response.error_message);
			}

			// Parse response to check for errors
			try {
				auto json_response = json_utils::parse(response.body);

				// Check if response contains an error
				if (json_response.contains("errors") && json_response["errors"].is_array()) {
					auto errors = json_response["errors"];
					if (!errors.empty()) {
						std::string error_desc = json_utils::getValueOr<std::string>(
							errors[0], "description", "Unknown error");
						return Result<void>(ErrorCode::AuthenticationFailed, error_desc);
					}
				}

				// If we got here, authentication is valid
				return Result<void>();

			}
			catch (const JsonParseException&) {
				// If we can't parse the response but got a 200, assume it's valid
				return Result<void>();
			}

		}
		catch (const std::exception& e) {
			return Result<void>(ErrorCode::UnknownError,
				std::string("Validation error: ") + e.what());
		}
	}

	std::vector<Light> Bridge::getLights() {
		if (!isAuthenticated()) {
			return {};
		}

		if (info_.ip_address.empty()) {
			return {};
		}

		try {
			HttpClient client;
			client.setVerifySsl(false);
			client.setTimeout(std::chrono::milliseconds(5000));

			std::string url = "https://" + info_.ip_address + "/clip/v2/resource/light";

			std::map<std::string, std::string> headers;
			headers["hue-application-key"] = auth_key_;

			auto response = client.get(url, headers);

			if (!response.isSuccess()) {
				return {};
			}

			auto json_response = json_utils::parse(response.body);

			// Check if response contains errors
			if (json_response.contains("errors") && json_response["errors"].is_array()) {
				auto errors = json_response["errors"];
				if (!errors.empty()) {
					return {};
				}
			}

			// Extract lights from the data array
			if (!json_response.contains("data") || !json_response["data"].is_array()) {
				return {};
			}

			std::vector<Light> lights;
			auto data = json_response["data"];

			for (const auto& light_data : data) {
				std::string id = json_utils::getValueOr<std::string>(light_data, "id", "");
				if (!id.empty()) {
					Light light(id, this);
					// Parse and set light properties from JSON
					light.updateFromJson(light_data);
					lights.push_back(std::move(light));
				}
			}

			return lights;

		}
		catch (const std::exception&) {
			return {};
		}
	}

	std::optional<Light> Bridge::getLight(const std::string& light_id) {
		if (!isAuthenticated() || info_.ip_address.empty() || light_id.empty()) {
			return std::nullopt;
		}

		try {
			HttpClient client;
			client.setVerifySsl(false);
			client.setTimeout(std::chrono::milliseconds(5000));

			std::string url = "https://" + info_.ip_address +
				"/clip/v2/resource/light/" + light_id;

			std::map<std::string, std::string> headers;
			headers["hue-application-key"] = auth_key_;

			auto response = client.get(url, headers);

			if (!response.isSuccess()) {
				return std::nullopt;
			}

			auto json_response = json_utils::parse(response.body);

			// Check if response contains errors
			if (json_response.contains("errors") && json_response["errors"].is_array()) {
				auto errors = json_response["errors"];
				if (!errors.empty()) {
					return std::nullopt;
				}
			}

			// Extract light from the data array
			if (!json_response.contains("data") || !json_response["data"].is_array()) {
				return std::nullopt;
			}

			auto data = json_response["data"];
			if (data.empty()) {
				return std::nullopt;
			}

			Light light(light_id, this);
			light.updateFromJson(data[0]);
			return light;

		}
		catch (const std::exception&) {
			return std::nullopt;
		}
	}

	void Bridge::setInfo(const BridgeInfo& info) {
		info_ = info;
		state_manager_->stop(); // Stop SSE connection if bridge info changes
	}

	const BridgeInfo& Bridge::getInfo() const {
		return info_;
	}

	StateManager& Bridge::getStateManager() {
		return *state_manager_;
	}

	std::string Bridge::getLightState(const std::string& light_id, bool refreshCache) {
		// First, check StateManager cache
		if (!refreshCache) {
			std::string cached_state = state_manager_->getResourceState(light_id);
			if (!cached_state.empty()) {
				return cached_state;
			}
		}

		// Cache miss - fetch from bridge API
		if (!isAuthenticated() || info_.ip_address.empty() || light_id.empty()) {
			return "";
		}

		try {
			HttpClient client;
			client.setVerifySsl(false);
			client.setTimeout(std::chrono::milliseconds(5000));

			std::string url = "https://" + info_.ip_address +
				"/clip/v2/resource/light/" + light_id;

			std::map<std::string, std::string> headers;
			headers["hue-application-key"] = auth_key_;

			auto response = client.get(url, headers);

			if (!response.isSuccess()) {
				return "";
			}

			auto json_response = json_utils::parse(response.body);

			// Check if response contains errors
			if (json_response.contains("errors") && json_response["errors"].is_array()) {
				auto errors = json_response["errors"];
				if (!errors.empty()) {
					return "";
				}
			}

			// Extract light from the data array
			if (!json_response.contains("data") || !json_response["data"].is_array()) {
				return "";
			}

			auto data = json_response["data"];
			if (data.empty()) {
				return "";
			}

			// Update cache with complete state from API
			std::string full_state = data[0].dump();
			state_manager_->setResourceState(light_id, full_state);
			return full_state;

		}
		catch (const std::exception&) {
			return "";
		}
	}

	std::string Bridge::getSensorState(const std::string& sensor_id, const std::string& sensor_type, bool refreshCache) {
		// First, check StateManager cache
		if (!refreshCache) {
			std::string cached_state = state_manager_->getResourceState(sensor_id);
			if (!cached_state.empty()) {
				return cached_state;
			}
		}

		// Cache miss - fetch from bridge API
		if (!isAuthenticated() || info_.ip_address.empty() || sensor_id.empty() || sensor_type.empty()) {
			return "";
		}

		try {
			HttpClient client;
			client.setVerifySsl(false);
			client.setTimeout(std::chrono::milliseconds(5000));

			std::string url = "https://" + info_.ip_address +
				"/clip/v2/resource/" + sensor_type + "/" + sensor_id;

			std::map<std::string, std::string> headers;
			headers["hue-application-key"] = auth_key_;

			auto response = client.get(url, headers);

			if (!response.isSuccess()) {
				return "";
			}

			auto json_response = json_utils::parse(response.body);

			// Check if response contains errors
			if (json_response.contains("errors") && json_response["errors"].is_array()) {
				auto errors = json_response["errors"];
				if (!errors.empty()) {
					return "";
				}
			}

			// Extract sensor from the data array
			if (!json_response.contains("data") || !json_response["data"].is_array()) {
				return "";
			}

			auto data = json_response["data"];
			if (data.empty()) {
				return "";
			}

			// Update cache with complete state from API
			std::string full_state = data[0].dump();
			state_manager_->setResourceState(sensor_id, full_state);
			return full_state;

		}
		catch (const std::exception&) {
			return "";
		}
	}

	std::vector<std::unique_ptr<Sensor>> Bridge::getSensors() {
		std::vector<std::unique_ptr<Sensor>> all_sensors;

		// Collect all sensor types
		auto motion = getMotionSensors();
		auto temperature = getTemperatureSensors();
		auto light_level = getLightLevelSensors();
		auto buttons = getButtonSensors();
		auto camera_motion = getCameraMotionSensors();
		auto bell_buttons = getBellButtonSensors();
		auto relative_rotary = getRelativeRotarySensors();
		auto geolocation = getGeolocationSensors();
		auto tamper = getTamperSensors();

		// Move them into the base class vector
		for (auto& sensor : motion) {
			all_sensors.push_back(std::move(sensor));
		}
		for (auto& sensor : temperature) {
			all_sensors.push_back(std::move(sensor));
		}
		for (auto& sensor : light_level) {
			all_sensors.push_back(std::move(sensor));
		}
		for (auto& sensor : buttons) {
			all_sensors.push_back(std::move(sensor));
		}
		for (auto& sensor : camera_motion) {
			all_sensors.push_back(std::move(sensor));
		}
		for (auto& sensor : bell_buttons) {
			all_sensors.push_back(std::move(sensor));
		}
		for (auto& sensor : relative_rotary) {
			all_sensors.push_back(std::move(sensor));
		}
		for (auto& sensor : geolocation) {
			all_sensors.push_back(std::move(sensor));
		}
		for (auto& sensor : tamper) {
			all_sensors.push_back(std::move(sensor));
		}

		return all_sensors;
	}

	std::unique_ptr<Sensor> Bridge::getSensor(const std::string& sensor_id) {
		// Try each sensor type endpoint to find the sensor
		std::vector<std::string> resource_types = { 
			"motion", "temperature", "light_level", "button",
			"camera_motion", "bell_button", "relative_rotary", "geolocation", "tamper"
		};

		for (const auto& resource_type : resource_types) {
			if (!isAuthenticated() || info_.ip_address.empty() || sensor_id.empty()) {
				continue;
			}

			try {
				HttpClient client;
				client.setVerifySsl(false);
				client.setTimeout(std::chrono::milliseconds(5000));

				std::string url = "https://" + info_.ip_address +
					"/clip/v2/resource/" + resource_type + "/" + sensor_id;

				std::map<std::string, std::string> headers;
				headers["hue-application-key"] = auth_key_;

				auto response = client.get(url, headers);

				if (!response.isSuccess()) {
					continue; // Try next resource type
				}

				auto json_response = json_utils::parse(response.body);

				// Check if response contains errors
				if (json_response.contains("errors") && json_response["errors"].is_array()) {
					auto errors = json_response["errors"];
					if (!errors.empty()) {
						continue;
					}
				}

				// Extract sensor from the data array
				if (!json_response.contains("data") || !json_response["data"].is_array()) {
					continue;
				}

				auto data = json_response["data"];
				if (data.empty()) {
					continue;
				}

				// Use factory to create the right sensor type
				auto sensor = createSensorFromJson(data[0], this);
				return sensor;

			}
			catch (const std::exception&) {
				continue;
			}
		}

		return nullptr;
	}

	std::vector<std::unique_ptr<MotionSensor>> Bridge::getMotionSensors() {
		if (!isAuthenticated()) {
			return {};
		}
		return fetchSensorsByType<MotionSensor>("motion");
	}

	std::vector<std::unique_ptr<TemperatureSensor>> Bridge::getTemperatureSensors() {
		if (!isAuthenticated()) {
			return {};
		}
		return fetchSensorsByType<TemperatureSensor>("temperature");
	}

	std::vector<std::unique_ptr<LightLevelSensor>> Bridge::getLightLevelSensors() {
		if (!isAuthenticated()) {
			return {};
		}
		return fetchSensorsByType<LightLevelSensor>("light_level");
	}

	std::vector<std::unique_ptr<ButtonSensor>> Bridge::getButtonSensors() {
		if (!isAuthenticated()) {
			return {};
		}
		return fetchSensorsByType<ButtonSensor>("button");
	}

	std::vector<std::unique_ptr<CameraMotionSensor>> Bridge::getCameraMotionSensors() {
		if (!isAuthenticated()) {
			return {};
		}
		return fetchSensorsByType<CameraMotionSensor>("camera_motion");
	}

	std::vector<std::unique_ptr<BellButtonSensor>> Bridge::getBellButtonSensors() {
		if (!isAuthenticated()) {
			return {};
		}
		return fetchSensorsByType<BellButtonSensor>("bell_button");
	}

	std::vector<std::unique_ptr<RelativeRotarySensor>> Bridge::getRelativeRotarySensors() {
		if (!isAuthenticated()) {
			return {};
		}
		return fetchSensorsByType<RelativeRotarySensor>("relative_rotary");
	}

	std::vector<std::unique_ptr<GeolocationSensor>> Bridge::getGeolocationSensors() {
		if (!isAuthenticated()) {
			return {};
		}
		return fetchSensorsByType<GeolocationSensor>("geolocation");
	}

	std::vector<std::unique_ptr<TamperSensor>> Bridge::getTamperSensors() {
		if (!isAuthenticated()) {
			return {};
		}
		return fetchSensorsByType<TamperSensor>("tamper");
	}

	// isReachable is implemented in discovery.cpp

} // namespace hue4cpp
