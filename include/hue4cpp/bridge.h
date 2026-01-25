#pragma once

#include "types.h"
#include <vector>
#include <memory>
#include <string>

/**
 * @file bridge.h
 * @brief Bridge discovery and connection management
 */

namespace hue4cpp {

	// Forward declarations
	class Light;
	class StateManager;

	/**
	 * @brief Represents a Philips Hue Bridge
	 *
	 * The Bridge class is the main entry point for interacting with a Hue system.
	 * It handles discovery, authentication, and provides access to lights and other resources.
	 */
	class Bridge {
	public:
		/**
		 * @brief Default constructor
		 */
		Bridge();

		/**
		 * @brief Construct a Bridge with known connection info
		 * @param info Bridge connection information
		 */
		explicit Bridge(const BridgeInfo& info);

		/**
		 * @brief Destructor
		 */
		~Bridge();

		// Prevent copying, allow moving
		Bridge(const Bridge&) = delete;
		Bridge& operator=(const Bridge&) = delete;
		Bridge(Bridge&&) noexcept;
		Bridge& operator=(Bridge&&) noexcept;

		/**
		 * @brief Discover Hue bridges on the local network
		 * @return Vector of discovered bridges
		 */
		static std::vector<Bridge> discover();

		/**
		 * @brief Discover bridges using mDNS
		 * @return Vector of discovered bridges
		 */
		static std::vector<Bridge> discoverMDNS();

		/**
		 * @brief Discover bridges using N-UPnP (broker-based)
		 * @return Vector of discovered bridges
		 */
		static std::vector<Bridge> discoverNUPnP();

		/**
		 * @brief Authenticate with the bridge
		 * @param app_name Name of the application
		 * @param device_name Name of the device (optional)
		 * @return Result indicating success or failure
		 */
		Result<std::string> authenticate(const std::string& app_name,
			const std::string& device_name = "");

		/**
		 * @brief Set authentication key for the bridge
		 * @param key Previously obtained authentication key
		 */
		void setAuthenticationKey(const std::string& key);

		/**
		 * @brief Get the current authentication key
		 * @return The authentication key, or empty string if not authenticated
		 */
		std::string getAuthenticationKey() const;

		/**
		 * @brief Check if bridge is authenticated
		 * @return true if authenticated, false otherwise
		 */
		bool isAuthenticated() const;

		/**
		 * @brief Validate the current authentication key with the bridge
		 * @return Result indicating if the key is valid
		 */
		Result<void> validateAuthentication() const;

		/**
		 * @brief Get all lights connected to this bridge
		 * @return Vector of Light objects
		 */
		std::vector<Light> getLights();

		/**
		 * @brief Get a specific light by ID
		 * @param light_id The unique identifier of the light
		 * @return Optional Light object
		 */
		std::optional<Light> getLight(const std::string& light_id);

		/**
		 * @brief Get bridge information
		 * @return BridgeInfo structure
		 */
		const BridgeInfo& getInfo() const;

		/**
		 * @brief Get the state manager for this bridge
		 * @return Reference to StateManager
		 */
		StateManager& getStateManager();

		/**
		 * @brief Get light state (cache-first, API-fallback)
		 * @param light_id The unique identifier of the light
		 * @param refreshCache If true, forces fetching the complete state from the bridge API
		 * @return JSON string with light state, or empty if not found
		 */
		std::string getLightState(const std::string& light_id, bool refreshCache);

		/**
		 * @brief Check if the bridge is reachable
		 * @return true if reachable, false otherwise
		 */
		bool isReachable() const;

	private:
		class Impl;
		std::unique_ptr<Impl> pImpl;
	};

} // namespace hue4cpp
