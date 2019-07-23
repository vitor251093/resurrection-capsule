
#ifndef _GAME_CONFIG_HEADER
#define _GAME_CONFIG_HEADER

// Include
#include <string>
#include <array>

// Game
namespace Game {
	enum ConfigValue {
		CONFIG_SKIP_LAUNCHER = 0,
		CONFIG_VERSION_LOCKED,
		CONFIG_SINGLEPLAYER_ONLY,
		CONFIG_SERVER_HOST,
		CONFIG_STORAGE_PATH,
		CONFIG_DARKSPORE_INDEX_PAGE_PATH,
		CONFIG_DARKSPORE_REGISTER_PAGE_PATH,
		CONFIG_DARKSPORE_LAUNCHER_NOTES_PATH,
		CONFIG_DARKSPORE_LAUNCHER_THEMES_PATH,
		CONFIG_END
	};

	// Config
	class Config {
		public:
			static void Load(const std::string& path);

			static const std::string& Get(ConfigValue key);
			static bool GetBool(ConfigValue key);
			static void Set(ConfigValue key, const std::string& value);

		private:
			static void GenerateDefault(const std::string& path);
		
		private:
			static std::array<std::string, CONFIG_END> mConfig;
	};
}

#endif
