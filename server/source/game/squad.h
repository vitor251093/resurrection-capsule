
#ifndef _GAME_SQUAD_HEADER
#define _GAME_SQUAD_HEADER

// Include
#include "creature.h"

// Game
namespace Game {
	// Squad
	struct Squad {
		Creatures creatures;

		std::string name;
		std::string category;

		uint32_t id = 0;
		uint32_t slot = 0;

		bool locked = true;

		void Read(const pugi::xml_node& node);
		void Write(pugi::xml_node& node) const;
	};

	// Squads
	class Squads {
		public:
			decltype(auto) begin() { return mSquads.begin(); }
			decltype(auto) begin() const { return mSquads.begin(); }
			decltype(auto) end() { return mSquads.end(); }
			decltype(auto) end() const { return mSquads.end(); }

			auto& data() { return mSquads; }
			const auto& data() const { return mSquads; }

			void Read(const pugi::xml_node& node);
			void Write(pugi::xml_node& node) const;

		private:
			std::vector<Squad> mSquads;
	};
}

#endif
